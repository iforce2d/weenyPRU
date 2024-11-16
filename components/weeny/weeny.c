/********************************************************************
* Description:  weeny.c
*               This file, 'weeny.c', is a HAL component that
*               provides an SPI connection to a external weeny PRU.
*  				
* Author: iforce2d
* License: GPL Version 2
*
*		Credit to Scott Alford for the 'Remora' system which inspired
*          this work and still forms the bulk of this particular file.
*		Portions of this code are based on stepgen.c by John Kasunich
*		and hm2_rpspi.c by Matsche
*
********************************************************************/

#include "rtapi.h"			/* RTAPI realtime OS API */
#include "rtapi_app.h"		/* RTAPI realtime module decls */
#include "hal.h"			/* HAL public API decls */

#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


// Using BCM2835 driver library by Mike McCauley, why reinvent the wheel!
// http://www.airspayce.com/mikem/bcm2835/index.html
// Include these in the source directory when using "halcompile --install weeny.c"
#include "bcm2835.h"
#include "bcm2835.c"

#include "weeny.h"


#define MODNAME "weeny"
#define PREFIX "weeny"

MODULE_AUTHOR("iforce2d");
MODULE_DESCRIPTION("Driver for weenyPRU");
MODULE_LICENSE("GPL v2");

char *ctrl_type[JOINTS] = { "p" };
RTAPI_MP_ARRAY_STRING(ctrl_type,JOINTS,"control type (pos or vel)");

/***********************************************************************
*                STRUCTURES AND GLOBAL VARIABLES                       *
************************************************************************/

typedef struct {
	hal_bit_t		*SPIenable;
	hal_bit_t		*SPIreset;
	hal_bit_t		*PRUreset;
	bool			SPIresetOld;
	hal_bit_t		*SPIstatus;
	hal_bit_t 		*stepperEnable[JOINTS];
	int				pos_mode[JOINTS];
	hal_float_t 	*pos_cmd[JOINTS];			// pin: position command (position units)
	hal_float_t 	*vel_cmd[JOINTS];			// pin: velocity command (position units/sec)
	hal_float_t 	*pos_fb[JOINTS];			// pin: position feedback (position units)
    //hal_s32_t		*count[JOINTS];				// pin: psition feedback (raw counts)
	hal_float_t 	pos_scale[JOINTS];			// param: steps per position unit
	float 			freq[JOINTS];				// param: frequency command sent to PRU
	hal_float_t 	*freq_cmd[JOINTS];			// pin: frequency command monitoring, available in LinuxCNC
	hal_float_t 	maxvel[JOINTS];				// param: max velocity, (pos units/sec)
	hal_float_t 	maxaccel[JOINTS];			// param: max accel (pos units/sec^2)
	hal_float_t		*pgain[JOINTS];
	hal_float_t		*ff1gain[JOINTS];
	hal_float_t		*deadband[JOINTS];
	float 			old_pos_cmd[JOINTS];		// previous position command (counts)
	float 			old_pos_cmd_raw[JOINTS];		// previous position command (counts)
	float 			old_scale[JOINTS];			// stored scale value
        //float 			scale_recip[JOINTS];		// reciprocal value used for scaling
	float			prev_cmd[JOINTS];
    //float			cmd_d[JOINTS];					// command derivative
    //hal_float_t 	*setPoint[VARIABLES];
    //hal_float_t 	*processVariable[VARIABLES];
    hal_s32_t       *jogcounts[3];          // analog0, analog1, rotary encoder
    hal_bit_t   	*outputs[DIGITAL_OUTPUTS];
    hal_bit_t   	*inputs[DIGITAL_INPUTS];
    hal_bit_t   	*rgb[RGB_BITS];
    hal_float_t 	*spindleSpeed[1];
    hal_float_t 	*adc[2]; // raw ADC values
    hal_s32_t       *microsteps[JOINTS];
    hal_s32_t       *rmsCurrent[JOINTS]; // use signed so that HAL configuration dialog shows eg. 200 instead of 0x000000c8
    hal_float_t     *pressure;
    hal_s32_t       *loadcell;
} data_t;

static data_t *data;

typedef struct __attribute__((__packed__))
{
        // this data goes from LinuxCNC to PRU

        int32_t header;                 // 4 bytes
        int32_t jointFreqCmd[JOINTS];   // 16 bytes
        uint8_t jointEnable;            // 1 byte
        uint16_t outputs;               // 2 bytes
        uint8_t rgb[6];                 // 6 bytes (3 bits per LED x 16 LEDs = 48 bits)
        uint16_t spindleSpeed;          // 2 bytes

        uint8_t microsteps[JOINTS];     // 4 bytes
        uint8_t rmsCurrent[JOINTS];     // 4 bytes

        uint8_t dummy[9];               // make up to same size as rxData_t
} txData_t;

typedef struct __attribute__((__packed__))
{
        // this data goes from PRU back to LinuxCNC

        int32_t header;                 // 4 bytes
        int32_t jointFeedback[JOINTS];  // 16 bytes
        int32_t jogcounts[4];           // 16 bytes
        uint16_t inputs;                // 2 bytes
        uint16_t adc[2];                // 4 bytes
        uint16_t pressure;              // 2 bytes
        int32_t loadcell;              // 4 bytes

} rxData_t;

typedef union
{
        rxData_t rx;
        txData_t tx;
} commPacket_t;

static rxData_t rxData;
static txData_t txData;


/* other globals */
static int 		comp_id;				// component ID
static const char 	*modname = MODNAME;
static const char 	*prefix = PREFIX;
static int 		num_chan = 0;			// number of step generators configured
static long 		old_dtns;				// update_freq function period in nsec - (THIS IS RUNNING IN THE PI)
static double		dt;						// update_freq period in seconds  - (THIS IS RUNNING IN THE PI)
static double 		recip_dt;				// recprocal of period, avoids divides

static int64_t 		accum[JOINTS] = { 0 };
static int32_t 		old_count[JOINTS] = { 0 };
static int32_t		accum_diff = 0;

typedef enum CONTROL { POSITION, VELOCITY, INVALID } CONTROL;

static int reset_gpio_pin = 25;				// RPI GPIO pin number used to force watchdog reset of the PRU 



/***********************************************************************
*                  LOCAL FUNCTION DECLARATIONS                         *
************************************************************************/
static int rt_bcm2835_init(void);

static void update_freq(void *arg, long period);
static void spi_write();
static void spi_read();
static void spi_transfer();
static CONTROL parse_ctrl_type(const char *ctrl);

/***********************************************************************
*                       INIT AND EXIT CODE                             *
************************************************************************/

int rtapi_app_main(void)
{
    char name[HAL_NAME_LEN + 1];
        int n, retval;

        for (n = 0; n < JOINTS; n++) {
                if(parse_ctrl_type(ctrl_type[n]) == INVALID) {
                        rtapi_print_msg(RTAPI_MSG_ERR,
                                        "STEPGEN: ERROR: bad control type '%s' for axis %i (must be 'p' or 'v')\n",
                                        ctrl_type[n], n);
                        return -1;
                }
    }

    // connect to the HAL, initialise the driver
    comp_id = hal_init(modname);
    if (comp_id < 0)
	{
		rtapi_print_msg(RTAPI_MSG_ERR, "%s ERROR: hal_init() failed \n", modname);
		return -1;
    }

	// allocate shared memory
	data = hal_malloc(sizeof(data_t));
	if (data == 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: ERROR: hal_malloc() failed\n", modname);
		hal_exit(comp_id);
		return -1;
	}

	// Map the RPi BCM2835 peripherals - uses "rtapi_open_as_root" in place of "open"
	if (!rt_bcm2835_init())
    {
      rtapi_print_msg(RTAPI_MSG_ERR,"rt_bcm2835_init failed. Are you running with root privlages??\n");
      return -1;
    }

	// Set the SPI0 pins to the Alt 0 function to enable SPI0 access, setup CS register
	// and clear TX and RX fifos
	if (!bcm2835_spi_begin())
    {
      rtapi_print_msg(RTAPI_MSG_ERR,"bcm2835_spi_begin failed. Are you running with root privlages??\n");
      return -1;
    }

	// Configure SPI0
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default

	//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128);		// 3.125MHz on RPI3
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);		// 6.250MHz on RPI3
	//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);		// 12.5MHz on RPI3

        bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
        bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default


	/* RPI_GPIO_P1_19        = 10 		MOSI when SPI0 in use
     * RPI_GPIO_P1_21        =  9 		MISO when SPI0 in use
     * RPI_GPIO_P1_23        = 11 		CLK when SPI0 in use
     * RPI_GPIO_P1_24        =  8 		CE0 when SPI0 in use
     * RPI_GPIO_P1_26        =  7 		CE1 when SPI0 in use
	 */

	// Configure pullups on SPI0 pins - source termination and CS high (does this allows for higher clock frequencies??? wiring is more important here)
	bcm2835_gpio_set_pud(RPI_GPIO_P1_19, BCM2835_GPIO_PUD_DOWN);	// MOSI
	bcm2835_gpio_set_pud(RPI_GPIO_P1_21, BCM2835_GPIO_PUD_DOWN);	// MISO
	bcm2835_gpio_set_pud(RPI_GPIO_P1_24, BCM2835_GPIO_PUD_UP);		// CS0

	// export spiPRU SPI enable and status bits
	retval = hal_pin_bit_newf(HAL_IN, &(data->SPIenable),
			comp_id, "%s.SPI-enable", prefix);
	if (retval != 0) goto error;
	
	retval = hal_pin_bit_newf(HAL_IN, &(data->SPIreset),
			comp_id, "%s.SPI-reset", prefix);
	if (retval != 0) goto error;

	retval = hal_pin_bit_newf(HAL_OUT, &(data->SPIstatus),
			comp_id, "%s.SPI-status", prefix);
	if (retval != 0) goto error;

	bcm2835_gpio_fsel(reset_gpio_pin, BCM2835_GPIO_FSEL_OUTP);
	retval = hal_pin_bit_newf(HAL_IN, &(data->PRUreset),
			comp_id, "%s.PRU-reset", prefix);
	if (retval != 0) goto error;


    // export all the variables for each joint
    for (n = 0; n < JOINTS; n++) {
		// export pins

		data->pos_mode[n] = (parse_ctrl_type(ctrl_type[n]) == POSITION);
/*
This is throwing errors from axis.py for some reason...
		
		if (data->pos_mode[n]){
			rtapi_print_msg(RTAPI_MSG_ERR, "Creating pos_mode[%d] = %d\n", n, data->pos_mode[n]);
			retval = hal_pin_float_newf(HAL_IN, &(data->pos_cmd[n]),
					comp_id, "%s.joint.%01d.pos-cmd", prefix, n);
			if (retval < 0) goto error;
			*(data->pos_cmd[n]) = 0.0;
		} else {
			rtapi_print_msg(RTAPI_MSG_ERR, "Creating vel_mode[%d] = %d\n", n, data->pos_mode[n]);
			retval = hal_pin_float_newf(HAL_IN, &(data->vel_cmd[n]),
					comp_id, "%s.joint.%01d.vel-cmd", prefix, n);
			if (retval < 0) goto error;
			*(data->vel_cmd[n]) = 0.0;			
		}
*/

		retval = hal_pin_bit_newf(HAL_IN, &(data->stepperEnable[n]),
				comp_id, "%s.joint.%01d.enable", prefix, n);
		if (retval != 0) goto error;

		retval = hal_pin_float_newf(HAL_IN, &(data->pos_cmd[n]),
				comp_id, "%s.joint.%01d.pos-cmd", prefix, n);
		if (retval < 0) goto error;
		*(data->pos_cmd[n]) = 0.0;
		
		if (data->pos_mode[n] == 0){
			retval = hal_pin_float_newf(HAL_IN, &(data->vel_cmd[n]),
					comp_id, "%s.joint.%01d.vel-cmd", prefix, n);
			if (retval < 0) goto error;
			*(data->vel_cmd[n]) = 0.0;			
		}

		retval = hal_pin_float_newf(HAL_OUT, &(data->freq_cmd[n]),
		        comp_id, "%s.joint.%01d.freq-cmd", prefix, n);
		if (retval < 0) goto error;
		*(data->freq_cmd[n]) = 0.0;

		retval = hal_pin_float_newf(HAL_OUT, &(data->pos_fb[n]),
		        comp_id, "%s.joint.%01d.pos-fb", prefix, n);
		if (retval < 0) goto error;
		*(data->pos_fb[n]) = 0.0;
		
		retval = hal_param_float_newf(HAL_RW, &(data->pos_scale[n]),
		        comp_id, "%s.joint.%01d.scale", prefix, n);
		if (retval < 0) goto error;
		data->pos_scale[n] = 1.0;

        // retval = hal_pin_s32_newf(HAL_OUT, &(data->count[n]),
        //         comp_id, "%s.joint.%01d.counts", prefix, n);
        // if (retval < 0) goto error;
        // *(data->count[n]) = 0;
		
		retval = hal_pin_float_newf(HAL_IN, &(data->pgain[n]),
				comp_id, "%s.joint.%01d.pgain", prefix, n);
		if (retval < 0) goto error;
		*(data->pgain[n]) = 0.0;
		
		retval = hal_pin_float_newf(HAL_IN, &(data->ff1gain[n]),
				comp_id, "%s.joint.%01d.ff1gain", prefix, n);
		if (retval < 0) goto error;
		*(data->ff1gain[n]) = 0.0;
		
		retval = hal_pin_float_newf(HAL_IN, &(data->deadband[n]),
				comp_id, "%s.joint.%01d.deadband", prefix, n);
		if (retval < 0) goto error;
		*(data->deadband[n]) = 0.0;
		
		retval = hal_param_float_newf(HAL_RW, &(data->maxaccel[n]),
		        comp_id, "%s.joint.%01d.maxaccel", prefix, n);
		if (retval < 0) goto error;
                data->maxaccel[n] = 1.0;

//                retval = hal_pin_s32_newf(HAL_OUT, &(data->jogcounts[n]), comp_id, "%s.joint.%01d.jogcounts", prefix, n);
//                if (retval < 0) goto error;
//                *(data->jogcounts[n]) = 0;
        }

        retval = hal_pin_s32_newf(HAL_OUT, &(data->jogcounts[0]), comp_id, "%s.analog0.jogcounts", prefix);
        if (retval < 0) goto error;
        retval = hal_pin_s32_newf(HAL_OUT, &(data->jogcounts[1]), comp_id, "%s.analog1.jogcounts", prefix);
        if (retval < 0) goto error;
        retval = hal_pin_s32_newf(HAL_OUT, &(data->jogcounts[2]), comp_id, "%s.encoder.jogcounts", prefix);
        if (retval < 0) goto error;
//        retval = hal_pin_s32_newf(HAL_OUT, &(data->jogcounts[3]), comp_id, "%s.buttons.jogcounts", prefix);
//        if (retval < 0) goto error;

        for (n = 0; n < 3; n++) {
            *(data->jogcounts[n]) = 0;
        }


        for (n = 0; n < 2; n++) {
            retval = hal_pin_float_newf(HAL_OUT, &(data->adc[n]), comp_id, "%s.analog%01d.raw", prefix, n);
            if (retval < 0) goto error;
            *(data->adc[n]) = 0;
        }

        for (n = 0; n < JOINTS; n++) {
            retval = hal_pin_s32_newf(HAL_IN, &(data->microsteps[n]), comp_id, "%s.tmc.%01d.microsteps", prefix, n);
            if (retval < 0) goto error;
            *(data->microsteps[n]) = 8;
        }

        for (n = 0; n < JOINTS; n++) {
            retval = hal_pin_s32_newf(HAL_IN, &(data->rmsCurrent[n]), comp_id, "%s.tmc.%01d.current", prefix, n);
            if (retval < 0) goto error;
            *(data->rmsCurrent[n]) = 200;
        }

        retval = hal_pin_float_newf(HAL_OUT, &(data->pressure), comp_id, "%s.pressure.%01d", prefix, 0);
        if (retval < 0) goto error;
        *(data->pressure) = 200;

        retval = hal_pin_s32_newf(HAL_OUT, &(data->loadcell), comp_id, "%s.loadcell.%01d", prefix, 0);
        if (retval < 0) goto error;
        *(data->loadcell) = 0;

//	for (n = 0; n < VARIABLES; n++) {
//	// export pins

//		retval = hal_pin_float_newf(HAL_IN, &(data->setPoint[n]),
//		        comp_id, "%s.SP.%01d", prefix, n);
//		if (retval < 0) goto error;
//		*(data->setPoint[n]) = 0.0;

//                retval = hal_pin_float_newf(HAL_OUT, &(data->processVariable[n]),
//                        comp_id, "%s.jog.%01d", prefix, n);
//                if (retval < 0) goto error;
//                *(data->processVariable[n]) = 0.0;
//	}

	for (n = 0; n < DIGITAL_OUTPUTS; n++) {
		retval = hal_pin_bit_newf(HAL_IN, &(data->outputs[n]),
                                comp_id, "%s.output.%02d", prefix, n);
		if (retval != 0) goto error;
		*(data->outputs[n])=0;
        }

        for (n = 0; n < DIGITAL_INPUTS; n++) {
                retval = hal_pin_bit_newf(HAL_OUT, &(data->inputs[n]),
                                comp_id, "%s.input.%02d", prefix, n);
                if (retval != 0) goto error;
                *(data->inputs[n])=0;
        }

        char* colors[] = {
            "red",
            "green",
            "blue"
        };

        for (n = 0; n < RGB_BITS; n++) {
            int ledPos = n / 3;
            int color = n % 3;
                retval = hal_pin_bit_newf(HAL_IN, &(data->rgb[n]),
                                comp_id, "%s.rgb.led%02d.%s", prefix, ledPos, colors[color]);
                if (retval != 0) goto error;
                *(data->rgb[n])=0;
        }

        retval = hal_pin_float_newf(HAL_IN, &(data->spindleSpeed[0]),
                comp_id, "%s.spindle.set-speed", prefix);
        if (retval < 0) goto error;
        *(data->spindleSpeed[0]) = 0.0;

	error:
	if (retval < 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
		        "%s: ERROR: pin export failed with err=%i\n",
		        modname, retval);
		hal_exit(comp_id);
		return -1;
	}





	// Export functions
	rtapi_snprintf(name, sizeof(name), "%s.update-freq", prefix);
	retval = hal_export_funct(name, update_freq, data, 1, 0, comp_id);
	if (retval < 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
		        "%s: ERROR: update function export failed\n", modname);
		hal_exit(comp_id);
		return -1;
	}

	rtapi_snprintf(name, sizeof(name), "%s.write", prefix);
	/* no FP operations */
	retval = hal_export_funct(name, spi_write, 0, 0, 0, comp_id);
	if (retval < 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
		        "%s: ERROR: write function export failed\n", modname);
		hal_exit(comp_id);
		return -1;
	}

	rtapi_snprintf(name, sizeof(name), "%s.read", prefix);
	retval = hal_export_funct(name, spi_read, data, 1, 0, comp_id);
	if (retval < 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
		        "%s: ERROR: read function export failed\n", modname);
		hal_exit(comp_id);
		return -1;
	}

	rtapi_print_msg(RTAPI_MSG_INFO, "%s: installed driver\n", modname);
	hal_ready(comp_id);
    return 0;
}

void rtapi_app_exit(void)
{
    hal_exit(comp_id);
}


/***********************************************************************
*                   LOCAL FUNCTION DEFINITIONS                         *
************************************************************************/


// This is the same as the standard bcm2835 library except for the use of
// "rtapi_open_as_root" in place of "open"

int rt_bcm2835_init(void)
{
    int  memfd;
    int  ok;
    FILE *fp;

    if (debug) 
    {
        bcm2835_peripherals = (uint32_t*)BCM2835_PERI_BASE;

	bcm2835_pads = bcm2835_peripherals + BCM2835_GPIO_PADS/4;
	bcm2835_clk  = bcm2835_peripherals + BCM2835_CLOCK_BASE/4;
	bcm2835_gpio = bcm2835_peripherals + BCM2835_GPIO_BASE/4;
	bcm2835_pwm  = bcm2835_peripherals + BCM2835_GPIO_PWM/4;
	bcm2835_spi0 = bcm2835_peripherals + BCM2835_SPI0_BASE/4;
	bcm2835_bsc0 = bcm2835_peripherals + BCM2835_BSC0_BASE/4;
	bcm2835_bsc1 = bcm2835_peripherals + BCM2835_BSC1_BASE/4;
	bcm2835_st   = bcm2835_peripherals + BCM2835_ST_BASE/4;
	bcm2835_aux  = bcm2835_peripherals + BCM2835_AUX_BASE/4;
	bcm2835_spi1 = bcm2835_peripherals + BCM2835_SPI1_BASE/4;

	return 1; /* Success */
    }

    /* Figure out the base and size of the peripheral address block
    // using the device-tree. Required for RPi2/3/4, optional for RPi 1
    */
    if ((fp = fopen(BMC2835_RPI2_DT_FILENAME , "rb")))
    {
        unsigned char buf[16];
        uint32_t base_address;
        uint32_t peri_size;
        if (fread(buf, 1, sizeof(buf), fp) >= 8)
        {
            base_address = (buf[4] << 24) |
              (buf[5] << 16) |
              (buf[6] << 8) |
              (buf[7] << 0);
            
            peri_size = (buf[8] << 24) |
              (buf[9] << 16) |
              (buf[10] << 8) |
              (buf[11] << 0);
            
            if (!base_address)
            {
                /* looks like RPI 4 */
                base_address = (buf[8] << 24) |
                      (buf[9] << 16) |
                      (buf[10] << 8) |
                      (buf[11] << 0);
                      
                peri_size = (buf[12] << 24) |
                (buf[13] << 16) |
                (buf[14] << 8) |
                (buf[15] << 0);
            }
            /* check for valid known range formats */
            if ((buf[0] == 0x7e) &&
                    (buf[1] == 0x00) &&
                    (buf[2] == 0x00) &&
                    (buf[3] == 0x00) &&
                    ((base_address == BCM2835_PERI_BASE) || (base_address == BCM2835_RPI2_PERI_BASE) || (base_address == BCM2835_RPI4_PERI_BASE)))
            {
                bcm2835_peripherals_base = (off_t)base_address;
                bcm2835_peripherals_size = (size_t)peri_size;
                if( base_address == BCM2835_RPI4_PERI_BASE )
                {
                    pud_type_rpi4 = 1;
                }
            }
        
        }
        
	fclose(fp);
    }
    /* else we are prob on RPi 1 with BCM2835, and use the hardwired defaults */

    /* Now get ready to map the peripherals block 
     * If we are not root, try for the new /dev/gpiomem interface and accept
     * the fact that we can only access GPIO
     * else try for the /dev/mem interface and get access to everything
     */
    memfd = -1;
    ok = 0;
    if (geteuid() == 0)
    {
      /* Open the master /dev/mem device */
      if ((memfd = rtapi_open_as_root("/dev/mem", O_RDWR | O_SYNC) ) < 0) 
	{
	  fprintf(stderr, "bcm2835_init: Unable to open /dev/mem: %s\n",
		  strerror(errno)) ;
	  goto exit;
	}
      
      /* Base of the peripherals block is mapped to VM */
      bcm2835_peripherals = mapmem("gpio", bcm2835_peripherals_size, memfd, bcm2835_peripherals_base);
      if (bcm2835_peripherals == MAP_FAILED) goto exit;
      
      /* Now compute the base addresses of various peripherals, 
      // which are at fixed offsets within the mapped peripherals block
      // Caution: bcm2835_peripherals is uint32_t*, so divide offsets by 4
      */
      bcm2835_gpio = bcm2835_peripherals + BCM2835_GPIO_BASE/4;
      bcm2835_pwm  = bcm2835_peripherals + BCM2835_GPIO_PWM/4;
      bcm2835_clk  = bcm2835_peripherals + BCM2835_CLOCK_BASE/4;
      bcm2835_pads = bcm2835_peripherals + BCM2835_GPIO_PADS/4;
      bcm2835_spi0 = bcm2835_peripherals + BCM2835_SPI0_BASE/4;
      bcm2835_bsc0 = bcm2835_peripherals + BCM2835_BSC0_BASE/4; /* I2C */
      bcm2835_bsc1 = bcm2835_peripherals + BCM2835_BSC1_BASE/4; /* I2C */
      bcm2835_st   = bcm2835_peripherals + BCM2835_ST_BASE/4;
      bcm2835_aux  = bcm2835_peripherals + BCM2835_AUX_BASE/4;
      bcm2835_spi1 = bcm2835_peripherals + BCM2835_SPI1_BASE/4;

      ok = 1;
    }
    else
    {
      /* Not root, try /dev/gpiomem */
      /* Open the master /dev/mem device */
      if ((memfd = open("/dev/gpiomem", O_RDWR | O_SYNC) ) < 0) 
	{
	  fprintf(stderr, "bcm2835_init: Unable to open /dev/gpiomem: %s\n",
		  strerror(errno)) ;
	  goto exit;
	}
      
      /* Base of the peripherals block is mapped to VM */
      bcm2835_peripherals_base = 0;
      bcm2835_peripherals = mapmem("gpio", bcm2835_peripherals_size, memfd, bcm2835_peripherals_base);
      if (bcm2835_peripherals == MAP_FAILED) goto exit;
      bcm2835_gpio = bcm2835_peripherals;
      ok = 1;
    }

exit:
    if (memfd >= 0)
        close(memfd);

    if (!ok)
	bcm2835_close();

    return ok;
}

void update_freq(void *arg, long period)
{
	int i;
	data_t *data = (data_t *)arg;
	double max_ac, vel_cmd, dv, new_vel, max_freq, desired_freq;
		   
	double error, command, feedback;
	double periodfp, periodrecip;
	float pgain, ff1gain, deadband;

	// precalculate timing constants
    periodfp = period * 0.000000001;
    periodrecip = 1.0 / periodfp;

    // calc constants related to the period of this function (LinuxCNC SERVO_THREAD)
    // only recalc constants if period changes
    if (period != old_dtns) 			// Note!! period = LinuxCNC SERVO_PERIOD
	{
		old_dtns = period;				// get ready to detect future period changes
		dt = period * 0.000000001; 		// dt is the period of this thread, used for the position loop
		recip_dt = 1.0 / dt;			// calc the reciprocal once here, to avoid multiple divides later
    }

    // loop through generators
	for (i = 0; i < JOINTS; i++)
	{
		// check for scale change
		if (data->pos_scale[i] != data->old_scale[i])
		{
			data->old_scale[i] = data->pos_scale[i];		// get ready to detect future scale changes
			// scale must not be 0
			if ((data->pos_scale[i] < 1e-20) && (data->pos_scale[i] > -1e-20))	// validate the new scale value
				data->pos_scale[i] = 1.0;										// value too small, divide by zero is a bad thing
				// we will need the reciprocal, and the accum is fixed point with
				//fractional bits, so we precalc some stuff
                        //data->scale_recip[i] = (1.0 / STEP_MASK) / data->pos_scale[i];
		}

		// calculate frequency limit
		//max_freq = PRU_BASEFREQ/(4.0); 			//limit of DDS running at 80kHz
                max_freq = PRU_BASEFREQ /(2.0);


		// check for user specified frequency limit parameter
		if (data->maxvel[i] <= 0.0)
		{
			// set to zero if negative
			data->maxvel[i] = 0.0;
		}
		else
		{
			// parameter is non-zero, compare to max_freq
			desired_freq = data->maxvel[i] * fabs(data->pos_scale[i]);

			if (desired_freq > max_freq)
			{
				// parameter is too high, limit it
				data->maxvel[i] = max_freq / fabs(data->pos_scale[i]);
			}
			else
			{
				// lower max_freq to match parameter
				max_freq = data->maxvel[i] * fabs(data->pos_scale[i]);
			}
		}
		
		/* set internal accel limit to its absolute max, which is
		zero to full speed in one thread period */
		max_ac = max_freq * recip_dt;
		
		// check for user specified accel limit parameter
		if (data->maxaccel[i] <= 0.0)
		{
			// set to zero if negative
			data->maxaccel[i] = 0.0;
		}
		else 
		{
			// parameter is non-zero, compare to max_ac
			if ((data->maxaccel[i] * fabs(data->pos_scale[i])) > max_ac)
			{
				// parameter is too high, lower it
				data->maxaccel[i] = max_ac / fabs(data->pos_scale[i]);
			}
			else
			{
				// lower limit to match parameter
				max_ac = data->maxaccel[i] * fabs(data->pos_scale[i]);
			}
		}

		/* at this point, all scaling, limits, and other parameter
		changes have been handled - time for the main control */

		

		if (data->pos_mode[i]) {

			/* POSITION CONTROL MODE */

			// use Proportional control with feed forward (pgain, ff1gain and deadband)
			
			if (*(data->pgain[i]) != 0)
			{
				pgain = *(data->pgain[i]);
			}
			else
			{
				pgain = 1.0;
			}
			
			if (*(data->ff1gain[i]) != 0)
			{
				ff1gain = *(data->ff1gain[i]);
			}
			else
			{
				ff1gain = 1.0;
			}
			
			if (*(data->deadband[i]) != 0)
			{
				deadband = *(data->deadband[i]);
			}
			else
			{
				deadband = 1 / data->pos_scale[i];
			}	

			// read the command and feedback
			command = *(data->pos_cmd[i]);
			feedback = *(data->pos_fb[i]);
			
			// calcuate the error
			error = command - feedback;
			
			// apply the deadband
			if (error > deadband)
			{
				error -= deadband;
			}
			else if (error < -deadband)
			{
				error += deadband;
			}
			else
			{
				error = 0;
			}
			
			// calcuate command and derivatives
                        //data->cmd_d[i] = (command - data->prev_cmd[i]) * periodrecip;
                        float cmd_d = (command - data->prev_cmd[i]) * periodrecip;
			
			// save old values
			data->prev_cmd[i] = command;
				
			// calculate the output value
                        vel_cmd = pgain * error + cmd_d * ff1gain;
		
		} else {

			/* VELOCITY CONTROL MODE */
			
			// calculate velocity command in counts/sec
			vel_cmd = *(data->vel_cmd[i]);
		}	
			
		vel_cmd = vel_cmd * data->pos_scale[i];
			
		// apply frequency limit
		if (vel_cmd > max_freq) 
		{
			vel_cmd = max_freq;
		} 
		else if (vel_cmd < -max_freq) 
		{
			vel_cmd = -max_freq;
		}
		
		// calc max change in frequency in one period
		dv = max_ac * dt;
		
		// apply accel limit
		if ( vel_cmd > (data->freq[i] + dv) )
		{
			new_vel = data->freq[i] + dv;
		} 
		else if ( vel_cmd < (data->freq[i] - dv) ) 
		{
			new_vel = data->freq[i] - dv;
		}
		else
		{
			new_vel = vel_cmd;
		}
		
		// test for disabled stepgen
		if (*data->stepperEnable == 0) {
			// set velocity to zero
			new_vel = 0; 
		}
		
		data->freq[i] = new_vel;				// to be sent to the PRU
		*(data->freq_cmd[i]) = data->freq[i];	// feedback to LinuxCNC
	}

}


float lastVac = -123;

void spi_read()
{
	int i;
	double curr_pos;

        static int readCount = 0;

	// Daxfonlmta header
        if ( txData.header != PRU_WRITE )
            txData.header = PRU_READ;
	
	// update the PRUreset output
	if (*(data->PRUreset))
	{ 
		bcm2835_gpio_set(reset_gpio_pin);
    }
	else
	{
		bcm2835_gpio_clr(reset_gpio_pin);
    }
	
	if (*(data->SPIenable))
	{
		if( (*(data->SPIreset) && !(data->SPIresetOld)) || *(data->SPIstatus) )
		{
			// reset rising edge detected, try SPI transfer and reset OR PRU running
			
                        // Transfer to and from the PRU
			spi_transfer();

                        switch (rxData.header)		// only process valid SPI payloads. This rejects bad payloads
			{
				case PRU_DATA:
                                        // we have received a GOOD payload from the PRU
					*(data->SPIstatus) = 1;

					for (i = 0; i < JOINTS; i++)
					{
                        // the PRU DDS accumulator uses 32 bit counter, this code converts that counter into 64 bits
                        //accum_diff = rxData.jointFeedback[i] - old_count[i];
                        //old_count[i] = rxData.jointFeedback[i];
                        //accum[i] += accum_diff;

                        //*(data->count[i]) = accum[i] >> STEPBIT;

                        //data->scale_recip[i] = (1.0 / STEP_MASK) / data->pos_scale[i];
                        //curr_pos = (double)(accum[i]-STEP_OFFSET) * (1.0 / STEP_MASK);
                        //*(data->pos_fb[i]) = (float)((curr_pos+0.5) / data->pos_scale[i]);

                        *(data->pos_fb[i]) = (float)(rxData.jointFeedback[i]) / data->pos_scale[i];
                    }
					
					for (i = 0; i < 3; i++)
						*(data->jogcounts[i]) = rxData.jogcounts[i];

                                        for (i = 0; i < 2; i++)
                                                *(data->adc[i]) = rxData.adc[i] / 4096.0f;

					// Feedback
//                                        for (i = 0; i < VARIABLES; i++)
//                                        {
//                                                *(data->processVariable[i]) = rxData.jog[i];
//                                        }

                                        // Inputs
                                        for (i = 0; i < DIGITAL_INPUTS; i++)
                                        {
                                                if ((rxData.inputs & (1 << i)) != 0)
                                                {
                                                        *(data->inputs[i]) = 1; 		// input is high
                                                }
                                                else
                                                {
                                                        *(data->inputs[i]) = 0;			// input is low
                                                }
                                        }

                                        float vac = ((float)rxData.pressure - 50000) / 500.0f;
                                        *(data->pressure) = vac;
//                                        if ( vac != lastVac ) {
//                                            rtapi_print("pressure = %d (%f)\n", rxData.pressure, vac);
//                                            lastVac = vac;
//                                        }

                                        *(data->loadcell) = rxData.loadcell;
                                        //rtapi_print("loadcell = %ld\n", rxData.loadcell);

					break;
					
				case PRU_ESTOP:
					// we have an eStop notification from the PRU
                                        *(data->SPIstatus) = 0;
					 rtapi_print_msg(RTAPI_MSG_ERR, "An E-stop is active");

				default:
					// we have received a BAD payload from the PRU
					*(data->SPIstatus) = 0;
					rtapi_print("Bad SPI payload = %x\n", rxData.header);
                                        break;
                        }

//                        if ( readCount++ % 1000 == 0 ) {
//                            rtapi_print("jogcounts = %ld, %ld\n", rxData.jogcounts[0], rxData.jogcounts[1]);
//                        }
		}
	}
	else
	{
		*(data->SPIstatus) = 0;
	}
	
	data->SPIresetOld = *(data->SPIreset);
}


void spi_write()
{
	int i;

	// Data header
        txData.header = PRU_WRITE;

	// Joint frequency commands
	for (i = 0; i < JOINTS; i++)
	{
		txData.jointFreqCmd[i] = data->freq[i];
	}

	for (i = 0; i < JOINTS; i++)
	{
		if (*(data->stepperEnable[i]) == 1)
		{
			txData.jointEnable |= (1 << i);		
		}
		else
		{
			txData.jointEnable &= ~(1 << i);	
		}
	}

        for (i = 0; i < JOINTS; i++)
        {
            uint8_t mstepsCode = 0xFF; // signifies no change, only allow values that make sense

            switch( *(data->microsteps[i]) ) {
              case 256: mstepsCode = 0; break;
              case 128: mstepsCode = 1; break;
              case  64: mstepsCode = 2; break;
              case  32: mstepsCode = 3; break;
              case  16: mstepsCode = 4; break;
              case   8: mstepsCode = 5; break;
              case   4: mstepsCode = 6; break;
              case   2: mstepsCode = 7; break;
              case   0: mstepsCode = 8; break;
              default: break;
            }

            txData.microsteps[i] = mstepsCode;


            int32_t c = *(data->rmsCurrent[i]);
            if ( c < 0 )
                c = 0;
            else if ( c > 2000 )
                c = 2000;
            c /= 10; // to fit into one byte
            txData.rmsCurrent[i] = (uint8_t)c;
        }

	// Set points
//	for (i = 0; i < VARIABLES; i++)
//	{
//		txData.setPoint[i] = *(data->setPoint[i]);
//	}

        // Outputs
        for (i = 0; i < DIGITAL_OUTPUTS; i++)
        {
                if (*(data->outputs[i]) == 1)
                {
                        txData.outputs |= (1 << i);		// output is high
                }
                else
                {
                        txData.outputs &= ~(1 << i);	// output is low
                }
        }

        for (i = 0; i < RGB_BITS; i++)
        {
            int bytePos = i / 8;
            int bitPos = i % 8;
            if (*(data->rgb[i]) == 1)
            {
                txData.rgb[bytePos] |= (1 << bitPos);
            }
            else
            {
                txData.rgb[bytePos] &= ~(1 << bitPos);
            }
        }


        float ssf = *(data->spindleSpeed[0]);
        if ( ssf < 0 )
            txData.spindleSpeed = 0;
        else if ( ssf > 65535 )
            txData.spindleSpeed = 65535;
        else
            txData.spindleSpeed = (uint16_t)ssf;


	if( *(data->SPIstatus) )
	{
		// Transfer to and from the PRU
          //rtapi_print("not Doing spi_write\n");
	  //spi_transfer();
	}

}


void spi_transfer()
{
        // send and receive data to and from the weeny PRU concurrently
        bcm2835_spi_transfernb( (char*)&txData, (char*)&rxData, sizeof(commPacket_t) );

}

static CONTROL parse_ctrl_type(const char *ctrl)
{
    if(!ctrl || !*ctrl || *ctrl == 'p' || *ctrl == 'P') return POSITION;
    if(*ctrl == 'v' || *ctrl == 'V') return VELOCITY;
    return INVALID;
}
