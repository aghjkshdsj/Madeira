#pragma once
#include <stdint.h>
#include <stddef.h>

#define SPC_INPUT_MAGIC 0x53504349u
#define SPC_INPUT_VERSION 1u
#define SPC_CONTROLLER_COUNT 4

typedef struct {
    volatile uint32_t sequence;
    uint32_t connected;
    uint32_t packet;
    uint16_t buttons;
    uint8_t left_trigger;
    uint8_t right_trigger;
    int16_t left_x;
    int16_t left_y;
    int16_t right_x;
    int16_t right_y;
    uint8_t battery_type;
    uint8_t battery_level;
    uint8_t reserved[6];
} SPCControllerSlot;

typedef struct {
    uint32_t magic;
    uint32_t version;
    SPCControllerSlot slots[SPC_CONTROLLER_COUNT];
} SPCControllerState;

_Static_assert(sizeof(SPCControllerSlot) == 32, "Controller ABI size");
_Static_assert(offsetof(SPCControllerSlot, buttons) == 12, "XInput gamepad offset");
_Static_assert(offsetof(SPCControllerSlot, battery_type) == 24, "Controller battery offset");
