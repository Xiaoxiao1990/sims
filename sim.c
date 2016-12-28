/***************************************************
 *  File:sim.c
 *  Date:2016-12-5
 *  Description:message parse and package,
 *  SIMS visual mapping...
 * ************************************************/
#include <stdio.h>
#include <pthread.h>

#include "types.h"
#include "functions.h"
#include "log.h"
/**
 * @brief block_length_check
 * @param datatype
 * @param data_len
 * @return
 */
int block_length_check(DataType_TypeDef datatype,uint8_t data_len)
{
    switch(datatype)
    {
        case DUMMY_READ:return 0;break;
        case READ_SWHW:return 3;break;
        case STOP_SIM:return 3;break;
        case RESET_SIM:return 3;break;
        case READ_INFO:return 3;break;
        case READ_STATE:return 3;break;
        case APDU_CMD:return data_len + 3;break;
        case TRANS_ERR:return 3;break;
        default:;
    }
    return SPI_TRANSFER_MTU + SPI_TRANSFER_MTU;//wrong type can't allocate.
}

#define DATA_FRAME_HEAD1                                (uint8_t)0xA5
#define DATA_FRAME_HEAD2                                (uint8_t)0xA5

/**
 * @brief frame_package
 * @param mcu
 * @param mcu_num
 * @return
 */

/*
void * frame_package(void *arg)
{//Running in an independent thread.
    uint8_t i;
    uint8_t sim_no = 0;
    uint8_t *data, apdu_len;
    uint16_t block_len;
    uint8_t *actionTbl;
    uint16_t mcu_num = 0;
    uint8_t real_state;

    DataType_TypeDef datatype;
    MCU_TypeDef *mcu = &MCUs[0];
    SPI_Buf_TypeDef *Tx = &(mcu->TxBuf);

    arg = arg;

    for(;;){
        //Check Tx buffer state at first
        pthread_mutex_lock(&tmutex_mcu_buf_access);
        switch(Tx->state){
            case SPI_BUF_STATE_PACKAGING:
                printf("Unexpected SPI buffer state[%d] of MCU[%d]. Attempt to reset it.\n", Tx->state, mcu_num);
                logs(misc_log, "Unexpected SPI buffer state[%d] of MCU[%d]. Attempt to reset it.\n", Tx->state, mcu_num);
                SPI_Buf_init(Tx);
            case SPI_BUF_STATE_TRANSMITING:
            case SPI_BUF_STATE_FULL:{
                //do nothing here.just switch to next Tx buffer.
                pthread_mutex_unlock(&tmutex_mcu_buf_access);
                goto NEXT_MCU;
            }break;
            case SPI_BUF_STATE_EMPTY:
            case SPI_BUF_STATE_READY:{
                real_state = Tx->state;
                Tx->state = SPI_BUF_STATE_PACKAGING;
            }break;
            default:;
        }
        pthread_mutex_unlock(&tmutex_mcu_buf_access);
        //printf("Package MCU[%d].\n",mcu_num);
        //Check whether have something to send to MCU.
        while(mcu->SIM_StateTblR | mcu->SIM_ResetTbl | mcu->SIM_StopTbl | mcu->SIM_APDUTblR | mcu->SIM_InfoTblR | mcu->VersionR | mcu->SIM_CheckErrR){
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
            if(real_state == SPI_BUF_STATE_EMPTY){//The first time come to this buffer.
                if((Tx->Length + block_len + 5) >= SPI_TRANSFER_MTU){
                    //Illegal blocks, data length is too long.skip this block,clear the CMD flag.
                    printf("Illegal data block:Data block length[%d] great than Tx buffer maximum length[%d].\n", block_len, SPI_TRANSFER_MTU);
                    logs(misc_log, "Illegal data block:Data block length[%d] great than Tx buffer maximum length[%d].\n", block_len, SPI_TRANSFER_MTU);
                    clear_flag(actionTbl, sim_no);
                    continue;
                }
            }
            else if(real_state == SPI_BUF_STATE_READY){//There is something packed in this buffer.
                if((Tx->Length + block_len) >= SPI_TRANSFER_MTU){
                    //There is no space to load this block,wait for next package.
                    printf("MCU[%d]->Tx buffer is no space to allocate data block, try again by next time.\n", mcu_num);
                    logs(misc_log, "MCU[%d]->Tx buffer is no space to allocate data block, try again by next time.\n", mcu_num);
                    Tx->state = SPI_BUF_STATE_FULL;
                    goto NEXT_MCU;
                }
            } else {
                logs(misc_log, "Unexpected SPI Tx buffer state[%d] of MCU[%d]. Attempt to reset it.\n", real_state, mcu_num);
                SPI_Buf_init(Tx);
                goto NEXT_MCU;
            }
            //state:EMPTY,READY
            switch(real_state){
                case SPI_BUF_STATE_EMPTY:{
                    //Frame Head;
                    Tx->Buf[0] = DATA_FRAME_HEAD1;
                    Tx->Buf[1] = DATA_FRAME_HEAD2;

                    //Locate to block
                    //|0   1|2       3|4    |5      |   |       |
                    //|A5 A5|lenH lenL|block|data[0]|...|data[n]|checksum|
                    Tx->Length = 5;//Length is Buf[] position.
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

                    real_state = SPI_BUF_STATE_READY;
                    //clear flag
                    clear_flag(actionTbl,sim_no);
                }break;
                case SPI_BUF_STATE_FULL://Never been here.
                case SPI_BUF_STATE_PACKAGING://Never been here.
                case SPI_BUF_STATE_TRANSMITING://Never been here.
                default:
                    printf("MCU[%d]'s Tx buffer state has been changed by unknow ways!\n", mcu_num);
                    logs(misc_log, "Unexpected SPI Tx buffer state[%d] of MCU[%d]. Attempt to reset it.\n", real_state, mcu_num);
                    SPI_Buf_init(Tx);
                    goto NEXT_MCU;
              break;
            }
            //printf("MCU[%d]Tx buffer:\n", mcu_num);
            //print_array(Tx->Buf);
        }

        Tx->state = real_state;
NEXT_MCU:
        mcu_num++;
        if(mcu_num >= MCU_NUMS)mcu_num = 0;
        mcu = &MCUs[mcu_num];
        Tx = &(mcu->TxBuf);
    }
    printf("Package frame error!\n");
    pthread_exit(NULL);
}
*/

static void SIM_info_init(SIM_TypeDef *sim)
{
    sim->AD = 0x00;
    sim->state = SIM_STATE_NOCARD;
    flush_array(sim->ICCID);
    flush_array(sim->IMSI);
    flush_array(sim->RX_APDU.APDU);
    sim->RX_APDU.length = 0x00;
    flush_array(sim->Tx_APDU.APDU);
    sim->Tx_APDU.length = 0x00;
    sim->auth_step = AUTH_STAGE_DEFAULT;
    sim->apdu_enable = DISABLE;
    sim->apdu_timeout = APDU_TIME_OUT;
    sim->log = NULL;
}

void MCU_Init(MCU_TypeDef *mcu)
{
    uint8_t j;
    //SIM information initial
    for(j = 0;j < SIM_NUMS;j++){
        SIM_info_init(&mcu->SIM[j]);
    }
    //Hardware version initial
    mcu->HardWare_Version[0] = 0x00;
    mcu->HardWare_Version[1] = 0x00;
    mcu->HardWare_Version[2] = 0x00;
    //Software version initial
    mcu->SoftWare_Version[0] = 0x00;
    mcu->SoftWare_Version[1] = 0x00;
    mcu->SoftWare_Version[2] = 0x00;
    //RxBuf initial
    SPI_Buf_init(&mcu->RxBuf);
    SPI_Buf_init(&mcu->TxBuf);
    //MCU events flags initial
    mcu->SIM_APDUTblR = 0x00;
    mcu->SIM_APDUTblN = 0x00;
    mcu->SIM_CheckErrR = 0x00;
    mcu->SIM_CheckErrN = 0x00;
    mcu->SIM_InfoTblR = 0x00;
    mcu->SIM_InfoTblN = 0x00;
    mcu->SIM_StateTblR = 0x00;
    mcu->SIM_StateTblN = 0x00;
    mcu->VersionR = 0x00;
    mcu->VersionN = 0x00;
    mcu->heartbeat = MCU_TIME_OUT;
}

void SIMs_Table_init(void)
{
    uint16_t i = 0;
    MCU_TypeDef *mcu;
    for(i = 0;i < MCU_NUMS;i++)
    {
        mcu = &MCUs[i];
        MCU_Init(mcu);

#ifdef CODING_DEBUG_NO_PRINT
        printf("===================== MCU[%d] =====================\n",i+1);
        print_MCU(&MCUs[i]);
#endif
    }
}
