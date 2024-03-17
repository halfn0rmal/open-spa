#ifndef _OUTPUT_MANAGER_H_
#define _OUTPUT_MANAGER_H_
#include <stdint.h>

#define OUT_1               4
#define OUT_2               0
#define OUT_3               2
#define OUT_4               15

typedef enum {
    off,
    on
} output_state_t;

typedef enum {
    latch,
    timed,
    flash
} output_mode_t;

typedef struct {
    uint32_t ioNumber;
    uint8_t state;
    uint32_t time;
    uint8_t mode;
} output_command_t;

void init_output_task(void);
bool set_output(uint32_t ioNumber, uint8_t state);

#endif // _OUTPUT_MANAGER_H_