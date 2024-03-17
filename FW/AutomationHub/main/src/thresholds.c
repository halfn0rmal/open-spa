#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int lowValue;
    int hiValue;
    int setPoint;
    int hysteresis;
    bool aboveSetPoint;
} threshold_t;

typedef struct {
    threshold_t thresholds[4];
} threshold_set_t;

void init_thresholds(void);
void set_threshold(int index,int setPoint, int hysteresis);
bool check_threshold(int index, int value);

static threshold_set_t thresholds;

void init_thresholds(void){    
    
    thresholds.thresholds[0].setPoint = 2000;
    thresholds.thresholds[0].hysteresis = 100;
    thresholds.thresholds[0].lowValue = thresholds.thresholds[0].setPoint - thresholds.thresholds[0].hysteresis;
    thresholds.thresholds[0].hiValue = thresholds.thresholds[0].setPoint + thresholds.thresholds[0].hysteresis;
    thresholds.thresholds[0].aboveSetPoint = false;

    thresholds.thresholds[1].setPoint = 2000;
    thresholds.thresholds[1].hysteresis = 100;
    thresholds.thresholds[1].lowValue = thresholds.thresholds[1].setPoint - thresholds.thresholds[1].hysteresis;
    thresholds.thresholds[1].hiValue = thresholds.thresholds[1].setPoint + thresholds.thresholds[1].hysteresis;
    thresholds.thresholds[1].aboveSetPoint = false;

    thresholds.thresholds[2].setPoint = 2000;
    thresholds.thresholds[2].hysteresis = 100;
    thresholds.thresholds[2].lowValue = thresholds.thresholds[2].setPoint - thresholds.thresholds[2].hysteresis;
    thresholds.thresholds[2].hiValue = thresholds.thresholds[2].setPoint + thresholds.thresholds[2].hysteresis;
    thresholds.thresholds[2].aboveSetPoint = false;

    thresholds.thresholds[3].setPoint = 2000;
    thresholds.thresholds[3].hysteresis = 100;
    thresholds.thresholds[3].lowValue = thresholds.thresholds[3].setPoint - thresholds.thresholds[3].hysteresis;
    thresholds.thresholds[3].hiValue = thresholds.thresholds[3].setPoint + thresholds.thresholds[3].hysteresis;
    thresholds.thresholds[3].aboveSetPoint = false;
}

void set_threshold(int index,int setPoint, int hysteresis){
    thresholds.thresholds[index].setPoint = setPoint;
    thresholds.thresholds[index].hysteresis = hysteresis;
    thresholds.thresholds[index].lowValue = thresholds.thresholds[index].setPoint - thresholds.thresholds[index].hysteresis;
    thresholds.thresholds[index].hiValue = thresholds.thresholds[index].setPoint + thresholds.thresholds[index].hysteresis;
    thresholds.thresholds[index].aboveSetPoint = false;
}

bool check_threshold(int index, int value){
    if(value > thresholds.thresholds[index].hiValue){
        thresholds.thresholds[index].aboveSetPoint = true;
    }else if(value < thresholds.thresholds[index].lowValue){
        thresholds.thresholds[index].aboveSetPoint = false;
    }
    return thresholds.thresholds[index].aboveSetPoint;
}