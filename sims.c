#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "sims.h"


MCU_TypeDef MCUs[MCU_NUMS];

pthread_mutex_t tmutex_mcu_buf_access = PTHREAD_MUTEX_INITIALIZER;

pthread_t tid[3], tid_sim_affair;

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

void thread_sleep(uint16_t sec)
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

void *sim_affair(void *arg)
{
    DataType_TypeDef action;
    arg = arg;
    int times;
    uint8_t mcu,sim;
    thread_sleep(8);

    for(;;){

        action = RESET_SIM;
        mcu_read(0, action, NULL);
        mcu_read(1, action, NULL);
        mcu_read(2, action, NULL);
        mcu_read(105, action, NULL);
        mcu_read(106, action, NULL);
        mcu_read(107, action, NULL);
        thread_sleep(3);

        action = READ_SWHW;
        mcu_read(0, action, NULL);
        mcu_read(1, action, NULL);
        mcu_read(2, action, NULL);
        mcu_read(105, action, NULL);
        mcu_read(106, action, NULL);
        mcu_read(107, action, NULL);
        //thread_sleep(0);

        action = READ_INFO;
        mcu_read(0, action, NULL);
        mcu_read(1, action, NULL);
        mcu_read(2, action, NULL);
        mcu_read(105, action, NULL);
        mcu_read(106, action, NULL);
        mcu_read(107, action, NULL);
        //thread_sleep(2);

        action = STOP_SIM;
        mcu_read(0, action, NULL);
        mcu_read(1, action, NULL);
        mcu_read(2, action, NULL);
        mcu_read(105, action, NULL);
        mcu_read(106, action, NULL);
        mcu_read(107, action, NULL);
        thread_sleep(2);

        action = READ_STATE;
        mcu_read(0, action, NULL);
        mcu_read(1, action, NULL);
        mcu_read(2, action, NULL);
        mcu_read(105, action, NULL);
        mcu_read(106, action, NULL);
        mcu_read(107, action, NULL);
        //thread_sleep(2);

        action = RESET_SIM;
        mcu_read(0, action, NULL);
        mcu_read(1, action, NULL);
        mcu_read(2, action, NULL);
        mcu_read(105, action, NULL);
        mcu_read(106, action, NULL);
        mcu_read(107, action, NULL);
        thread_sleep(3);

        action = APDU_CMD;
        mcu_read(0, action, &apdu1);
        mcu_read(1, action, &apdu1);
        mcu_read(2, action, &apdu1);
        mcu_read(105, action, &apdu1);
        mcu_read(106, action, &apdu1);
        mcu_read(107, action, &apdu1);

        thread_sleep(5);
        times++;
        if((times%10) == 0){
            for(mcu = 0;mcu < MCU_NUMS;mcu++){
                for(sim = 0;sim < SIM_NUMS;sim++){
                    fflush(MCUs[mcu].SIM[sim].log);
                }
            }
             fflush(misc_log);
        }
        if(times == 100){
             times = 0;
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

    pthread_mutex_init(&tmutex_mcu_buf_access, NULL);
    //data frame parse thread
    printf("Start parsing ... \n");
   if((pthread_create(&tid[0], NULL, &frame_parse, NULL)) != 0){
        perror("Create parse thread error");
    }
    pthread_detach(tid[0]);

    //data transfer
    printf("Start transmitting ... \n");
    if((pthread_create(&tid[1], NULL, &transfer, NULL)) != 0){
        perror("Create transfer thread error");
    }
    pthread_detach(tid[1]);

    //data frame package
    printf("Start packaging ... \n");
    if((pthread_create(&tid[2], NULL, &frame_package, NULL)) != 0){
        perror("Create frame_package thread error");
    }
    pthread_detach(tid[2]);

    //sim affairs
    printf("sim affairs ... \n");
    if((pthread_create(&tid_sim_affair, NULL, &sim_affair, NULL)) != 0){
        perror("Create frame_package thread error");
    }
    pthread_detach(tid_sim_affair);

    printf("I/O message: \n");
    for(;;){//sim stuff
        //how to use OR manager a sim card ?
        //debug info
        SIMs_Printer();
    }
    pthread_mutex_destroy(&tmutex_mcu_buf_access);

    if(0 != close_logs()){
        return -1;
    }

    return 0;
}
