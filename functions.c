/**************************************************************************************
 * Date:2016-11-15
 * Author:Zhou Linlin
 * E-mail:461146760@qq.com
 * Description:
 * Defines some common functions used by other files.
 * ***********************************************************************************/

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#include "types.h"
#include "functions.h"
#include "log.h"
/*********************************************************
 * Function:flush spi transfer buffer.
 * Params:buffer structure
*********************************************************/
/**
 * @brief SPI_Buf_init
 * @param buf
 * Initial the buffer.
 */
void SPI_Buf_init(SPI_Buf_TypeDef *buf)
{
    buf->block = 0x00;
    buf->Length = 0x05;
    buf->checksum = 0x00;
    buf->pre_sum = 0x00;
    flush_array(buf->Buf);
}

void flush_array_r(uint8_t *arr,uint16_t arr_len)
{
#ifdef PRINT_INFO
    uint16_t i;
    printf("Before Initial:\n");
    for(i = 0;i < arr_len;i++)
    {
        printf("%.2X ",arr[i]);
        if(i%15 == 14)puts("");
    }

    puts("");
#endif
     memset(arr,0xff,arr_len);

#ifdef PRINT_INFO
    printf("After Initial:\n");
    for(i = 0;i < arr_len;i++)
    {
      printf("%.2X ",arr[i]);
       if(i%15 == 14)puts("");
    }
    puts("");
#endif
}

/**
 * @brief slot_parse
 * @param ActionTbl
 * @return SIM_NO[1~5]
 */
uint8_t slot_parse(uint8_t *ActionTbl)
{
    uint8_t slot;
    if(*ActionTbl & SIM_NO_ALL_BIT)
    {
        *ActionTbl = 0x0F;
        slot = SIM_NO_5;
    }
    else if(*ActionTbl & SIM_NO_1_BIT)
    {
        slot = SIM_NO_1;
    }
    else if(*ActionTbl & SIM_NO_2_BIT)
    {
        slot = SIM_NO_2;
    }
    else if(*ActionTbl & SIM_NO_3_BIT)
    {
        slot = SIM_NO_3;
    }
    else if(*ActionTbl & SIM_NO_4_BIT)
    {
        slot = SIM_NO_4;
    }
    else if(*ActionTbl & SIM_NO_5_BIT)
    {
        slot = SIM_NO_5;
    }
    return slot;
}

/**
 * @brief clear_flag
 * @param ActionTbl
 * @param slot[0~5]
 */
void clear_flag(uint8_t *ActionTbl, uint8_t slot)
{
    switch(slot)
    {
        case SIM_NO_ALL:*ActionTbl &= ~SIM_NO_ALL;break;
        case SIM_NO_1:*ActionTbl &= ~SIM_NO_1_BIT;break;
        case SIM_NO_2:*ActionTbl &= ~SIM_NO_2_BIT;break;
        case SIM_NO_3:*ActionTbl &= ~SIM_NO_3_BIT;break;
        case SIM_NO_4:*ActionTbl &= ~SIM_NO_4_BIT;break;
        case SIM_NO_5:*ActionTbl &= ~SIM_NO_5_BIT;break;
        default:;
    }
}

//all those functions for test print out.
extern MCU_TypeDef MCUs[MCU_NUMS];

void print_array_r(uint8_t *arr,uint16_t arr_len)
{
    uint16_t i;
//    printf("Array is:\n");
    for(i = 0;i < arr_len;i++)
    {
        printf("%.2X ",arr[i]);
        if(i%15 == 14)puts("");
    }
}

void fprint_array_r(FILE *file,uint8_t *arr,uint16_t arr_len)
{
    uint16_t i;
//    printf("Array is:\n");
    for(i = 0;i < arr_len;i++)
    {
        fprintf(file, "%.2X ",arr[i]);
        if(i%15 == 14)fputs("\n",file);
    }
    fputs("\n",file);
}

void bin_echo(uint8_t byte)
{
    uint8_t i = 0;
    printf("0B");
    for(i = 0;i < 8;i++)
    {
        if(byte & 0x80)printf("1");
        else printf("0");
        if(i == 3)putchar(' ');
        byte <<= 1;
    }
}

void print_sim(SIM_TypeDef *sim)
{
    printf("SIM State:%.2X\tAD:%.2X\n",(uint16_t)sim->state,(uint16_t)sim->AD);
    printf("ICCID:");print_array(sim->ICCID);
    puts("");
    printf("IMSI :");print_array(sim->IMSI);
    puts("");
    printf("APDU Rx Buffer Length:%d\n",(uint16_t)sim->RX_APDU.length);
    printf("APDU Rx Buffer:\n");
    print_array(sim->RX_APDU.APDU);
    puts("");
    printf("APDU Tx Buffer Length:%d\n",(uint16_t)sim->Tx_APDU.length);
    printf("APDU Tx Buffer:\n");
    print_array(sim->Tx_APDU.APDU);
}

void print_MCU(MCU_TypeDef *mcu)
{
    uint8_t i;
    //SPI_Buf_TypeDef *buf = &mcu->RxBuf;
    //print SIMs
    for(i = 0;i < SIM_NUMS;i++)
    {
        printf("============ SIM[%d] ============\n",i+1);
        print_sim(&mcu->SIM[i]);
        puts("\n");
    }
/*    //Print Rx Buffer
    printf("======================= RxBuf: =======================\n");
    printf("Buffer State:%d\n",buf->state);
    printf("Buffer Length:%d\n",buf->Length);
    printf("Buffer Left Length:%d\n",buf->Buf_Len_Left);
    printf("Buffer Checksum:%d\n",buf->checksum);
    printf("Buffer pre-checksum:%d\n",buf->pre_sum);
//    printf("Buffer:\n");
//    print_array(buf->Buf);

    buf = &mcu->TxBuf;
    //Print Tx Buffer
    printf("======================= TxBuf: =======================\n");
    printf("Buffer State:%d\n",buf->state);
    printf("Buffer Length:%d\n",buf->Length);
    printf("Buffer Left Length:%d\n",buf->Buf_Len_Left);
    printf("Buffer Checksum:%d\n",buf->checksum);
    printf("Buffer pre-checksum:%d\n",buf->pre_sum);
//    printf("Buffer:\n");
//    print_array(buf->Buf);
    //Print event flag
    */
    printf("SIM_APDUTblR :");bin_echo(mcu->SIM_APDUTblR);puts("");
    printf("SIM_APDUTblW :");bin_echo(mcu->SIM_APDUTblN);puts("");
    printf("SIM_CheckErrR:");bin_echo(mcu->SIM_CheckErrR);puts("");
    printf("SIM_CheckErrW:");bin_echo(mcu->SIM_CheckErrN);puts("");
    printf("SIM_InfoTblR :");bin_echo(mcu->SIM_InfoTblR);puts("");
    printf("SIM_InfoTblW :");bin_echo(mcu->SIM_InfoTblN);puts("");
    printf("SIM_StateTblR:");bin_echo(mcu->SIM_StateTblR);puts("");
    printf("SIM_StateTblW:");bin_echo(mcu->SIM_StateTblN);puts("");
    printf("SIM_VersionR :");bin_echo(mcu->VersionR);puts("");
    printf("SIM_VersionW :");bin_echo(mcu->VersionN);puts("");
    //Print version
    printf("Hardware version:");print_array(mcu->HardWare_Version);puts("");
    printf("Software version:");print_array(mcu->SoftWare_Version);puts("");
}

double time_use(struct timeval *start_time, struct timeval *end_time)
{
    return ((double)1000000*(end_time->tv_sec - start_time->tv_sec) + end_time->tv_usec - start_time->tv_usec);//us
}

void thread_sleep(uint32_t sec)
{
    pthread_mutex_t tmutex_sleep = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t tcond_sleep = PTHREAD_COND_INITIALIZER;
    struct timeval now;
    struct timespec timeout;

    pthread_mutex_init(&tmutex_sleep, NULL);
    pthread_cond_init(&tcond_sleep, NULL);


    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + sec;
    timeout.tv_nsec = now.tv_usec * 1000;
    pthread_cond_timedwait(&tcond_sleep, &tmutex_sleep, &timeout);

    pthread_cond_destroy(&tcond_sleep);
    pthread_mutex_destroy(&tmutex_sleep);
}

/******************************************************************************
 * Function:print changes
 * ***************************************************************************/

extern APDU_BUF_TypeDef apdu2;
extern int sim_read(uint16_t sim_no, DataType_TypeDef datatype, void *data_addr);

void SIMs_Printer(void)
{
    MCU_TypeDef *mcu;
    uint8_t *actionTbl;
    uint8_t sim_no = 0;
    uint8_t i;
    uint16_t abs_sim_no;
    APDU_BUF_TypeDef *apdu;

    for(i = 0;i < MCU_NUMS;i++){
        mcu = &MCUs[i];

        while(mcu->SIM_StateTblN | mcu->SIM_APDUTblN | mcu->SIM_CheckErrN | mcu->SIM_InfoTblN | mcu->VersionN){
            if(mcu->SIM_StateTblN){
                actionTbl = &(mcu->SIM_StateTblN);
                sim_no = slot_parse(actionTbl) - 1;

                printf("SIM[%d]'s state changed:",((i*SIM_NUMS) + sim_no + 1));
                printf("%.2X\n",mcu->SIM[sim_no].state);

            } else if(mcu->SIM_APDUTblN) {
                actionTbl = &(mcu->SIM_APDUTblN);
                sim_no = slot_parse(actionTbl) - 1;//offset 1.

                abs_sim_no = (i*SIM_NUMS)+sim_no+1;

                printf("Recv. APDU ACK from SIM[%d]:\n", abs_sim_no);
                apdu = &(mcu->SIM[sim_no].RX_APDU);
                print_array_r(apdu->APDU,apdu->length);
                puts("");
            } else if(mcu->SIM_CheckErrN) {
                actionTbl = &(mcu->SIM_CheckErrN);
                sim_no = slot_parse(actionTbl);

                printf("[%s:%d]SIM[%d] transmit error.\n", __FILE__, __LINE__, ((i*SIM_NUMS)+sim_no));
            } else if(mcu->SIM_InfoTblN) {
                actionTbl = &(mcu->SIM_InfoTblN);
                sim_no = slot_parse(actionTbl) - 1;//offset 1.

                printf("SIM[%d]'s information changed:\n",((i*SIM_NUMS) + sim_no + 1));
                printf("ICCID:");
                print_array(mcu->SIM[sim_no].ICCID);
                puts("");
                printf("IMSI :");
                print_array(mcu->SIM[sim_no].IMSI);
                puts("");
                printf("AD:%.2X\n",mcu->SIM[sim_no].AD);
            } else if(mcu->VersionN) {
                actionTbl = &(mcu->VersionN);
                mcu->VersionN &= 0x00;
                printf("MCU[%d]'s hardware & software Version Changed:\nHardware version:",i);
                print_array(mcu->HardWare_Version);puts("");
                printf("Software version:");
                print_array(mcu->HardWare_Version);puts("");
                continue;
            }

            clear_flag(actionTbl, sim_no + 1);
        }
    }
}

