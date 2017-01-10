#ifndef ARBITRATOR_H
#define ARBITRATOR_H

void DC_Power_ON(void);
void DC_Power_OFF(void);
void GPIO_OUT(unsigned int gpio,unsigned int value);
void Card_Board_Reset_ON(void);
void Card_Board_Reset_OFF(void);
int Arbitrator_Init(void);
int Arbitrator(uint8_t mcu_num);

#endif // ARBITRATOR_H
