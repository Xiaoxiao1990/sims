#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "types.h"
#include <sys/time.h>

void flush_array_r(uint8_t *arr,uint16_t arr_len);
#define flush_array(a) flush_array_r(a,ARRAY_SIZE(a))

void print_array_r(uint8_t *arr,uint16_t arr_len);
#define print_array(a) print_array_r(a,ARRAY_SIZE(a))

void bin_echo(uint8_t byte);
void print_sim(SIM_TypeDef *sim);
void print_MCU(MCU_TypeDef *mcu);

uint8_t slot_parse(uint8_t *ActionTbl);

void clear_flag(uint8_t *ActionTbl, uint8_t slot);
void _SIMs_Printer(void);
void SPI_Buf_init(SPI_Buf_TypeDef *buf);

double time_use(struct timeval *start_time, struct timeval *end_time);

#endif // FUNCTIONS_H
