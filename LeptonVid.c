#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <wiringPi.h>

#include "leptonSDKEmb32OEM/LEPTON_SDK.h"
#include "leptonSDKEmb32OEM/LEPTON_SYS.h"
#include "leptonSDKEmb32OEM/LEPTON_Types.h"
#include "leptonSDKEmb32OEM/LEPTON_AGC.h"
#include "leptonSDKEmb32OEM/LEPTON_OEM.h"
#include "leptonSDKEmb32OEM/LEPTON_ErrorCodes.h"

#define REBOOT_LEPTON
//#define POWER_CYCLE_LEPTON
//#define _DEBUG

#define SLOW_ALG    	    1
#define FAST_ALG    	    2
#define VSYNC_ALG   	    3
#define GPIO_PIN	        6

#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))
#define DEF_START_TIME 	    5
#define DEF_VSYNC_DELAY	    10
#define DEF_SYNC_DELAY 	    5000
#define DEF_FRAME_DELAY     65535
#define DEF_SPEED 		    16000000
#define DEF_PORT 		    "/dev/spidev0.0"
#define DEF_GPIO_PIN        6

#define LEPTON_WIDTH 	    160
#define LEPTON_HEIGHT 	    120
#define VOSPI_FRAME_SIZE 	(164)
#define LEP_SPI_BUFFER 		(118080) //(118080)39360

#define NUMBER_OF_SEGMENTS      4
#define PACKETS_PER_SEGMENT     60
#define PACKET_SIZE             164
#define PACKET_SIZE_UINT16      (PACKET_SIZE/2)
#define PACKETS_PER_FRAME       (PACKETS_PER_SEGMENT*NUMBER_OF_SEGMENTS)
#define FRAME_SIZE_UINT16       (PACKET_SIZE_UINT16*PACKETS_PER_FRAME)

#define OPTIONS                 "D:s:p:f:F:?irw:"
#define USAGE                   "[-f frames] [-F frames timeout] [-D startup delay] [-p port] [-s bitrate] [-i strip frame delimiters] [-r reset on startup] [-w gpio pin]"

static char *device = DEF_PORT;
static uint8_t mode = SPI_CPOL | SPI_CPHA;
static uint8_t bits = 8;
static uint32_t speed = DEF_SPEED;
static uint16_t frame_delay = DEF_FRAME_DELAY;
static uint8_t status_bits = 0;
static uint8_t rx_buf[LEP_SPI_BUFFER] = {0};
static unsigned int lepton_image[LEPTON_HEIGHT*2][LEPTON_WIDTH/2];
static uint8_t result[PACKET_SIZE*PACKETS_PER_FRAME];
static uint16_t *frameBuffer;
static int sync_delay = DEF_SYNC_DELAY;
static bool frame_ready = false;
static int fd, opt;
static int gpio_pin = DEF_GPIO_PIN;

static void push_frame()
{
    int i,j;
    unsigned int maxval = 0;
    unsigned int minval = UINT_MAX;

    for(i = 0; i < LEPTON_HEIGHT*2; i++)
        for(j = 0; j < LEPTON_WIDTH/2; j++)
        {
            if (lepton_image[i][j] > maxval)
                maxval = lepton_image[i][j];

            if (lepton_image[i][j] < minval)
                minval = lepton_image[i][j];
        }

    printf("P2\n%d %d \n%u\n", LEPTON_WIDTH, LEPTON_HEIGHT, maxval-minval);

    for(i = 0; i < LEPTON_HEIGHT*2; i += 2)
    {
        for(j = 0; j < LEPTON_WIDTH/2; j++)
            printf("%d ", lepton_image[i][j] - minval);

        for(j = 0; j < LEPTON_WIDTH/2; j++)
            printf("%d ", lepton_image[i + 1][j] - minval);

        printf("\n");
    }

    printf("\n\n");
}

static int get_frame(int fd)
{
    int ret;
    int i;
    int ip;
    uint8_t packet_number = 0;
    uint8_t segment = 0;
    uint8_t current_segment = 0;
    int packet = 0;
    int state = 0;
    int pixel = 0;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)NULL,
        .rx_buf = (unsigned long)rx_buf,
        .len = LEP_SPI_BUFFER,
        .delay_usecs = frame_delay,
        .speed_hz = speed,
        .bits_per_word = bits
    };

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

    if (ret < 1)
        return -1;

    for(ip = 0; ip < (ret / VOSPI_FRAME_SIZE); ip++)
    {
        packet = ip * VOSPI_FRAME_SIZE;

        //check for invalid packet number
        if((rx_buf[packet] & 0x0f) == 0x0f)
        {
            state = 0;
            continue;
        }

        packet_number = rx_buf[packet + 1];

        if(packet_number > 0 && state == 0)
            continue;

        if(state == 1 && packet_number == 0)
            state = 0;  //reset for new segment

        //look for the start of a segment
        if(state == 0 && packet_number == 0 && (packet + (20 * VOSPI_FRAME_SIZE)) < ret)
        {
            segment = (rx_buf[packet + (20 * VOSPI_FRAME_SIZE)] & 0x70) >> 4;

            if(segment > 0 && segment < 5 && rx_buf[packet + (20 * VOSPI_FRAME_SIZE) + 1] == 20)
            {
                state = 1;
                current_segment = segment;
            }
        }

        if(!state)
            continue;

        for(i = 4; i < VOSPI_FRAME_SIZE; i+=2)
        {
            pixel = packet_number + ((current_segment - 1) * 60);
            lepton_image[pixel][(i - 4) / 2] = (rx_buf[packet + i] << 8 | rx_buf[packet + (i + 1)]);
        }

        if(packet_number == 59)
            status_bits |= ( 0x01 << (current_segment - 1));
    }

    return status_bits;
}

static int resetLepton()
{
    LEP_CAMERA_PORT_DESC_T _port;

    LEP_RESULT _result = LEP_OpenPort(1, LEP_CCI_TWI, 400, &_port);

    if(_result != LEP_OK)
        return -1;

    _result = LEP_RunOemReboot(&_port);

    if(_result != LEP_OK)
        return -1;

    return 0;
}

static int powerdownLepton()
{
    LEP_CAMERA_PORT_DESC_T _port;

    LEP_RESULT _result = LEP_OpenPort(1, LEP_CCI_TWI, 400, &_port);

    if(_result != LEP_OK)
        return -1;

    _result = LEP_RunOemPowerDown(&_port);

    if(_result != LEP_OK)
        return -1;

    return 0;
}

static int poweronLepton()
{
    LEP_CAMERA_PORT_DESC_T _port;

    LEP_RESULT _result = LEP_OpenPort(1, LEP_CCI_TWI, 400, &_port);

    if(_result != LEP_OK)
        return -1;

    _result = LEP_RunOemPowerOn(&_port);

    if(_result != LEP_OK)
        return -1;

    return 0;
}

static int setGPIOLepton()
{
    LEP_CAMERA_PORT_DESC_T _port;

    LEP_RESULT _result = LEP_OpenPort(1, LEP_CCI_TWI, 400, &_port);

    if(_result != LEP_OK)
        return -1;

    _result = LEP_SetOemGpioMode(&_port, LEP_OEM_GPIO_MODE_VSYNC);

    if(_result != LEP_OK)
        return -1;

    return 0;
}

void vsync_isr(void)
{
    int _result;

#ifdef _DEBUG
	printf("IRQ received\n");
#endif

    _result = get_frame(fd);

    if(_result != -1)
        frame_ready = true;
#ifdef _DEBUG
    else
        printf("INVALID FRAME\n");
#endif
}

int main(int argc, char *argv[])
{
    int ret = 0;
    bool limit_frames = false;
    int max_frames = 0, frames = 0;
    int startup_delay = DEF_START_TIME;
    bool strip_frame_delimiters = false;
    bool reset_lepton = false;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1)
    {
        switch (opt)
        {
            case 'd':
                sync_delay = atoi(optarg);
                break;

            case 'D':
                startup_delay = atoi(optarg);
                break;

            case 'F':
                frame_delay = atoi(optarg);
                break;

            case 'p':
                strcpy(device,optarg);
                break;

            case 's':
                speed = atoi(optarg);
                break;

            case 'f':
                max_frames = atoi(optarg);
                limit_frames = true;
                break;

            case 'i':
                strip_frame_delimiters = true;
                break;

            case 'r':
                reset_lepton = true;
                break;

            case 'w':
                gpio_pin = atoi(optarg);
            break;

            case '?':
                printf("Usage: %s %s\n", argv[0], USAGE);
                exit(EXIT_SUCCESS);
                break;

            default:
                fprintf(stderr, "Usage: %s %s\n", argv[0], USAGE);
                exit(EXIT_FAILURE);
        }
    }

    sleep(startup_delay);

    if(reset_lepton)
        resetLepton();

    fd = open(device, O_RDWR);

    if (fd < 0)
    {
        perror("can't open device\n");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
    {
        perror("can't set spi mode\n");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1)
    {
        perror("can't get spi mode\n");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
    {
        perror("can't set bits per word\n");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
    {
        perror("can't get bits per word\n");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
    {
        perror("can't set max speed hz\n");
        exit(EXIT_FAILURE);
    }

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
    {
        perror("can't get max speed hz\n");
        exit(EXIT_FAILURE);
    }

    if(setGPIOLepton()!=0)
		perror("Unable to set GPIO3\n");
    else if(wiringPiSetup()<0)
		perror("Unable to setup wiringPi\n");
    else if (wiringPiISR(GPIO_PIN, INT_EDGE_BOTH, &vsync_isr) < 0)
		perror("Unable to set ISR\n");

    do
    {
        if(frame_ready)
        {
            frame_ready = false;

            if(!strip_frame_delimiters)
                printf("\nF\n");

            push_frame();

            if(!strip_frame_delimiters)
                printf("\nEF\n");

            frames++;
        }

        delay(DEF_VSYNC_DELAY);
    } while((!limit_frames) || (frames < max_frames));

    close(fd);

    return ret;
}
