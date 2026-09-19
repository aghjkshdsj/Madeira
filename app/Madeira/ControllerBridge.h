#pragma once
#include <stdint.h>

int spc_controller_open(const char *prefix);
void spc_controller_update(int index, int connected, uint16_t buttons,
                           uint8_t left_trigger, uint8_t right_trigger,
                           int16_t left_x, int16_t left_y, int16_t right_x, int16_t right_y,
                           uint8_t battery_type, uint8_t battery_level);
