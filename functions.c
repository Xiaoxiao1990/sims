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
    buf->Length = 0x00;
    buf->Buf_Len_Left = SPI_TRANSFER_MTU;
    buf->checksum = 0x00;
    buf->pre_sum = 0x00;
    buf->state = SPI_BUF_STATE_EMPTY;
    flush_array(buf->Buf);
}

void flush_array_r(uint8_t *arr,uint16_t arr_len)
{
    uint16_t i;
    printf("Before Initial:\n");
    for(i = 0;i < arr_len;i++)
    {
        printf("%.2X ",arr[i]);
        if(i%15 == 14)puts("");
    }

    puts("");

     memset(arr,0xff,arr_len);


    printf("After Initial:\n");
    for(i = 0;i < arr_len;i++)
    {
      printf("%.2X ",arr[i]);
       if(i%15 == 14)puts("");
    }
    puts("");

}

uint8_t slot_parse(uint8_t *ActionTbl)
{
    uint8_t slot;
    if(*ActionTbl & SIM_NO_ALL_BIT)
    {
        slot = SIM_NO_ALL;
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
    printf("APDU Rx Buffer Length:%d\n",(uint16_t)sim->Rx_Length);
    printf("APDU Rx Buffer:\n");
    print_array(sim->Rx_APDU);
    puts("");
    printf("APDU Tx Buffer Length:%d\n",(uint16_t)sim->Tx_Length);
    printf("APDU Tx Buffer:\n");
    print_array(sim->Tx_APDU);
}

void print_MCU(MCU_TypeDef *mcu)
{
    uint8_t i;
    SPI_Buf_TypeDef *buf = &mcu->RxBuf;
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
    printf("SIM_APDUTblW :");bin_echo(mcu->SIM_APDUTblW);puts("");
    printf("SIM_CheckErrR:");bin_echo(mcu->SIM_CheckErrR);puts("");
    printf("SIM_CheckErrW:");bin_echo(mcu->SIM_CheckErrW);puts("");
    printf("SIM_InfoTblR :");bin_echo(mcu->SIM_InfoTblR);puts("");
    printf("SIM_InfoTblW :");bin_echo(mcu->SIM_InfoTblW);puts("");
    printf("SIM_StateTblR:");bin_echo(mcu->SIM_StateTblR);puts("");
    printf("SIM_StateTblW:");bin_echo(mcu->SIM_StateTblW);puts("");
    printf("SIM_VersionR :");bin_echo(mcu->VersionR);puts("");
    printf("SIM_VersionW :");bin_echo(mcu->VersionW);puts("");
    //Print version
    printf("Hardware version:");print_array(mcu->HardWare_Version);puts("");
    printf("Software version:");print_array(mcu->SoftWare_Version);puts("");
}

/******************************************************************************
 * Function:print changes
 * ***************************************************************************/
void _SIMs_Printer(void)
{
    MCU_TypeDef *mcu;
    uint8_t *actionTbl;
    uint8_t sim_no = 0;
    uint8_t i;
    for(i = 0;i < MCU_NUMS;i++)
    {
        mcu = &MCUs[i];

        while(mcu->SIM_StateTblW | mcu->SIM_APDUTblW | mcu->SIM_CheckErrW | mcu->SIM_InfoTblW | mcu->VersionW)
        {
            if(mcu->SIM_StateTblW)
            {
                actionTbl = &(mcu->SIM_StateTblW);
                sim_no = slot_parse(actionTbl);
                if(sim_no > 0)
                {
                    printf("SIM[%d] state changed:",((i*SIM_NUMS)+sim_no));
                    printf("%.2X\n",mcu->SIM[sim_no - 1].state);
                }
            }
            else if(mcu->SIM_APDUTblW)
            {
                actionTbl = &(mcu->SIM_APDUTblW);
                sim_no = slot_parse(actionTbl);
                if(sim_no > 0)
                {
                    printf("APDU ACK. from SIM[%d]:\n",((i*SIM_NUMS)+sim_no));
                    print_array_r(mcu->SIM[sim_no - 1].Rx_APDU,mcu->SIM[sim_no - 1].Rx_Length);
                    puts("");
                }
            }
            else if(mcu->SIM_CheckErrW)
            {
                actionTbl = &(mcu->SIM_CheckErrW);
                sim_no = slot_parse(actionTbl);
                if(sim_no > 0)
                {
                    printf("SIM[%d] transmit error.\n",((i*SIM_NUMS)+sim_no));
                }
            }
            else if(mcu->SIM_InfoTblW)
            {
                actionTbl = &(mcu->SIM_InfoTblW);
                sim_no = slot_parse(actionTbl);
                if(sim_no > 0)
                {
                    printf("SIM[%d] information changed:\n",((i*SIM_NUMS)+sim_no));
                    printf("ICCID:");
                    print_array(mcu->SIM[sim_no - 1].ICCID);
                    puts("");
                    printf("IMSI :");
                    print_array(mcu->SIM[sim_no - 1].IMSI);
                    puts("");
                    printf("AD:%.2X\n",mcu->SIM[sim_no - 1].AD);
                }
            }
            else if(mcu->VersionW)
            {
                actionTbl = &(mcu->VersionW);
                mcu->VersionW &= 0x00;
                printf("MCU[%d] hardware & software Version Changed:\n Hardware version:",i);
                print_array(mcu->HardWare_Version);puts("");
                printf("Software version:");
                print_array(mcu->HardWare_Version);puts("");
            }
            clear_flag(actionTbl,sim_no);
        }
    }
}

double time_use(struct timeval *start_time, struct timeval *end_time)
{
    double timeuse;
    return (timeuse = (double)1000000*(end_time->tv_sec - start_time->tv_sec) + end_time->tv_usec - start_time->tv_usec);
}
