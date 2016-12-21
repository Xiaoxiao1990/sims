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

typedef enum{
    SPI0 = 0,
    SPI1 = 1
}SPI_NUM_TypeDef;

static const char *dev_path[] ={
    "/dev/spidev0.0",
    "/dev/spidev2.0"
};

//static const char **device = &dev_path[SPI0];
static uint8_t spi_mode = (uint8_t)0;
static uint8_t spi_bits = (uint8_t)8;
static uint32_t spi_speed = (uint32_t)11000000;
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
        case 'D':optarg = optarg;break;
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

/*
int SPI_Dev_Init(void)
{
    int ret = 0;
    int fd;
    int i;

    uint8_t retry = 0;
    //set by default.
 //   parse_opts(argc, argv);
    for(i = 0;i < 2;i++){
        //Open device
        printf("Open device : %s\n",dev_path[i]);
        if((fd = open(dev_path[i], O_RDWR) < 0)){
            printf("Can't open SPI[%d]",i);
            perror("");puts("");
            if(retry++ > MAX_RETRY_TIME){
                retry = 0;
                return -1;
            }
        }

        //set SPI mode
        printf("Set SPI mode.\n");
        ret = ioctl(fd, SPI_IOC_WR_MODE, &spi_mode);
        if(ret == -1){
            printf("Can't set SPI[%d]",i);
            perror(" write mode");puts("");
            retry++;
        }

        ret = ioctl(fd, SPI_IOC_RD_MODE, &spi_mode);
        if(ret == -1){
            printf("Can't set SPI[%d]",i);
            perror(" read mode");puts("");
            retry++;
        }

        //set bits per word.
        ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
        if(ret == -1){
            printf("Can't set  SPI_IOC_WR_BITS_PER_WORD of SPI[%d]",i);
            perror("");puts("");
            retry++;
        }

        ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
        if(ret == -1){
            printf("Can't set SPI_IOC_RD_BITS_PER_WORD of SPI[%d]",i);
            perror("");puts("");
            retry++;
        }
        // max speed hz

        ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
        if(ret == -1){
            printf("Can't set SPI_IOC_WR_MAX_SPEED_HZ of SPI[%d]",i);
            perror("");puts("");
            retry++;
        }

        ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
        if(ret == -1){
            printf("Can't set SPI_IOC_RD_MAX_SPEED_HZ of SPI[%d]",i);
            perror("");puts("");
            retry++;
        }

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
*/
/*******************************************************************
 * Function:SPI device initial
 * ****************************************************************/

int SPI_Dev_Init(void)
{
    int ret = 0;
    int fd;
    //Parse options
//    parse_opts(argc, argv);

    //SPI0 init
    fd = open(dev_path[SPI0], O_RDWR);
    if (fd < 0)perror("Can't open SPI[0]");

    //set SPI mode
    ret = ioctl(fd, SPI_IOC_WR_MODE, &spi_mode);
    if(ret == -1)perror("Can't set SPI[0] write mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE, &spi_mode);
    if(ret == -1)perror("Can't get SPI[0] read mode");

    //set bits per word.
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
    if(ret == -1)perror("Can't set  SPI_IOC_WR_BITS_PER_WORD of SPI[0]");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
    if(ret == -1)perror("Can't set SPI_IOC_RD_BITS_PER_WORD of SPI[0]");

    // max speed hz

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
    if(ret == -1)perror("Can't set SPI_IOC_WR_MAX_SPEED_HZ of SPI[0]");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
    if(ret == -1)perror("Can't set SPI_IOC_RD_MAX_SPEED_HZ of SPI[0]");

    printf("SPI[0]:\n");
    printf("SPI Mode: %d\n", spi_mode);
    printf("Bits Per Word: %d\n", spi_bits);
    printf("Max Speed: %d Hz (%d KHz)\n", spi_speed, spi_speed/1000);

    SPI_fd[SPI0] = fd;
    //close SPI0
    //close(fd);

    //SPI1 init
    fd = open(dev_path[SPI1], O_RDWR);
    if (fd < 0)perror("Can't open SPI[1]");

    //set SPI mode
    ret = ioctl(fd, SPI_IOC_WR_MODE, &spi_mode);
    if(ret == -1)perror("Can't set SPI[1] write mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE, &spi_mode);
    if(ret == -1)perror("Can't get SPI[1] read mode");

    //set bits per word.
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
    if(ret == -1)perror("Can't set  SPI_IOC_WR_BITS_PER_WORD of SPI[1]");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
    if(ret == -1)perror("Can't set SPI_IOC_RD_BITS_PER_WORD of SPI[1]");

    // max speed hz

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
    if(ret == -1)perror("Can't set SPI_IOC_WR_MAX_SPEED_HZ of SPI[1]");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
    if(ret == -1)perror("Can't set SPI_IOC_RD_MAX_SPEED_HZ of SPI[1]");

    printf("SPI[1]:\n");
    printf("SPI Mode: %d\n", spi_mode);
    printf("Bits Per Word: %d\n", spi_bits);
    printf("Max Speed: %d Hz (%d KHz)\n", spi_speed, spi_speed/1000);

    SPI_fd[SPI1] = fd;

    //Arbitration initialization.
    printf("Initial Arbitrator...\n");
    if(Arbitrator_Init() != 0){
        printf("Arbitration Initialize Error!\n");
        return -1;
    }

    return 0;
}
/*
//reentrant
int frame_package(uint16_t mcu_num)
{//Running in an independent thread.
    uint8_t i;
    uint8_t sim_no = 0;
    uint8_t *data, apdu_len;
    uint16_t block_len;
    uint8_t *actionTbl;

    MCU_TypeDef *mcu = &MCUs[mcu_num];
    DataType_TypeDef datatype;
    SPI_Buf_TypeDef *Tx = &(mcu->TxBuf);

    while(mcu->SIM_StateTblR | mcu->SIM_ResetTbl | mcu->SIM_StopTbl | mcu->SIM_APDUTblR | mcu->SIM_InfoTblR | mcu->VersionR | mcu->SIM_CheckErrR){
        //printf("There is something new need to pack to the buffer.\n");
        if(mcu->SIM_StateTblR){
            actionTbl = &(mcu->SIM_StateTblR);
            sim_no = slot_parse(actionTbl);
            datatype = READ_STATE;
            printf("Read SIM[%d] State.\n",((mcu_num*SIM_NUMS)+sim_no));
        } else if (mcu->SIM_ResetTbl) {
            actionTbl = &(mcu->SIM_ResetTbl);
            sim_no = slot_parse(actionTbl);
            datatype = RESET_SIM;
            printf("Reset SIM[%d].\n",((mcu_num*SIM_NUMS)+sim_no));
        } else if(mcu->SIM_StopTbl) {
            actionTbl = &(mcu->SIM_StopTbl);
            sim_no = slot_parse(actionTbl);
            datatype = STOP_SIM;
            printf("Stop SIM[%d].\n",((mcu_num*SIM_NUMS)+sim_no));
        } else if(mcu->SIM_APDUTblR) {
            actionTbl = &(mcu->SIM_APDUTblR);
            sim_no = slot_parse(actionTbl);
            datatype = APDU_CMD;
            printf("APDU Commands To SIM[%d].\n",((mcu_num*SIM_NUMS)+sim_no));
        } else if(mcu->SIM_CheckErrR) {
            actionTbl = &(mcu->SIM_CheckErrR);
            sim_no = slot_parse(actionTbl);
            datatype = TRANS_ERR;
            printf("SIM[%d] Checksum Error.\n",((mcu_num*SIM_NUMS)+sim_no));
        } else if(mcu->SIM_InfoTblR) {
            actionTbl = &(mcu->SIM_InfoTblR);
            sim_no = slot_parse(actionTbl);
            datatype = READ_INFO;
            printf("Read Information From SIM[%d].\n",((mcu_num*SIM_NUMS)+sim_no));
        } else if(mcu->VersionR) {
            actionTbl = &(mcu->VersionR);
            mcu->VersionR &= 0x00;
            sim_no = 0;//slot_parse(actionTbl);
            datatype = READ_SWHW;
            printf("Read HW&SW Version From MCU[%d].\n",mcu_num + 1);
        }

        apdu_len = mcu->SIM[sim_no - 1].Tx_APDU.length; // offset 1. array start from 0
        data = mcu->SIM[sim_no - 1].Tx_APDU.APDU;

        block_len = block_length_check(datatype, apdu_len);
        //calculate Tx buffer free space.
        if(Tx->state == SPI_BUF_STATE_EMPTY){//The first time come to this buffer.
            if(SPI_TRANSFER_MTU <= (mcu->TxBuf.Length + block_len)){
                //Illegal blocks, data length is too long.skip this block,clear the CMD flag.
                printf("Illegal data block:Data block length[%d] great than Tx buffer maximum length[%d].\n", block_len, SPI_TRANSFER_MTU);
                clear_flag(actionTbl, sim_no);
                continue;
            }
        } else if(Tx->state == SPI_BUF_STATE_READY){//There is something packed in this buffer.
            if(SPI_TRANSFER_MTU <= (mcu->TxBuf.Length + block_len)){
                //There is no space to load this block,wait for next package.
                printf("MCU[%d]->Tx buffer is no space to allocate data block, try again by next time.\n", mcu_num);
                Tx->state = SPI_BUF_STATE_FULL;
                break;
            }
        } else if(Tx->state == SPI_BUF_STATE_FULL){
            if(SPI_TRANSFER_MTU > (mcu->TxBuf.Length + block_len)){
                //There is no space to load this block,wait for next package.
                printf("MCU[%d]->Tx buffer is no space to allocate data block, try again by next time.\n", mcu_num);
                Tx->state = SPI_BUF_STATE_READY;
            } else break;
        } else{
            logs(misc_log,"MCU[%d]'s Tx buffer state is not available.\n", mcu_num);
            return -1;
        }
        //state:EMPTY,READY
        switch(Tx->state){
            case SPI_BUF_STATE_EMPTY:{
                //Frame Head;
                Tx->Buf[0] = DATA_FRAME_HEAD1;
                Tx->Buf[1] = DATA_FRAME_HEAD2;

                //Locate to block
                //|0   1|2       3|4     |5      |   |       |
                //|A5 A5|lenH lenL|blocks|data[0]|...|data[n]|checksum|
                Tx->Length = 5;//Length is Buf[] position.
                Tx->pre_sum = 0;//clear
            }//go on
            case SPI_BUF_STATE_READY:{
                //data block begin
                Tx->Buf[Tx->Length] = sim_no;                       //slot
                Tx->pre_sum += Tx->Buf[Tx->Length++];

                //printf("Length = %d\n",MCU_Tx_WriteP->Length);
                //|0   |1      |2       |
                //|slot|sub_len|datatype|data|
                Tx->Buf[Tx->Length] = (datatype == APDU_CMD)?(apdu_len+1):0x01;//sub length
               // printf("sub length = %d\n",data_len);
                Tx->pre_sum += Tx->Buf[Tx->Length++];

                Tx->Buf[Tx->Length] = datatype;                     //data type
                Tx->pre_sum += Tx->Buf[Tx->Length++];

                //Others are all 1 byte length.
                if(datatype == APDU_CMD){//Here,Tx->Length point to apdu data[].
                    for(i = 0;i < apdu_len;i++){
                        Tx->Buf[Tx->Length++] = data[i];
                        data[i] = 0xFF;//flush APDU tx buffer.
                        Tx->pre_sum += data[i];
                    }
                    //flush APDU buffer length
                    mcu->SIM[sim_no - 1].Tx_APDU.length = 0;//clear
                }
                //Here,Tx->Length point to checksum.
                //Block quantity
                Tx->block++;
                Tx->Buf[4] = Tx->block;

                //pre_sum stop to calculate.
                Tx->checksum = Tx->pre_sum;
                Tx->checksum += Tx->Buf[4];

                //Data total length = Tx->Length - checksum(1 byte) - frame head(2 bytes) - total length(2 bytes) + 1(start with 0) = Length - 4
                Tx->Buf[2] = (uint8_t)((Tx->Length - 4) >> 8);
                Tx->checksum += Tx->Buf[2];
                Tx->Buf[3] = (uint8_t)((Tx->Length - 4) & 0xFF);
                Tx->checksum += Tx->Buf[3];
                //data block end
                Tx->Buf[Tx->Length] = Tx->checksum;
                Tx->state = SPI_BUF_STATE_READY;//for next block.

                //clear flag
                clear_flag(actionTbl,sim_no);
            }break;
            default:;
        }
//            printf("MCU[%d]' Tx buffer:\n",mcu_num);
//            print_array(Tx->Buf);puts("");
    }
    return 0;
}
*/

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
    uint8_t rx_state,tx_state;
    int fd;

    //for each MCUs
    for(;;){
        //Check buffer state first

        pthread_mutex_lock(&tmutex_mcu_buf_access);
        rx_state = mcu->RxBuf.state;
        tx_state = mcu->TxBuf.state;
        //Rxbuf could be used by frame_parse thread.
        if(mcu->RxBuf.state != SPI_BUF_STATE_EMPTY){
//            printf("MCU[%d]'s Rx buffer is busy,state = %d.\n",mcu_num,mcu->RxBuf.state);
            pthread_mutex_unlock(&tmutex_mcu_buf_access);
            goto NEXT_MCU;
        }else{
            mcu->RxBuf.state = SPI_BUF_STATE_TRANSMITING;
        }

        //TxBuf could be used by frame_package thread
        if(mcu->TxBuf.state == SPI_BUF_STATE_PACKAGING){
//          printf("MCU[%d]'s Tx buffer is busy,state = %d.\n",mcu_num,mcu->TxBuf.state);
            //*
            //*NOTE*
            mcu->RxBuf.state = rx_state;
            pthread_mutex_unlock(&tmutex_mcu_buf_access);
            goto NEXT_MCU;
        }else{
            mcu->TxBuf.state = SPI_BUF_STATE_TRANSMITING;
        }
        pthread_mutex_unlock(&tmutex_mcu_buf_access);

//        printf("Exchange with MCU[%d].\n",mcu_num);
        //SPI0==>MCU[0~55]   SPI1 ==> MCU[56~108]
        if(mcu_num < MCU_ON_SPI_NUMS)fd = SPI_fd[SPI0];
        else fd = SPI_fd[SPI1];

        //Arbitration
        if(Arbitrator(mcu_num) != 0){
            //Arbitration error.
            printf("Arbitrate Error at MCU[%d].\n",mcu_num);
        }

        //Upload data to SPI
        tr.tx_buf = (unsigned long)mcu->TxBuf.Buf;
        tr.rx_buf = (unsigned long)mcu->RxBuf.Buf;

        //Transmitting.
        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if(ret < 1){
            printf("Can't send message to MCU[%d].\n",mcu_num/MCU_ON_SPI_NUMS);
            //something wrong,recovery the buffer state.
            mcu->TxBuf.state = tx_state;
            mcu->RxBuf.state = rx_state;
            goto NEXT_MCU;
        }

        //Reset Tx buffer
        SPI_Buf_init(&mcu->TxBuf);
        //Set buffer flags.
        mcu->RxBuf.state = SPI_BUF_STATE_FULL;
        mcu->TxBuf.state = SPI_BUF_STATE_EMPTY;
//        printf("MCU[%d]'s Rx is full.\n",mcu_num);

NEXT_MCU:
        mcu_num++;
        if(mcu_num >= MCU_NUMS){
            mcu_num = 0;
        }
        mcu = &MCUs[mcu_num];
    }

    //thread error...
    close(SPI_fd[SPI0]);
    close(SPI_fd[SPI1]);
    printf("Transfer error!\n");
    pthread_exit(NULL);
}
