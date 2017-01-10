/***************************************************
 *  File:sim.c
 *  Date:2016-12-5
 *  Description:message parse and package,
 *  SIMS visual mapping...
 * ************************************************/
#include <stdio.h>
#include <pthread.h>
#include <string.h>

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
    //log file always be opened in memory.initialized in log_init.
    //sim->log = NULL;
    pthread_mutex_init(&sim->lock, NULL);
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
    mcu->state = OFF_LINE;
    mcu->heartbeat.time_out = MCU_TIME_OUT;
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

static int upload_apdu(APDU_BUF_TypeDef *apdu, APDU_BUF_TypeDef *data)
{
    if(data->length > SIM_APDU_LENGTH){
        printf("APDU commands length[%d bytes] is too long than the APDU buffer maximum[%d bytes] length.\n",data->length,SIM_APDU_LENGTH);
        return  -1;
    }

    memcpy(apdu->APDU, data->APDU,data->length);
    apdu->length = data->length;
    return 0;
}

int sim_read(uint16_t sim_abs, DataType_TypeDef datatype, void *data_addr){
    uint8_t mcu_no, sim_offset, flag;
    APDU_BUF_TypeDef *apdu;
    APDU_BUF_TypeDef *data;

    if((0 == sim_abs) || (sim_abs > MAX_SIM_NUMS)){
        printf("SIM NO. range[1~%d].\n", (SIM_NUMS * MCU_NUMS));
        return -1;
    }

    mcu_no = (sim_abs - 1)/SIM_NUMS;
    sim_offset = ((sim_abs - 1)%SIM_NUMS);

    flag = (0x01 << sim_offset);//flag clear by frame_package()

    switch(datatype){
        case DUMMY_READ:break;
        case READ_SWHW:MCUs[mcu_no].VersionR = 0x1F;break;
        case STOP_SIM:MCUs[mcu_no].SIM_StopTbl |= flag;break;
        case RESET_SIM:MCUs[mcu_no].SIM_ResetTbl |= flag;break;
        case READ_INFO:MCUs[mcu_no].SIM_InfoTblR |= flag;break;
        case READ_STATE:MCUs[mcu_no].SIM_StateTblR |= flag;break;
        case APDU_CMD:{
            apdu = &(MCUs[mcu_no].SIM[sim_offset].Tx_APDU);
            data = data_addr;
            if((upload_apdu(apdu, data)) != 0){
                printf("SIM[%d] upload apdu data error.\n", sim_abs);
                break;
            }
            MCUs[mcu_no].SIM_APDUTblR |= flag;
        }break;
        case TRANS_ERR:MCUs[mcu_no].SIM_CheckErrR |= flag;break;
        default:printf("Read argument error!\n");return -1;break;
    }

    return 0;
}

int mcu_read(uint8_t mcu_no, DataType_TypeDef datatype, void *arg){
    uint16_t i, sim_base;
    if(mcu_no >= MCU_NUMS){
        printf("MCU NO.[0~%d]", MCU_NUMS);
        return -1;
    }

    sim_base = (uint16_t)mcu_no*SIM_NUMS;
    for(i = 1;i <= SIM_NUMS;i++){
        if(sim_read((sim_base + i), datatype , arg) != 0){
            printf("Actions error:SIM[%d],Operat type:[%d].\n",sim_base+i, datatype);
        }
    }

    return 0;
}

APDU_BUF_TypeDef apdu1 = {
    {0x00,0x88,0x00,0x81,0x22},
    5
};

APDU_BUF_TypeDef apdu2 = {
    {
        0x10,0xA6,0x75,0x25,0xAE,0x62,0x13,0x74,0x98,0x06,0x8F,0x5D,0x55,0x97,0x4E,0xD7,0x94,
        0x10,0x8A,0x87,0xE1,0x50,0xF2,0xB0,0x82,0x34,0xF1,0xA3,0x49,0x9D,0x91,0xA3,0x8F,0xAD
    },
   34
};

static void sims_update(uint8_t mcu_num)
{
    MCU_TypeDef *mcu = &MCUs[mcu_num];
    SIM_TypeDef *sim;
    APDU_BUF_TypeDef *apdu;
    uint8_t *actionTbl, sim_no;
    uint16_t abs_sim_no = (mcu_num * SIM_NUMS) + 1;

    while(mcu->SIM_StateTblN | mcu->SIM_APDUTblN | mcu->SIM_InfoTblN | mcu->SIM_CheckErrN | mcu->VersionN){
        if(mcu->SIM_StateTblN){
            actionTbl = &(mcu->SIM_StateTblN);
            sim_no = slot_parse(actionTbl) - 1;//offset 1
            sim = &mcu->SIM[sim_no];
            abs_sim_no += sim_no;
            printf("SIM[%d]'s state changed:", abs_sim_no);
            printf("%.2X\n",sim->state);

        } else if(mcu->SIM_APDUTblN) {
            actionTbl = &(mcu->SIM_APDUTblN);
            sim_no = slot_parse(actionTbl) - 1;//offset 1
            sim = &mcu->SIM[sim_no];
            abs_sim_no += sim_no;

            printf("Recv. APDU ACK from SIM[%d]:\n", abs_sim_no);
            apdu = &(sim->RX_APDU);
            print_array_r(apdu->APDU,apdu->length);
            puts("");
        } else if(mcu->SIM_CheckErrN) {
            actionTbl = &(mcu->SIM_CheckErrN);
            sim_no = slot_parse(actionTbl) - 1;//offset 1
            //sim = &mcu->SIM[sim_no];
            abs_sim_no += sim_no;

            printf("[%s:%d]SIM[%d] transmit error.\n", __FILE__, __LINE__, abs_sim_no);
        } else if(mcu->SIM_InfoTblN) {
            actionTbl = &(mcu->SIM_InfoTblN);
            sim_no = slot_parse(actionTbl) - 1;//offset 1
            sim = &mcu->SIM[sim_no];
            abs_sim_no += sim_no;

            printf("SIM[%d]'s information changed:\n", abs_sim_no);
            printf("ICCID:");
            print_array(sim->ICCID);
            puts("");
            printf("IMSI :");
            print_array(sim->IMSI);
            puts("");
            printf("AD:%.2X\n",sim->AD);
        } else if(mcu->VersionN) {
            actionTbl = &(mcu->VersionN);
            mcu->VersionN &= 0x00;
            printf("MCU[%d]'s hardware & software Version Changed:\nHardware version:", mcu_num);
            print_array(mcu->HardWare_Version);puts("");
            printf("Software version:");
            print_array(mcu->HardWare_Version);puts("");
            continue;
        }
//        printf("actionTblA:");bin_echo(*actionTbl);puts("");
        clear_flag(actionTbl, sim_no + 1);
//        printf("actionTblB:");bin_echo(*actionTbl);puts("");
        //thread_sleep(1);
    }
}
