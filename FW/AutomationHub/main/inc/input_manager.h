#ifndef _ADC_INPUT_H_
#define _ADC_INPUT_H_

#define NUMBER_OF_INPUTS 4
enum{
    eInput1 = 0,
    eInput2,
    eInput3,
    eInput4
};

typedef struct{
    int raw[NUMBER_OF_INPUTS];
    int voltage[NUMBER_OF_INPUTS];
}input_state_t;

void init_input_task(void);
bool get_state(input_state_t * state);

#endif // _ADC_INPUT_H_