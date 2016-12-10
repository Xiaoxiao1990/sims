/*****************************************************
 * File:spi.c
 * Date:2016-12-5
 * Description:SPI device opreats
*****************************************************/
#include <getopt.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "functions.h"
#include "arbitrator.h"

#define MAX_RETRY_TIME          (uint8_t)(3)
#define MCU_ON_SPI_NUMS         (uint8_t)(54)
static int SPI_fd[2];

extern MCU_TypeDef MCUs[MCU_NUMS];

typedef enum{
    SPI0 = 0,
    SPI1 = 1
}SPI_NUM_TypeDef;

static const char *dev_path[] ={
    "/dev/spidev0.0",
    "/dev/spidev2.0"
};

//static const char **device = &dev_path[SPI0];
static uint8_t spi_mode = 0;
static uint8_t spi_bits = (uint8_t)8;
static uint32_t spi_speed = (uint32_t)5000000;
static uint16_t trans_delay = 0;

/**
 * @brief print_usage
 * @param prog
 * Error info. print.
 */
/*
static void print_usage(const char *prog)
{
    printf("Usage: %s [-DsbdlHOLC3]\n", prog);
    puts("  -D --device   device to use (default /dev/spidev0.0)\n"
         "  -s --speed    max speed (Hz)\n"
         "  -d --delay    delay (usec)\n"
         "  -b --bpw      bits per word \n"
         "  -l --loop     loopback\n"
         "  -H --cpha     clock phase\n"
         "  -O --cpol     clock polarity\n"
         "  -L --lsb      least significant bit first\n"
         "  -C --cs-high  chip select active high\n"
         "  -3 --3wire    SI/SO signals shared\n");
}
*/
/**
 * @brief parse_opts
 * Parse the options from command line
 * @param argc
 * @param argv
 */
/*
static void parse_opts(int argc, char *argv[])
{
    while (1) {
        static const struct option lopts[] = {
            { "device",  1, 0, 'D' },
            { "speed",   1, 0, 's' },
            { "delay",   1, 0, 'd' },
            { "bpw",     1, 0, 'b' },
            { "loop",    0, 0, 'l' },
            { "cpha",    0, 0, 'H' },
            { "cpol",    0, 0, 'O' },
            { "lsb",     0, 0, 'L' },
            { "cs-high", 0, 0, 'C' },
            { "3wire",   0, 0, '3' },
            { "no-cs",   0, 0, 'N' },
            { "ready",   0, 0, 'R' },
            { NULL, 0, 0, 0 },
        };
        int c;

        c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NR", lopts, NULL);

        if (c == -1)break;

        switch (c){
        case 'D':*device = optarg;break;
        case 's':spi_speed = atoi(optarg);break;
        case 'd':trans_delay = atoi(optarg);break;
        case 'b':spi_bits = atoi(optarg);break;
        case 'l':spi_mode |= SPI_LOOP;break;
        case 'H':spi_mode |= SPI_CPHA;break;
        case 'O':spi_mode |= SPI_CPOL;break;
        case 'L':spi_mode |= SPI_LSB_FIRST;break;
        case 'C':spi_mode |= SPI_CS_HIGH;break;
        case '3':spi_mode |= SPI_3WIRE;break;
        case 'N':spi_mode |= SPI_NO_CS;break;
        case 'R':spi_mode |= SPI_READY;break;
        default:print_usage(argv[0]);break;
        }
    }
}
*/

/**
 * @brief SPI_Dev_Init
 * @param argc
 * @param argv
 * @return
 */
int SPI_Dev_Init(void)
{
    int ret = 0;
    int fd;
    int i;
    uint8_t retry = 0;
    //set by default.
    //parse_opts(argc, argv);
    for(i = 0;i <= SPI1;i++){
        //Open device
        do{
            if((fd = open(dev_path[i], O_RDWR) < 0)){
                printf("Can't open SPI[%d]",i);
                perror("");
                if(retry++ > MAX_RETRY_TIME){
                    retry = 0;
                    return -1;
                }
            }
        }while(fd < 0);

        do{
            //set SPI mode
            ret = ioctl(fd, SPI_IOC_WR_MODE, &spi_mode);
            if(ret == -1){
                printf("Can't set SPI[%d]",i);
                perror(" write mode");
                continue;
            }

            ret = ioctl(fd, SPI_IOC_RD_MODE, &spi_mode);
            if(ret == -1){
                printf("Can't get SPI[%d]",i);
                perror(" read mode");
                continue;
            }

            //set bits per word.
            ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
            if(ret == -1){
                printf("Can't set  SPI_IOC_WR_BITS_PER_WORD of SPI[%d]",i);
                perror("");
                continue;
            }

            ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
            if(ret == -1){
                printf("Can't set SPI_IOC_RD_BITS_PER_WORD of SPI[%d]",i);
                perror("");
                continue;
            }
            // max speed hz

            ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
            if(ret == -1){
                printf("Can't set SPI_IOC_WR_MAX_SPEED_HZ of SPI[%d]",i);
                perror("");
                continue;
            }

            ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
            if(ret == -1){
                printf("Can't set SPI_IOC_RD_MAX_SPEED_HZ of SPI[%d]",i);
                perror("");
                continue;
            }
        }while(retry++ < MAX_RETRY_TIME);

        if(retry >= MAX_RETRY_TIME){
            close(fd);
            return -1;
        }

        printf("SPI[%d]:\n",i);
        printf("SPI Mode: %d\n", spi_mode);
        printf("Bits Per Word: %d\n", spi_bits);
        printf("Max Speed: %d Hz (%d KHz)\n", spi_speed, spi_speed/1000);

        //use for transfer.
        SPI_fd[i] = fd;
    }

    //Arbitration initialization.
    if(Arbitrator_Init() != 0){
        printf("Arbitration Initialize Error!\n");
        return -1;
    }

    return 0;
}

/**
 * @brief transfer
 * @param mcu_num
 * @return excute status
 * Transfer the messages between main & sim board.
 * Arbitrations
 */
void * transfer(void *arg)
{
    arg = arg;
    int ret;
    MCU_TypeDef *mcu = &MCUs[0];
    struct spi_ioc_transfer tr;

    tr.len = ARRAY_SIZE(mcu->TxBuf.Buf);
    tr.delay_usecs = trans_delay;
    tr.speed_hz = spi_speed;
    tr.bits_per_word = spi_bits;
    //Transfer
    uint8_t mcu_num = 0;
    int fd;

    //for each MCUs
    for(;;){
        mcu = &MCUs[mcu_num];
        //SPI0==>MCU[0~55]   SPI1 ==> MCU[56~108]
        if(mcu_num < MCU_ON_SPI_NUMS)fd = SPI_fd[SPI0];
        else fd = SPI_fd[SPI1];

        //Arbitration
        if(Arbitrator(mcu_num) != 0){
            //Arbitration error.
            printf("Arbitrate Error at MCU[%d].\n",mcu_num);
        }

        //Check buffer state

        //Rxbuf could be used by frame_parse thread.
        if(mcu->RxBuf.state != SPI_BUF_STATE_EMPTY){
            printf("MCU[%d]'s Rx buffer is busy!\n",mcu_num);
            continue;
        }else{
            mcu->RxBuf.state = SPI_BUF_STATE_TRANSMITING;
        }

        //TxBuf could be used by frame_package thread
        if(mcu->TxBuf.state == SPI_BUF_STATE_PACKAGING){
            printf("MCU[%d]'s Tx buffer is busy!\n",mcu_num);
            continue;
        }else{
            mcu->TxBuf.state = SPI_BUF_STATE_TRANSMITING;
        }

        //Upload data to SPI
        tr.tx_buf = (unsigned long)mcu->TxBuf.Buf;
        tr.rx_buf = (unsigned long)mcu->RxBuf.Buf;

        //Transmitting.
        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if(ret < 1){
            printf("Can't send message to MCU[%d].\n",mcu_num/MCU_ON_SPI_NUMS);
            continue;
        }

        //Set buffer flags.
        mcu->RxBuf.state = SPI_BUF_STATE_FULL;
        mcu->TxBuf.state = SPI_BUF_STATE_EMPTY;
        //Reset Tx buffer
        SPI_Buf_init(&mcu->TxBuf);
    }

    //thread error...
    close(SPI_fd[SPI0]);
    close(SPI_fd[SPI1]);

    return 0;
}
