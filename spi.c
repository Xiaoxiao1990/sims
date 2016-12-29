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
#include "log.h"
#include "sim.h"

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


int package(uint8_t mcu_num)
{//called before transfer.
    uint8_t *actionTbl,*data;
    uint8_t sim_no;
    uint8_t i,apdu_len,block_len;
    MCU_TypeDef *mcu = &MCUs[mcu_num];
    DataType_TypeDef datatype;
    SPI_Buf_TypeDef *Tx = &mcu->TxBuf;

    //Check whether have something to send to MCU.
    while(mcu->SIM_StateTblR | mcu->SIM_ResetTbl | mcu->SIM_StopTbl | mcu->SIM_APDUTblR | mcu->SIM_InfoTblR | mcu->VersionR | mcu->SIM_CheckErrR){
        //get info.
        if(mcu->SIM_StateTblR){
            actionTbl = &(mcu->SIM_StateTblR);
            sim_no = slot_parse(actionTbl);
            datatype = READ_STATE;
            //printf("Read SIM[%d] State.\n",((mcu_num*SIM_NUMS)+sim_no));
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
        //package
        apdu_len = mcu->SIM[sim_no - 1].Tx_APDU.length; // offset 1. array start from 0
        data = mcu->SIM[sim_no - 1].Tx_APDU.APDU;

        block_len = block_length_check(datatype, apdu_len);
        //calculate Tx buffer free space.

        if((block_len + Tx->Length) >= SPI_TRANSFER_MTU){
            clear_flag(actionTbl,sim_no);
            return -1;
        }
        Tx->Buf[0] = DATA_FRAME_HEAD1;
        Tx->Buf[1] = DATA_FRAME_HEAD2;

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
            //logs(mcu->SIM[sim_no-1].log,"APDU[%d] send to sim:\n", apdu_len);
            //fprint_array_r(mcu->SIM[sim_no-1].log, data, apdu_len);
            for(i = 0;i < apdu_len;i++){
                Tx->Buf[Tx->Length++] = data[i];
                data[i] = 0xFF;//flush APDU tx buffer.
                Tx->pre_sum += data[i];
            }
            //flush APDU buffer
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

        //clear flag
        clear_flag(actionTbl,sim_no);

        //printf("MCU[%d]Tx buffer:\n", mcu_num);
        //print_array(Tx->Buf);
   }
    return 0;
}


/******************************************************************************************
 * Function: Set flags while get information from card boards.
 * ***************************************************************************************/
static void set_flag(uint8_t *ActionTbl, uint8_t slot)
{
    //printf("flag before set:%d\n",*ActionTbl);
    switch(slot)
    {
        case SIM_NO_ALL:*ActionTbl |= SIM_NO_ALL;break;
        case SIM_NO_1:*ActionTbl |= SIM_NO_1_BIT;break;
        case SIM_NO_2:*ActionTbl |= SIM_NO_2_BIT;break;
        case SIM_NO_3:*ActionTbl |= SIM_NO_3_BIT;break;
        case SIM_NO_4:*ActionTbl |= SIM_NO_4_BIT;break;
        case SIM_NO_5:*ActionTbl |= SIM_NO_5_BIT;break;
        default:;
    }
    //printf("flag after set:%d\n",*ActionTbl);
}

/******************************************************************************************
 * Function:Frame parse
 * Params:Rx_Buf
 * ***************************************************************************************/
#define PARSE_FRAME_HEAD1                                   (uint8_t)0x00
#define PARSE_FRAME_HEAD2                                   (uint8_t)0x01
#define PARSE_TOTAL_LENGTH                                  (uint8_t)0x02
#define PARSE_BLOCK_NUMS                                    (uint8_t)0x03
#define PARSE_BLOCK_SLOT                                    (uint8_t)0x04
#define PARSE_BLOCK_SUBLEN                                  (uint8_t)0x05
#define PARSE_BLOCK_TYPE                                    (uint8_t)0x06
#define PARSE_BLOCK_DATA                                    (uint8_t)0x07
#define PARSE_FRAME_CHECKSUM                                (uint8_t)0x08

/**
 * @brief frame_parse
 * data frame from mcu's parse ,run in a depend thread
 * @param mcu
 * @return
 */
int parse(uint8_t mcu_num)
{
    uint16_t total_length,
            i = 0,
            blocks = 0,
            slot = 0,
            sublen = 0,
            k = 0;
    uint8_t step = 0;

    DataType_TypeDef datatype;
    MCU_TypeDef *mcu = &MCUs[mcu_num];
    SPI_Buf_TypeDef *Rx = &(mcu->RxBuf);

    for(i = 0;i < ARRAY_SIZE(Rx->Buf);i++){//Frame start
        switch(step){
            case PARSE_FRAME_HEAD1:{
                if(Rx->Buf[i] == DATA_FRAME_HEAD1){
                    step++;
                }
            }break;
            case PARSE_FRAME_HEAD2:{
                if(Rx->Buf[i] == DATA_FRAME_HEAD2){
                    step++;
                }
                else step--;
            }break;
            case PARSE_TOTAL_LENGTH:{
                //MCU online check...
                mcu->time_out = MCU_TIME_OUT;
                if(mcu->online == OFF_LINE){
                    mcu->online = ON_LINE;
                    logs(misc_log, "MCU[%d] on line.\n", mcu_num);
                    printf("MCU[%d] on line.\n", mcu_num);
                }

                total_length = (uint16_t)Rx->Buf[i++] << 8;
                total_length += (uint16_t)Rx->Buf[i];
                if(total_length < 2){//data length should be great than 2 bytes.
                    printf("Frame length is too short:[%d btyes].\n",total_length);
                } else if(total_length > ARRAY_SIZE(Rx->Buf)) {
                    printf("Frame length is too long:[%d btyes].\n",total_length);
                } else{
                    //Calculate checksum.
#ifdef CHECKSUM_ON
                    uint16 j = 0;
                    uint8_t checksum = 0;
                    for(j = 2;j < (total_length + 2);j++){//start frome [2] to [2+total_length]
                        checksum += Rx->Buf[j];
                    }
                    if(checksum != Rx->Buf[j]){
                        printf("Received Data Checksum Error!\n");
                        //report an error?
                        goto FRAME_END;
                    }
#endif
                    step++;
                }
            }break;
            case PARSE_BLOCK_NUMS:{
                blocks = Rx->Buf[i];
                if(blocks < 1){//data blocks should be great than 0.
                    printf("Data blocks should be more than 0. Here blocks = %d.\n", blocks);
                } else step++;
            }break;
            case PARSE_BLOCK_SLOT:{
                slot = Rx->Buf[i];
                step++;
            }break;
            case PARSE_BLOCK_SUBLEN:{
                sublen = Rx->Buf[i];
                step++;
            }break;
            case PARSE_BLOCK_TYPE:{
                datatype = Rx->Buf[i];
                step++;
#ifdef CODING_DEBUG_NO_PRINT
                printf("Datatype:");
                switch(datatype){
                    case DUMMY_READ:puts("DUMMY_READ");break;
                    case READ_SWHW:puts("READ_SWHW");break;
                    case STOP_SIM:puts("STOP_SIM");break;
                    case RESET_SIM:puts("RESET_SIM");break;
                    case READ_INFO:puts("READ_INFO");break;
                    case READ_STATE:puts("READ_STATE");break;
                    case APDU_CMD:puts("APDU_CMD");break;
                    case TRANS_ERR:puts("TRANS_ERR");break;
                    default:;
                }
#endif
            }break;
            case PARSE_BLOCK_DATA:{
                switch (datatype){
                    case DUMMY_READ:break;          //Never been here
                    case READ_SWHW:{
                        mcu->SoftWare_Version[0] = Rx->Buf[i++];
                        mcu->SoftWare_Version[1] = Rx->Buf[i++];
                        mcu->SoftWare_Version[2] = Rx->Buf[i++];
                        mcu->HardWare_Version[0] = Rx->Buf[i++];
                        mcu->HardWare_Version[1] = Rx->Buf[i++];
                        mcu->HardWare_Version[2] = Rx->Buf[i];

                        //Get new version
                        mcu->VersionN |= 0x1F;
                    }break;
                    case STOP_SIM:break;            //Never been here
                    case RESET_SIM:break;           //Never been here
                    case READ_INFO:{//Read SIM Info. check the slot number.
                        if((slot == 0) || (slot > SIM_NO_5)){
                            printf("MCU[%d]'s data length:%d\n",mcu_num, total_length);
                            printf("MCU[%d]'s Data type is SIM info. But slot = %d.\nRx->Buf:\n",mcu_num, slot);
                            print_array(Rx->Buf);
                            printf("============================================\n");
                            //this block is broken, but there have something in the end.
                            //there have two ways, use the data in the end,else throw it.
                            for(k = 0;k < ICCID_LENGTH;k++)i++;
                            for(k = 0;k < IMSI_LENGTH;k++)i++;
                            //i++;
                            // goto FRAME_END;
                            break;
                        }
                        //restore SIM info.
                        for(k = 0;k < ICCID_LENGTH;k++){//ICCID
                            mcu->SIM[slot-1].ICCID[k] = Rx->Buf[i++];
                        }
                        for(k = 0;k < IMSI_LENGTH;k++){//IMSI
                            mcu->SIM[slot-1].IMSI[k] = Rx->Buf[i++];
                        }
                        //AD may needn't
                        mcu->SIM[slot-1].AD = Rx->Buf[i];
                        set_flag(&mcu->SIM_InfoTblN,slot);
                    }break;
                    case READ_STATE:{
                        //SIM state. check slot ?
                        if(slot > 0){
                            if(mcu->SIM[slot - 1].state != Rx->Buf[i])
                                set_flag(&mcu->SIM_StateTblN,slot);
                            mcu->SIM[slot-1].state = Rx->Buf[i];
                        } else {
                           logs(misc_log, "Slot number ERROR in MCU[%d]'s Rx buffer.\n", mcu_num);
                        }
                    }break;
                    case APDU_CMD:{
                    //may be should check slot.
                        for(k = 0;k < (sublen - 1);k++)mcu->SIM[slot-1].RX_APDU.APDU[k] = Rx->Buf[i++];
                        mcu->SIM[slot-1].RX_APDU.length = k;
                        //logs(mcu->SIM[slot-1].log,"Recv. from sim APDU[%d]:\n",k);
                        //fprint_array_r(mcu->SIM[slot-1].log, mcu->SIM[slot-1].RX_APDU.APDU,k);
                        set_flag(&mcu->SIM_APDUTblN,slot);
                        i--;//back to data(use for PARSE_FRAME_CHECKSUM
                    }break;
                    case TRANS_ERR:{//may be ... never use the function. just redo it if time out,determined by host.
                        set_flag(&mcu->SIM_CheckErrN,slot);
                    }break;               //Maybe should do something here.
                    default:;
                }
                if(--blocks > 0)step = PARSE_BLOCK_SLOT;
                else step = PARSE_FRAME_HEAD1;
            }break;
            case PARSE_FRAME_CHECKSUM:{
                //This state has been do before parsing information.
                //Here just recovery the status machine.
            }break;
            default:;
        }

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
void * spi0(void *arg)
{
    arg = arg;
    int ret;
    uint8_t mcu_num = 0;
    MCU_TypeDef *mcu = &MCUs[mcu_num];
    struct spi_ioc_transfer tr;

    tr.len = ARRAY_SIZE(mcu->TxBuf.Buf);
    tr.delay_usecs = trans_delay;
    tr.speed_hz = spi_speed;
    tr.bits_per_word = spi_bits;

    //for each MCUs
    for(;;){
        package(mcu_num);
        //Arbitration
        if(Arbitrator(mcu_num) != 0){
            //Arbitration error.
            printf("Arbitrate Error at MCU[%d].\n",mcu_num);
        }

        //Upload data to SPI
        tr.tx_buf = (unsigned long)mcu->TxBuf.Buf;
        tr.rx_buf = (unsigned long)mcu->RxBuf.Buf;

        //Transmitting.
        ret = ioctl(SPI_fd[SPI0], SPI_IOC_MESSAGE(1), &tr);
        if(ret < 1){
            printf("Can't send message to MCU[%d].\n",mcu_num);
            logs(misc_log, "Can't send message to MCU[%d].\n",mcu_num);
        }

        parse(mcu_num);

        SPI_Buf_init(&mcu->TxBuf);
        SPI_Buf_init(&mcu->RxBuf);
        mcu_num++;
        if(mcu_num >= 36){
            mcu_num = 0;
        }
        mcu = &MCUs[mcu_num];
    }

    //thread error...
    close(SPI_fd[SPI0]);
    logs(misc_log,"SPI0 Abort!\n");
    pthread_exit(NULL);
}

void * spi2(void *arg)
{
    arg = arg;
    int ret;
    uint8_t mcu_num = 72;
    MCU_TypeDef *mcu = &MCUs[mcu_num];
    struct spi_ioc_transfer tr;

    tr.len = ARRAY_SIZE(mcu->TxBuf.Buf);
    tr.delay_usecs = trans_delay;
    tr.speed_hz = spi_speed;
    tr.bits_per_word = spi_bits;

    //for each MCUs
    for(;;){

        package(mcu_num);
//        printf("Exchange with MCU[%d].\n",mcu_num);

        //Arbitration
        if(Arbitrator(mcu_num) != 0){
            //Arbitration error.
            printf("Arbitrate Error at MCU[%d].\n",mcu_num);
        }

        //Upload data to SPI
        tr.tx_buf = (unsigned long)mcu->TxBuf.Buf;
        tr.rx_buf = (unsigned long)mcu->RxBuf.Buf;

        //Transmitting.
        ret = ioctl(SPI_fd[SPI1], SPI_IOC_MESSAGE(1), &tr);
        if(ret < 1){
            printf("Can't send message to MCU[%d].\n",mcu_num);
            logs(misc_log, "Can't send message to MCU[%d].\n",mcu_num);
        }

        parse(mcu_num);

        SPI_Buf_init(&mcu->RxBuf);
        SPI_Buf_init(&mcu->TxBuf);

        mcu_num++;
        if(mcu_num >= MCU_NUMS){
            mcu_num = 72;
        }
        mcu = &MCUs[mcu_num];
    }

    //thread error...
    close(SPI_fd[SPI1]);
    logs(misc_log,"SPI2 Abort!\n");
    pthread_exit(NULL);
}
