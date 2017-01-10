#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "sims.h"


MCU_TypeDef MCUs[MCU_NUMS];

pthread_t tid[4], tid_sim_affair, tid_timewheel;

void * sim_affair(void *arg)
{
    MCU_TypeDef *mcu;
//    SIM_TypeDef *sim;
//    DataType_TypeDef action;
//    FILE *log_file;
//    uint8_t *actionTbl;
    uint8_t i, j;

    arg = arg;

    thread_sleep(5);
    while(1){

        for(i = 0;i < MCU_NUMS;i++){
            mcu = &MCUs[i];
            //check whether something new from sims
            // <==== SIMs
            //sims_update(i);

            //check whether something new send to sim
            // ====> SIMs
            for(j = 0;j < SIM_NUMS;j++){
//                sim = &mcu->SIM[j];

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
//                if(sim->apdu_enable){
//                    if(sim->apdu_timeout == 0){
//                        sim->apdu_enable = DISABLE;
//                        sim->apdu_timeout = APDU_TIME_OUT;
//                        sim->auth_step = AUTH_STAGE_DEFAULT;
//                        logs(sim->log,"APDU command time out. APDU[%d]:\n",sim->Tx_APDU.length);
//                        fprint_array_r(sim->log, sim->Tx_APDU.APDU, sim->Tx_APDU.length);
//                    }
//                }
            }
            //MCU offline check.
            if(mcu->state == ON_LINE){
                //MCU_Init(mcu);
                if(mcu->heartbeat.time_out == 0){
                    mcu->state = OFF_LINE;
                    mcu->heartbeat.time_out = MCU_TIME_OUT;
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

extern int mcu_read(uint8_t, DataType_TypeDef, void *);
void *timerwheel(void *arg)
{
    uint8_t mcu,sim;
    uint8_t logautosave_time = 0;
    SIM_TypeDef SIM;
    Client_TypeDef *ct;

    arg = arg;

    while(1){
        thread_sleep(1);

        //client time out
        ct = current_client;
        while(ct != NULL){
            if(ct->time_out > 0)
            {
                ct->time_out--;
                printf("[%s:%d]Client[%d]:Time out after (%d) second(s).\n", inet_ntoa(ct->ip), ct->port, ct->num, ct->time_out);
            }
            ct = ct->prev;
            if(ct == current_client)break;
        }
        //time ticker
        for(mcu = 0;mcu < MCU_NUMS;mcu++){
            if(MCUs[mcu].state == ON_LINE){
                if(MCUs[mcu].heartbeat.time_out > 0)MCUs[mcu].heartbeat.time_out--;
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

    DC_Power_OFF();
    sleep(1);
    printf("Power ON card board...\n");
    DC_Power_ON();
    sleep(2);
    printf("Reset card boards...\n");
    Card_Board_Reset_OFF();
    sleep(1);
    Card_Board_Reset_ON();
    sleep(2);

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

    for(;;){//sim stuff
        //VSIM_Management();
        SIMs_Printer();
    }

    if(0 != close_logs()){
        return -1;
    }

    return 0;
}
