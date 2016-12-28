/*****************************
*SIMs typedef
*****************************/

#ifndef _SIMS_TYPE_H_
#define _SIMS_TYPE_H_

#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>

/******************* sims.c ***********************/
#define ENABLE                                          (uint8_t)1
#define DISABLE                                         (uint8_t)0
#define APDU_TIME_OUT                                   (uint8_t)5
#define MCU_TIME_OUT                                    (uint8_t)5
/******************* spi.c ************************/
#define SPI_TRANSFER_MTU                                (uint16_t)300
#define DATA_FRAME_HEAD1                                (uint8_t)0xA5
#define DATA_FRAME_HEAD2                                (uint8_t)0xA5
//SPI Buffer state
//#define SPI_BUF_STATE_EMPTY                             (uint8_t)0x00
//#define SPI_BUF_STATE_PACKAGING                         (uint8_t)0x01
//#define SPI_BUF_STATE_READY                             (uint8_t)0x02
//#define SPI_BUF_STATE_FULL                              (uint8_t)0x03
//#define SPI_BUF_STATE_TRANSMITING                       (uint8_t)0x04
//SIM_BIT & No.
#define SIM_NO_1_BIT                                    (uint8_t)0x01
#define SIM_NO_2_BIT                                    (uint8_t)0x02
#define SIM_NO_3_BIT                                    (uint8_t)0x04
#define SIM_NO_4_BIT                                    (uint8_t)0x08
#define SIM_NO_5_BIT                                    (uint8_t)0x10
#define SIM_NO_ALL_BIT                                  (uint8_t)0x20
//
#define SIM_NO_ALL                                      (uint8_t)0x00
#define SIM_NO_1                                        (uint8_t)0x01
#define SIM_NO_2                                        (uint8_t)0x02
#define SIM_NO_3                                        (uint8_t)0x03
#define SIM_NO_4                                        (uint8_t)0x04
#define SIM_NO_5                                        (uint8_t)0x05

//SIM state
#define SIM_STATE_NORMAL                                (uint8_t)0xAA
#define SIM_STATE_NOCARD                                (uint8_t)0x00
#define SIM_STATE_STOPED                                (uint8_t)0x55
#define SIM_STATE_NO_ATR                                (uint8_t)0xF0
#define SIM_STATE_NOIMSI                                (uint8_t)0xF1
#define SIM_STATE_NOICCD                                (uint8_t)0xF2


//typedef enum{
//    NORMAL = 0xAA,

//}SIM_STATE_TypeDef;

typedef struct SPI_Buf{
    uint8_t Buf[SPI_TRANSFER_MTU];
    uint16_t Length;
    uint8_t block;
    uint8_t pre_sum;
    uint8_t checksum;
}SPI_Buf_TypeDef;

typedef enum{
    DUMMY_READ = 0,
    READ_SWHW,
    STOP_SIM,
    RESET_SIM,
    READ_INFO,
    READ_STATE,
    APDU_CMD,
    TRANS_ERR
}DataType_TypeDef;
/******************* SIM ************************/
#define MCU_NUMS                                (uint8_t)108
#define SIM_NUMS                                (uint8_t)0x05
#define MAX_SIM_NUMS                            (SIM_NUMS*MCU_NUMS)

#define SIM_APDU_LENGTH                         (uint8_t)40
#define ICCID_LENGTH                            (uint8_t)0x0A
#define IMSI_LENGTH                             (uint8_t)0x08

#define AUTH_STAGE_DEFAULT                      (uint8_t)0x00
#define AUTH_STAGE_REQ1                         (uint8_t)0x01
#define AUTH_STAGE_ACK1                         (uint8_t)0x02
#define AUTH_STAGE_REQ2                         (uint8_t)0x03
#define AUTH_STAGE_ACK2                         (uint8_t)0x04
typedef struct {
    uint8_t APDU[SIM_APDU_LENGTH];
    uint8_t length;
}APDU_BUF_TypeDef;

typedef struct{
    uint8_t ICCID[ICCID_LENGTH];
    uint8_t IMSI[IMSI_LENGTH];
    uint8_t AD;
    APDU_BUF_TypeDef Tx_APDU;
    APDU_BUF_TypeDef RX_APDU;
    struct timeval start,end;
    double timeuse;
    uint8_t apdu_enable;
    uint8_t apdu_timeout;
    uint8_t auth_step;
    uint8_t state;
    FILE *log;
}SIM_TypeDef;

//flags -- R read,N something new
typedef struct{
    SIM_TypeDef SIM[SIM_NUMS];
    SPI_Buf_TypeDef TxBuf;
    SPI_Buf_TypeDef RxBuf;
    uint8_t HardWare_Version[3];
    uint8_t SoftWare_Version[3];
    uint8_t SIM_StateTblR, SIM_StateTblN;
    uint8_t SIM_APDUTblR, SIM_APDUTblN;
    uint8_t SIM_InfoTblR, SIM_InfoTblN;
    uint8_t SIM_CheckErrR,SIM_CheckErrN;
    uint8_t SIM_ResetTbl,SIM_StopTbl;
    uint8_t VersionR,VersionN;
    uint8_t heartbeat;
}MCU_TypeDef;
/******************* SIM end ************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

extern MCU_TypeDef MCUs[MCU_NUMS];
extern pthread_mutex_t tmutex_mcu_buf_access;

#endif
