/*****************************
*SIMs typedef
*****************************/

#ifndef _SIMS_TYPE_H_
#define _SIMS_TYPE_H_

#include <stdint.h>
/******************* spi.c ************************/
#define SPI_TRANSFER_MTU                                (uint16_t)300
//SPI Buffer state
#define SPI_BUF_STATE_EMPTY                             (uint8_t)0x00
#define SPI_BUF_STATE_PACKAGING                         (uint8_t)0x01
#define SPI_BUF_STATE_READY                             (uint8_t)0x02
#define SPI_BUF_STATE_FULL                              (uint8_t)0x03
#define SPI_BUF_STATE_TRANSMITING                       (uint8_t)0x04
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

typedef struct SPI_Buf{
    uint8_t Buf[SPI_TRANSFER_MTU];
    uint16_t Length;
    uint16_t Buf_Len_Left;
    uint8_t state;
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

#define SIM_APDU_LENGTH                         (uint8_t)40
#define ICCID_LENGTH                            (uint8_t)0x0A
#define IMSI_LENGTH                             (uint8_t)0x08

typedef struct{
    uint8_t ICCID[ICCID_LENGTH];
    uint8_t IMSI[IMSI_LENGTH];
    uint8_t AD;
    uint8_t Tx_APDU[SIM_APDU_LENGTH];
    uint8_t Tx_Length;
    uint8_t Rx_APDU[SIM_APDU_LENGTH];
    uint8_t Rx_Length;
    uint8_t state;
}SIM_TypeDef;

typedef struct{
    SIM_TypeDef SIM[SIM_NUMS];
    SPI_Buf_TypeDef TxBuf;
    SPI_Buf_TypeDef RxBuf;
    uint8_t HardWare_Version[3];
    uint8_t SoftWare_Version[3];
    uint8_t SIM_StateTblR, SIM_StateTblW;
    uint8_t SIM_APDUTblR, SIM_APDUTblW;
    uint8_t SIM_InfoTblR, SIM_InfoTblW;
    uint8_t SIM_CheckErrR,SIM_CheckErrW;
    uint8_t SIM_ResetTbl,SIM_StopTbl;
    uint8_t VersionR,VersionW;
}MCU_TypeDef;
/******************* SIM end ************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif
