#ifndef _TEST_TASK_H_
#define _TEST_TASK_H_

void init_state_handler(void);

void updateSetTemp(uint8_t temp);
uint8_t readSetTemp();
uint8_t getMode(void);
void setMode(uint8_t mode);
uint8_t getTemp();
#endif // _TEST_TASK_H_