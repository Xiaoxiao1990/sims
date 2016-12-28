#ifndef SIM_H
#define SIM_H


//extern void * frame_package(void *arg);
//extern void * frame_parse(void *arg);
extern int block_length_check(DataType_TypeDef datatype,uint8_t data_len);
void MCU_Init(MCU_TypeDef *mcu);
void SIMs_Table_init(void);

#endif // SIM_H
