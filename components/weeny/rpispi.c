
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

//#include <atomic>
//#include <cerrno>

#ifdef __linux__
#include <sys/fsuid.h>
#endif

#include "bcm2835.h"

#include "rp1lib.c"
#include "gpiochip_rp1.c"
#include "spi-dw.c"

#define RPI5_RP1_PERI_BASE 0x7c000000

bool isRP1 = false; // Pi5 uses RP1 hardware, otherwise assume Pi3 or Pi4 with bcm hardware

int SPI_num = -1;
int CS_num = -1;
int32_t SPI_freq = -1;

#define DTC_BOARD_MAKE_RPI		"raspberrypi"

#define DTC_BOARD_RPI_5CM		"5-compute-module"
#define DTC_BOARD_RPI_5B		"5-model-b"
#define DTC_BOARD_RPI_4CM		"4-compute-module"
#define DTC_BOARD_RPI_4B		"4-model-b"
#define DTC_BOARD_RPI_400		"400"
#define DTC_BOARD_RPI_3CM		"3-compute-module"
#define DTC_BOARD_RPI_3BP		"3-model-b-plus"
#define DTC_BOARD_RPI_3AP		"3-model-a-plus"
#define DTC_BOARD_RPI_3B		"3-model-b"
#define DTC_BOARD_RPI_2B		"2-model-b"
#define DTC_BOARD_RPI_CM		"compute-module"
#define DTC_BOARD_RPI_BP		"model-b-plus"
#define DTC_BOARD_RPI_AP		"model-a-plus"
#define DTC_BOARD_RPI_BR2		"model-b-rev2"
#define DTC_BOARD_RPI_B			"model-b"
#define DTC_BOARD_RPI_A			"model-a"
#define DTC_BOARD_RPI_ZERO_2W	"model-zero-2-w"
#define DTC_BOARD_RPI_ZERO_W	"model-zero-w"
#define DTC_BOARD_RPI_ZERO		"model-zero"

#define DTC_SOC_MAKE_BRCM		"brcm"

#define DTC_SOC_MODEL_BCM2712	"bcm2712"
#define DTC_SOC_MODEL_BCM2711	"bcm2711"
#define DTC_SOC_MODEL_BCM2837	"bcm2837"
#define DTC_SOC_MODEL_BCM2836	"bcm2836"
#define DTC_SOC_MODEL_BCM2835	"bcm2835"

/* The device-tree compatible strings for the boards */
#define DTC_RPI_SOC_BCM2712		DTC_SOC_MAKE_RPI "," DTC_SOC_MODEL_BCM2712
#define DTC_RPI_MODEL_5CM		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_5CM
#define DTC_RPI_MODEL_5B		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_5B

#define DTC_RPI_SOC_BCM2711		DTC_SOC_MAKE_RPI "," DTC_SOC_MODEL_BCM2711
#define DTC_RPI_MODEL_4CM		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_4CM
#define DTC_RPI_MODEL_4B		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_4B
#define DTC_RPI_MODEL_400		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_400

#define DTC_RPI_SOC_BCM2837		DTC_SOC_MAKE_BRCM "," DTC_SOC_MODEL_BCM2837
#define DTC_RPI_MODEL_3CM		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_3CM
#define DTC_RPI_MODEL_3BP		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_3BP
#define DTC_RPI_MODEL_3AP		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_3AP
#define DTC_RPI_MODEL_3B		DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_3B
#define DTC_RPI_MODEL_ZERO_2W	DTC_BOARD_MAKE_RPI "," DTC_BOARD_RPI_ZERO_2W

bool checkPiType() {

    FILE *fp;
    int i, j;
    char buf[256];
    ssize_t buflen;
    char* cptr;
    const int DTC_MAX = 8;
    const char* dtcs[DTC_MAX + 1];

    if ((fp = fopen("/proc/device-tree/compatible" , "rb"))){

        // Read the 'compatible' string-list from the device-tree
        buflen = fread(buf, 1, sizeof(buf), fp);
        if(buflen < 0) {
            rtapi_print_msg(RTAPI_MSG_ERR, "Failed to read platform identity.\n");
            return false;
        }

        // Decompose the device-tree buffer into a string-list with the pointers to
        // each string in dtcs. Don't go beyond the buffer's size.
        memset(dtcs, 0, sizeof(dtcs));
        for(i = 0, cptr = buf; i < DTC_MAX && cptr; i++) {
            dtcs[i] = cptr;
            j = strlen(cptr);
            if((cptr - buf) + j + 1 < buflen)
                cptr += j + 1;
            else
                cptr = NULL;
        }

        fclose(fp);

        for(i = 0; dtcs[i] != NULL; i++) {
            if(        !strcmp(dtcs[i], DTC_RPI_MODEL_4B)
                ||    !strcmp(dtcs[i], DTC_RPI_MODEL_4CM)
                ||    !strcmp(dtcs[i], DTC_RPI_MODEL_400)
                ||    !strcmp(dtcs[i], DTC_RPI_MODEL_3BP)
                ||    !strcmp(dtcs[i], DTC_RPI_MODEL_3AP)
                ||    !strcmp(dtcs[i], DTC_RPI_MODEL_3B)) {
                rtapi_print_msg(RTAPI_MSG_INFO, "Raspberry Pi 3 or 4, using BCM2835 driver\n");
                return true;
            } else if(!strcmp(dtcs[i], DTC_RPI_MODEL_5B) || !strcmp(dtcs[i], DTC_RPI_MODEL_5CM)) {
                rtapi_print_msg(RTAPI_MSG_INFO, "Raspberry Pi 5, using RP1 driver\n");
                isRP1 = true;
                return true;
            } else {
                rtapi_print_msg(RTAPI_MSG_ERR, "Error, RPi not detected\n");
                return false;
            }
        }

    } else {
        rtapi_print_msg(RTAPI_MSG_ERR, "Cannot open '/proc/device-tree/compatible' for read.\n");
        return false;
    }

    return false;
}

// This is the same as the standard bcm2835 library except for the use of
// "rtapi_open_as_root" in place of "open"

bool rt_bcm2835_init()
{
    FILE *fp;

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
                    //pud_type_rpi4 = 1;
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
    int memfd = 0;
    bool ok = false;
    if (geteuid() == 0)
    {
        /* Open the master /dev/mem device */
        int memfd = rtapi_open_as_root("/dev/mem", O_RDWR | O_SYNC);
        if (memfd < 0) {
            rtapi_print_msg(RTAPI_MSG_ERR, "bcm2835_init: Unable to open /dev/mem: %s", strerror(errno)) ;
            goto exit;
        }

        /* Base of the peripherals block is mapped to VM */
        bcm2835_peripherals = (uint32_t*)mapmem("gpio", bcm2835_peripherals_size, memfd, bcm2835_peripherals_base);
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

        ok = true;
    }

exit:
    if (memfd >= 0)
        close(memfd);

    if ( ! ok )
        bcm2835_close();

    return ok;
}

bool bcm2835_setupSPI() {

    // Set the SPI0 pins to the Alt 0 function to enable SPI0 access, setup CS register
    // and clear TX and RX fifos
    if (!bcm2835_spi_begin())
    {
        rtapi_print_msg(RTAPI_MSG_ERR, "bcm2835_spi_begin failed. Are you running with root privlages??");
        return false;
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
    bcm2835_gpio_set_pud(RPI_GPIO_P1_24, BCM2835_GPIO_PUD_UP);

    return true;

}




bool rt_rp1lib_init(void)
{
    uint64_t phys_addr = RP1_BAR1;

    DEBUG_PRINT("Initialising RP1 library: %s\n", __func__);

    // rp1_chip is declared in gpiochip_rp1.c
    chip = &rp1_chip;

    inst = rp1_create_instance(chip, phys_addr, NULL);
    if (!inst) {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: rp1_create_instance failed!!", __func__);
        return false;
    }

    inst->phys_addr = phys_addr;

    // map memory
    inst->mem_fd = rtapi_open_as_root("/dev/mem", O_RDWR | O_SYNC);
    DEBUG_PRINT("Initialising RP1 library inst->mem_fd: %d\n", inst->mem_fd);
    if (inst->mem_fd < 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: open failed!!", __func__);
        return false;
    }

    inst->priv = mmap(
        NULL,
        RP1_BAR1_LEN,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        inst->mem_fd,
        inst->phys_addr
        );

    DEBUG_PRINT("Base address: %11lx, size: %x, mapped at address: %p", inst->phys_addr, RP1_BAR1_LEN, inst->priv);

    if (inst->priv == MAP_FAILED) {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: mmap failed!!", __func__);
        return false;
    }

    return true;
}

bool rpispi_init() {

    if ( isRP1 ) {

        if ( ! rt_rp1lib_init() )
        {
            rtapi_print_msg(RTAPI_MSG_ERR, "rt_rp1lib_init failed. Are you running as root?");
            return false;
        }

        if (SPI_num == -1) SPI_num = 0; // default to SPI0
        if (CS_num == -1) CS_num = 0; // default to CS0
        if (SPI_freq == -1) SPI_freq = 6250000; // default to 6MHz

        if ( ! rp1spi_init(SPI_num, CS_num, SPI_MODE_0, SPI_freq) )  // SPIx, CSx, mode, freq
        {
            rtapi_print_msg(RTAPI_MSG_ERR, "rp1spi_init failed.");
            return false;
        }

        return true;
    }
    else {
        if (!rt_bcm2835_init())
        {
            rtapi_print_msg(RTAPI_MSG_ERR, "rt_bcm2835_init failed. Are you running as root?");
            return false;
        }

        if ( ! bcm2835_setupSPI() ) {
            rtapi_print_msg(RTAPI_MSG_ERR, "bcm2835_setupSPI failed.");
            return false;
        }

        return true;
    }

}

void rpispi_transfernb(char* tbuf, char* rbuf, uint32_t len) {
    if ( isRP1 ) {
        rp1spi_transfer(0, tbuf, rbuf, len);
    }
    else {
        bcm2835_spi_transfernb(tbuf, rbuf, len);
    }
}

bool rpispi_close() {
    if ( isRP1 ) {
        return (rp1lib_deinit() == 0);
    }
    else {
        return bcm2835_close();
    }
}







