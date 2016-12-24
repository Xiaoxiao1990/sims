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
static int block_length_check(DataType_TypeDef datatype,uint8_t data_len)
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

/**
 * @brief frame_parse
 * data frame from mcu's parse ,run in a depend thread
 * @param mcu
 * @return
 */
void * frame_parse(void *arg)
{
    uint16_t total_length,
            i = 0,
            blocks = 0,
            slot = 0,
            sublen = 0,
            k = 0;
    uint8_t step = 0, mcu_num = 0;

    DataType_TypeDef datatype;
    MCU_TypeDef *mcu = &MCUs[0];
    SPI_Buf_TypeDef *Rx = &(mcu->RxBuf);

    arg = arg;//clear complier warnning.

    for(;;){
        //check whether the RX buffer is busy.set full by transer, and flush in this function.
        switch(Rx->state){
            case SPI_BUF_STATE_READY:
            case SPI_BUF_STATE_PACKAGING:
            case SPI_BUF_STATE_EMPTY:
            case  SPI_BUF_STATE_TRANSMITING:{
                //Nothing to do. correct in spi transfer.
                goto PARSE_END;
            }break;           
            case SPI_BUF_STATE_FULL:{
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
                            total_length = (uint16_t)Rx->Buf[i++] << 8;
                            total_length += (uint16_t)Rx->Buf[i];
                            if(total_length < 2){//data length should be great than 2 bytes.
                                printf("Frame length is too short:[%d btyes].\n",total_length);
                                goto FRAME_END;
                            } else if(total_length > ARRAY_SIZE(Rx->Buf)) {
                                printf("Frame length is too long:[%d btyes].\n",total_length);
                                goto FRAME_END;
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
                                goto FRAME_END;
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
                                    mcu->SIM[slot-1].state = Rx->Buf[i];
                                    set_flag(&mcu->SIM_StateTblN,slot);
                                }break;
                                case APDU_CMD:{
                                //may be should check slot.
                                    for(k = 0;k < (sublen - 1);k++)mcu->SIM[slot-1].RX_APDU.APDU[k] = Rx->Buf[i++];
                                    mcu->SIM[slot-1].RX_APDU.length = k;
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
                }//Frame end
FRAME_END:
                //reset local variables.
                step = PARSE_FRAME_HEAD1;
                slot = 0;
                total_length = 0;
                sublen = 0;
                //Reset buffer
                SPI_Buf_init(Rx);
            }break;
            default:;
        }//parse end

PARSE_END:
        mcu_num++;
        if(mcu_num >= MCU_NUMS)mcu_num = 0;
        mcu = &MCUs[mcu_num];
        Rx = &(mcu->RxBuf);
    }

    logs(misc_log, "Frame parse thread abort by unkonw ways!\n");
    pthread_exit(NULL);
}

static void SIM_info_init(SIM_TypeDef *sim)
{
    sim->AD = 0x00;
    sim->state = SIM_STATE_NOSIM;
    flush_array(sim->ICCID);
    flush_array(sim->IMSI);
    flush_array(sim->RX_APDU.APDU);
    sim->RX_APDU.length = 0x00;
    flush_array(sim->Tx_APDU.APDU);
    sim->Tx_APDU.length = 0x00;
    sim->auth_step = AUTH_STAGE_DEFAULT;
}

void SIMs_Table_init(void)
{
    uint16_t i = 0,j = 0;
    for(i = 0;i < MCU_NUMS;i++)
    {
        //SIM information initial
        for(j = 0;j < SIM_NUMS;j++){
            SIM_info_init(&MCUs[i].SIM[j]);
        }
        //Hardware version initial
        MCUs[i].HardWare_Version[0] = 0x00;
        MCUs[i].HardWare_Version[1] = 0x00;
        MCUs[i].HardWare_Version[2] = 0x00;
        //Software version initial
        MCUs[i].SoftWare_Version[0] = 0x00;
        MCUs[i].SoftWare_Version[1] = 0x00;
        MCUs[i].SoftWare_Version[2] = 0x00;
        //RxBuf initial
        SPI_Buf_init(&MCUs[i].RxBuf);
        SPI_Buf_init(&MCUs[i].TxBuf);
        /*
        MCUs[i].RxBuf.block = 0x00;
        MCUs[i].RxBuf.Length = 0x00;
        MCUs[i].RxBuf.Buf_Len_Left = SPI_TRANSFER_MTU;
        MCUs[i].RxBuf.checksum = 0x00;
        MCUs[i].RxBuf.pre_sum = 0x00;
        MCUs[i].RxBuf.state = SPI_BUF_STATE_EMPTY;
        flush_array(MCUs[i].RxBuf.Buf);
        //TxBuf initial
        MCUs[i].TxBuf.block = 0x00;
        MCUs[i].TxBuf.Length = 0x00;
        MCUs[i].TxBuf.Buf_Len_Left = SPI_TRANSFER_MTU;
        MCUs[i].TxBuf.checksum = 0x00;
        MCUs[i].TxBuf.pre_sum = 0x00;
        MCUs[i].TxBuf.state = SPI_BUF_STATE_EMPTY;
        flush_array(MCUs[i].TxBuf.Buf);
        */

        //MCU events flags initial
        MCUs[i].SIM_APDUTblR = 0x00;
        MCUs[i].SIM_APDUTblN = 0x00;
        MCUs[i].SIM_CheckErrR = 0x00;
        MCUs[i].SIM_CheckErrN = 0x00;
        MCUs[i].SIM_InfoTblR = 0x00;
        MCUs[i].SIM_InfoTblN = 0x00;
        MCUs[i].SIM_StateTblR = 0x00;
        MCUs[i].SIM_StateTblN = 0x00;
        MCUs[i].VersionR = 0x00;
        MCUs[i].VersionN = 0x00;

#ifdef CODING_DEBUG_NO_PRINT
        printf("===================== MCU[%d] =====================\n",i+1);
        print_MCU(&MCUs[i]);
#endif
    }
}
