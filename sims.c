#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "sims.h"


MCU_TypeDef MCUs[MCU_NUMS];

pthread_t tid[4], tid_sim_affair, tid_timewheel;

/**
 * @brief upload_apdu
 * @param apdu
 * @param data
 * @return
 */
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

/**
 * @brief sim_read
 * @param sim_no
 * @param datatype
 * @return
 */
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
            //set time.
            if(MCUs[mcu_no].SIM[sim_offset].auth_step == AUTH_STAGE_DEFAULT){
                gettimeofday(&(MCUs[mcu_no].SIM[sim_offset].start), 0);
            }
            MCUs[mcu_no].SIM[sim_offset].auth_step++;
        }break;
        case TRANS_ERR:MCUs[mcu_no].SIM_CheckErrR |= flag;break;
        default:printf("Read argument error!\n");return -1;break;
    }

    return 0;
}

/**
 * @brief mcu_read
 * @param mcu_no
 * @param datatype
 * @return
 */
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


void sims_update(uint8_t mcu_num)
{
    MCU_TypeDef *mcu = &MCUs[mcu_num];
    SIM_TypeDef *sim;
    FILE *log_file;
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

            log_file = sim->log;
            printf("Recv. APDU ACK from SIM[%d]:\n", abs_sim_no);
            apdu = &(sim->RX_APDU);
            print_array_r(apdu->APDU,apdu->length);
            puts("");

            switch(sim->auth_step){
            case AUTH_STAGE_DEFAULT:break;
            case AUTH_STAGE_REQ1:{
                if(apdu->length == 1){//0x88
                    if(apdu->APDU[0] != 0x88){
                        printf("Auth. ACK1 error.\n");
                        logs(log_file, "SIM[%d] Auth ACK1 error. Tx_APDU[%d]:\n", abs_sim_no, apdu->length);
                        fprint_array_r(log_file, apdu->APDU, apdu->length);
                        sim->auth_step = AUTH_STAGE_DEFAULT;
                        break;
                    }
                } else if(apdu->length == 2){//0x60 0x88
                    if((apdu->APDU[0] | apdu->APDU[1]) != 0xE8){
                        printf("Auth. ACK1 error.\n");
                        logs(log_file, "SIM[%d] Auth ACK1 error. Tx_APDU[%d]:\n", abs_sim_no, apdu->length);
                        fprint_array_r(log_file, apdu->APDU, apdu->length);
                        sim->auth_step = AUTH_STAGE_DEFAULT;
                        break;
                    }
                } else {
                    printf("Auth. ACK1 length error.\n");
                    logs(log_file, "SIM[%d] Auth ACK1 length error. Tx_APDU[%d]:\n", abs_sim_no, apdu->length);
                    fprint_array_r(log_file, apdu->APDU, apdu->length);
                    sim->auth_step = AUTH_STAGE_DEFAULT;
                    break;
                }

                sim->auth_step = AUTH_STAGE_ACK1;
                sim_read(abs_sim_no, APDU_CMD, &apdu2);
            }break;
            case AUTH_STAGE_ACK1:break;
            case AUTH_STAGE_REQ2:{
                if(apdu->length == 2){//0x98 0x62
                    if((apdu->APDU[0]|apdu->APDU[1]) != 0xFA){
                        printf("Auth. ACK2 error.\n");
                        logs(log_file, "SIM[%d] Auth. ACK2 error. Tx_APDU[%d]:", abs_sim_no, apdu->length);
                        fprint_array_r(log_file, apdu->APDU, apdu->length);
                        sim->auth_step = AUTH_STAGE_DEFAULT;
                        break;
                    }
                } else if (apdu->length == 3){//0x60 0x98 0x62
                    if((apdu->APDU[1] | apdu->APDU[2]) == 0xFA){
                        if(apdu->APDU[0] != 0x60){//auth. failed
                            printf("Auth. ACK2 error.\n");
                            logs(log_file, "SIM[%d] Auth. ACK2 error. Tx_APDU[%d]:", abs_sim_no, apdu->length);
                            fprint_array_r(log_file, apdu->APDU, apdu->length);
                            sim->auth_step = AUTH_STAGE_DEFAULT;
                            break;
                        }
                    } else {//auth. failed
                        printf("Auth. ACK2 error.\n");
                        logs(log_file, "SIM[%d] Auth. ACK2 error. Tx_APDU[%d]:", abs_sim_no, apdu->length);
                        fprint_array_r(log_file, apdu->APDU, apdu->length);
                        sim->auth_step = AUTH_STAGE_DEFAULT;
                        break;
                    }
                } else {//auth. failed
                    printf("Auth. ACK2 length error.\n");
                    logs(log_file, "SIM[%d] Auth. ACK2 length error. Tx_APDU[%d]:", abs_sim_no, apdu->length);
                    fprint_array_r(log_file, apdu->APDU, apdu->length);
                    sim->auth_step = AUTH_STAGE_DEFAULT;
                    break;
                }

                sim->auth_step = AUTH_STAGE_DEFAULT;
                gettimeofday(&(sim->end), 0);
                sim->timeuse = time_use(&(sim->start), &(sim->end));//us
                printf("SIM[%d] Auth. time use:%lf(us)\n", abs_sim_no, sim->timeuse);
                logs(log_file, "SIM[%d] Auth. time use:%lf(us)\n", abs_sim_no, sim->timeuse);
            }break;
            case AUTH_STAGE_ACK2:
            default:sim->auth_step = AUTH_STAGE_DEFAULT;;
            }

        } else if(mcu->SIM_CheckErrN) {
            actionTbl = &(mcu->SIM_CheckErrN);
            sim_no = slot_parse(actionTbl) - 1;//offset 1
            //sim = &mcu->SIM[sim_no];
            abs_sim_no += sim_no;

            printf("SIM[%d] transmit error.\n", abs_sim_no);
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

uint8_t authentication(SIM_TypeDef *sim)
{
    switch(sim->auth_step){
    case AUTH_STAGE_DEFAULT:break;
    case AUTH_STAGE_REQ1:break;
    case AUTH_STAGE_ACK1:break;
    case AUTH_STAGE_REQ2:break;
    case AUTH_STAGE_ACK2:break;
    default:;
    }
}

void * sim_affair(void *arg)
{
    MCU_TypeDef *mcu;
    SIM_TypeDef *sim;
//    DataType_TypeDef action;
//    FILE *log_file;
//    uint8_t *actionTbl;
    uint8_t i, j, auth_try;

    arg = arg;

    thread_sleep(5);
    while(1){

        for(i = 0;i < MCU_NUMS;i++){
            mcu = &MCUs[i];
            //check whether something new from sims
            // <==== SIMs
            sims_update(i);

            //check whether something new send to sim
            // ====> SIMs
            for(j = 0;j < SIM_NUMS;j++){
                sim = &mcu->SIM[j];

//                switch(sim->state){
//                case SIM_STATE_NORMAL:
//                    //if(authentication(sim) != AUTH_STAGE_ACK2)
//                    break;
//                case SIM_STATE_NOCARD:
//                case SIM_STATE_NOICCD:
//                case SIM_STATE_NOIMSI:
//                case SIM_STATE_NO_ATR:
//                case SIM_STATE_STOPED:
//                    //All those status should be report to BSS.
//                    //thus, no command come in those case, if not, attempt to reset this SIM.
//                    //sim_read((i*SIM_NUMS + j + 1), RESET_SIM, NULL);
//                default:break;
//                }

                //APDU time out check
                if(sim->apdu_enable){
                    if(sim->apdu_timeout == 0){
                        sim->apdu_enable = DISABLE;
                        sim->apdu_timeout = APDU_TIME_OUT;
                        sim->auth_step = AUTH_STAGE_DEFAULT;
                        logs(sim->log,"APDU command time out. APDU[%d]:\n",sim->Tx_APDU.length);
                        fprint_array_r(sim->log, sim->Tx_APDU.APDU, sim->Tx_APDU.length);
                    }
                }
            }
            //MCU offline check.
            if(mcu->online == ON_LINE){
                //MCU_Init(mcu);
                if(mcu->time_out == 0){
                    mcu->online = OFF_LINE;
                    mcu->time_out = MCU_TIME_OUT;
                    logs(misc_log, "MCU[%d] off line.\n", i);
                    printf("MCU[%d] off line.\n", i);
                    MCU_Init(mcu);
                }
            }
        }

//        action = READ_STATE;
//        mcu_read(0, action, NULL);
//        mcu_read(1, action, NULL);
//        mcu_read(2, action, NULL);
//        mcu_read(105, action, NULL);
//        mcu_read(106, action, NULL);
//        mcu_read(107, action, NULL);
//        thread_sleep(5);

//        action = READ_SWHW;
//        mcu_read(0, action, NULL);
//        mcu_read(1, action, NULL);
//        mcu_read(2, action, NULL);
//        mcu_read(105, action, NULL);
//        mcu_read(106, action, NULL);
//        mcu_read(107, action, NULL);
//        thread_sleep(1);

//        action = READ_INFO;
//        mcu_read(0, action, NULL);
//        mcu_read(1, action, NULL);
//        mcu_read(2, action, NULL);
//        mcu_read(105, action, NULL);
//        mcu_read(106, action, NULL);
//        mcu_read(107, action, NULL);
//        thread_sleep(1);

//        action = STOP_SIM;
//        mcu_read(0, action, NULL);
//        mcu_read(1, action, NULL);
//        mcu_read(2, action, NULL);
//        mcu_read(105, action, NULL);
//        mcu_read(106, action, NULL);
//        mcu_read(107, action, NULL);
//        thread_sleep(1);

//        action = RESET_SIM;
//        mcu_read(0, action, NULL);
//        mcu_read(1, action, NULL);
//        mcu_read(2, action, NULL);
//        mcu_read(105, action, NULL);
//        mcu_read(106, action, NULL);
//        mcu_read(107, action, NULL);
//        thread_sleep(5);

//        action = APDU_CMD;
//        mcu_read(0, action, &apdu1);
//        mcu_read(1, action, &apdu1);
//        mcu_read(2, action, &apdu1);
//        mcu_read(105, action, &apdu1);
//        mcu_read(106, action, &apdu1);
//        mcu_read(107, action, &apdu1);
//        thread_sleep(5);

    }
}


void *timerwheel(void *arg)
{
    uint8_t mcu,sim;
    uint8_t logautosave_time = 0;
    SIM_TypeDef SIM;

    arg = arg;

    while(1){
        thread_sleep(1);

        //time ticker
        for(mcu = 0;mcu < MCU_NUMS;mcu++){
            if(MCUs[mcu].online == ON_LINE){
                if(MCUs[mcu].time_out > 0)MCUs[mcu].time_out--;
            }
            mcu_read(mcu, READ_STATE, NULL);
        }
        //auto save log file per min.
        if((logautosave_time++) >= 60){
            logautosave_time = 0;
            for(mcu = 0;mcu < MCU_NUMS;mcu++){
                for(sim = 0;sim < SIM_NUMS;sim++){
                    SIM = MCUs[mcu].SIM[sim];
                    fflush(SIM.log);
                }
            }
             fflush(misc_log);
        }
    }
    pthread_exit(NULL);
}

void break_out(int signo)
{
    int ret = signo;
    ret = close_logs();
    _exit(ret);
}

int main(int argc, char *argv[])
{
    argc = argc;
    argv = argv;

    //for debug
    signal(SIGINT, &break_out);
    //end

    //SIM VMAP initialization
    printf("Initial visual SIM maps...\n");
    SIMs_Table_init();
    printf("OK!\n");

    //SPI initialization
    printf("Initial Log...\n");
    if(0 != Log_Init()){
        printf("\nFailed to initialize Log!\n");
        return -1;
    }
    printf("OK!\n");

    //SPI initialization
    printf("Initial SPI & Arbitrator...\n");
    if(0 != SPI_Dev_Init()){
        printf("\nFailed to initialize SPI device !\n");
        return -1;
    }
    printf("OK!\n");

    //data transfer
    printf("Start transmitting[spi0] ... \n");
    if((pthread_create(&tid[1], NULL, &spi0, NULL)) != 0){
        perror("Create transfer thread error");
    }
    pthread_detach(tid[1]);

    printf("Start transmitting[spi2] ... \n");
    if((pthread_create(&tid[2], NULL, &spi2, NULL)) != 0){
        perror("Create transfer thread error");
    }
    pthread_detach(tid[2]);

    //time wheel
    printf("Start sim affairs ... \n");
    if((pthread_create(&tid_sim_affair, NULL, &sim_affair, NULL)) != 0){
        perror("Create time wheel thread error");
    }
    pthread_detach(tid_sim_affair);

    //time wheel
    printf("Start time wheel ... \n");
    if((pthread_create(&tid_timewheel, NULL, &timerwheel, NULL)) != 0){
        perror("Create time wheel thread error");
    }
    pthread_detach(tid_timewheel);

    printf("I/O message: \n");
    int i;
    for(;;){//sim stuff
        //SIMs_Printer();
        thread_sleep(3);
       //for(i = 0;i < MCU_NUMS;i++)sims_update(i);
    }

    if(0 != close_logs()){
        return -1;
    }

    return 0;
}
