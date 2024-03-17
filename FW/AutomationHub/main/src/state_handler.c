#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "inc/output_manager.h"
#include "inc/input_manager.h"
#include "gatts_table_creat_demo.h"
#include <math.h>
#include "inc/config.h"

#define STATE_HANDLER_STACK_SIZE        (2048)
#define STATE_HANDLER_TASK_PRIORITY     (9)
#define HYSTERESIS_VALUE                (1) // 1 degree hysteresis
const static char *TAG = "TEST";

#define DELAY_TIME 1000
enum systemState {
    startup,
    transitionToHeating,
    idle,
    heating,
    transitionToJets,
    jets,
    fault
};

static uint8_t state = startup;
static uint8_t setTemp = 37;
static uint8_t currentTemp = 0;

void changeState(uint8_t newState){
    gattUpdateMode(newState);
    state = newState;
}

void circ_timer_callback(TimerHandle_t xTimer){
    printf("Circulation timer expired\n");
    if(state == idle){
        changeState(transitionToHeating);
    }else if(state == heating){
        changeState(idle);
    }
}

void jets_timer_callback(TimerHandle_t xTimer){
    printf("Jets timer expired\n");
    if(state == jets){
        changeState(transitionToHeating);
    }
}

uint8_t getMode(void){
    return state;
}

void setMode(uint8_t mode){
    // safety to only allow supported modes
    if(mode == transitionToHeating || mode == transitionToJets){
        changeState(mode);
    }
}

uint8_t getTemp(){
    return currentTemp;
}

void updateSetTemp(uint8_t temp){
    setTemp = temp;
    storeSetTemp(setTemp);
    changeState(transitionToHeating);
}

uint8_t readSetTemp(){
    return setTemp;
}

bool isAboveSetTemp(uint8_t temp){
    // Check against the hysterisis
    static bool isAbove = false;
    if(temp > setTemp){
        isAbove = true;
        return true;
    }else if (isAbove){
        if (temp < (setTemp - HYSTERESIS_VALUE)){
            isAbove = false;
            return false;
        }else{
            return true;
        }
    }else{
        isAbove = false;
        return false;
    }
}

uint8_t getTempFromVoltage(int voltage_mV){
    // We know that the lower section of the resistor divider is 10K and the thermistor is 10K at 25C
    // So we can use the voltage to calculate the resistance of the thermistor and then use a first order approximation to get the temperature
    // With the voltage being 5V and the ADC being 12 bits we can calculate the voltage to resistance and then to temperature
    // R1 = ( (vin - vout) * R2 ) / vout
    float k = 3892;
    float coefficent = -4.4;
    // Convert mV to V
    float voltage = (float)voltage_mV / 1000;
    //ESP_LOGI(TAG, "Voltage: %f", voltage);

    // Calculate the total resistance of the thermistor and the 10K resistor
    float resistance = ((5 - voltage) * 10000) / voltage;
    //ESP_LOGI(TAG, "Resistance: %f", resistance);
    // Calculate the resistance of the thermistor
    resistance = resistance - 11500;
    //ESP_LOGI(TAG, "Resistance: %f", resistance);

    // Calculate the temperature using the B-parameter equation
    float temp = 1 / ((log(resistance / 10000) / k) + (1 / (25 + 273.15))) - 273.15;
    //ESP_LOGI(TAG, "Temp: %f", temp);
    // clamp the temp to 0-50
    if(temp < 0){
        temp = 0;
    }else if(temp > 50){
        temp = 50;
    }
    currentTemp = temp;
    gattUpdateTemp(currentTemp);
    return currentTemp;
}

void state_handler(void *pvParameters){
    input_state_t inputState;
    printf("test_task startup\n");

    TimerHandle_t circ_timer = xTimerCreate("circulation_timer", 10800000 / portTICK_PERIOD_MS, pdTRUE, ( void * ) 0, circ_timer_callback);
    TimerHandle_t jets_timer = xTimerCreate("jets_timer", 1800000 / portTICK_PERIOD_MS, pdTRUE, ( void * ) 0, jets_timer_callback);
    for(;;){
        if(get_state(&inputState)){
            // printf("Input 1: %d\n", inputState.voltage[0]);
            // printf("Input 2: %d\n", inputState.voltage[1]);
            // printf("Input 3: %d\n", inputState.voltage[2]);
            // printf("Input 4: %d\n", inputState.voltage[3]);
        }
        getTempFromVoltage(inputState.voltage[eInput1]);
        switch(state){
            case startup:
            // All off, initialize state from power on
                set_output(OUT_1, off);
                set_output(OUT_2, off);
                set_output(OUT_3, off);
                set_output(OUT_4, off);
                changeState(transitionToHeating);
                break;

            case transitionToHeating:
            {
                ESP_LOGI(TAG, "Transition to heating state");
                ESP_LOGI(TAG, "Set temp: %d", setTemp);
                xTimerStart( circ_timer, 0 );
                xTimerStop( jets_timer, 0 );
                changeState(heating);
                break;
            }

            case idle:
            // All off, circ on for 3hrs then off for 3 hrs
                set_output(OUT_1, off);
                set_output(OUT_2, off);
                set_output(OUT_3, off);
                set_output(OUT_4, off);
                // if jets pressed then go to jets state.
                break;

            case heating:
            // Circ and Heater on for 3 hrs then off for 3 hrs
                set_output(OUT_3, off);
                set_output(OUT_4, off);
                set_output(OUT_1, on);

                
                // If the temp is above the set point then turn heater off
                if(isAboveSetTemp(getTempFromVoltage(inputState.voltage[eInput1]))){
                    set_output(OUT_2, off);
                }else{
                    set_output(OUT_2, on);
                }
                break;

            case transitionToJets:
            {
                ESP_LOGI(TAG, "Transition to jets");
                xTimerStart( jets_timer, 0 );
                xTimerStop( circ_timer, 0 );
                changeState(jets);
                break; 
            }

            case jets:
            // High speed on, circ and pump off
                set_output(OUT_1, off);
                set_output(OUT_2, off);
                set_output(OUT_3, on);
                set_output(OUT_4, off);
                break;

            case fault:
            // All off disabled
                set_output(OUT_1, off);
                set_output(OUT_2, off);
                set_output(OUT_3, off);
                set_output(OUT_4, off);
                xTimerStop( circ_timer, 0 );
                break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void init_state_handler(void)
{
    uint8_t storedTemp = fetchSetTemp();
    if (storedTemp != 0){
        setTemp = storedTemp;
    }
    xTaskCreate(state_handler, "State Handler", STATE_HANDLER_STACK_SIZE, NULL, STATE_HANDLER_TASK_PRIORITY, NULL);
}