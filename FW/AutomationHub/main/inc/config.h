#pragma once

#include <stdbool.h>
#include <stdint.h>

bool init_nvm(void);
bool storeSetTemp(uint8_t temp);
uint8_t fetchSetTemp(void);