#include "ControllerBridge.h"
#include "ControllerState.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static _Atomic(SPCControllerState *) shared_state;

int spc_controller_open(const char *prefix)
{
    char path[4096];
    if (atomic_load(&shared_state)) return 0;
    if (snprintf(path, sizeof(path), "%s/drive_c/somethingpc-input.bin", prefix) >= sizeof(path)) return -1;
    int descriptor = open(path, O_RDWR | O_CREAT, 0600);
    if (descriptor < 0) return -1;
    if (ftruncate(descriptor, sizeof(SPCControllerState)) != 0) {
        close(descriptor);
        return -1;
    }
    SPCControllerState *state = mmap(NULL, sizeof(*state), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
    close(descriptor);
    if (state == MAP_FAILED) return -1;
    memset(state, 0, sizeof(*state));
    state->version = SPC_INPUT_VERSION;
    state->magic = SPC_INPUT_MAGIC;
    atomic_store_explicit(&shared_state, state, memory_order_release);
    return 0;
}

void spc_controller_update(int index, int connected, uint16_t buttons,
                           uint8_t left_trigger, uint8_t right_trigger,
                           int16_t left_x, int16_t left_y, int16_t right_x, int16_t right_y,
                           uint8_t battery_type, uint8_t battery_level)
{
    SPCControllerState *state = atomic_load_explicit(&shared_state, memory_order_acquire);
    if (!state || index < 0 || index >= SPC_CONTROLLER_COUNT) return;
    SPCControllerSlot *slot = &state->slots[index];
    SPCControllerSlot next = {0};
    next.connected = connected != 0;
    if (connected) {
        next.buttons = buttons;
        next.left_trigger = left_trigger;
        next.right_trigger = right_trigger;
        next.left_x = left_x;
        next.left_y = left_y;
        next.right_x = right_x;
        next.right_y = right_y;
        next.battery_type = battery_type;
        next.battery_level = battery_level;
    }
    if (slot->connected == next.connected &&
        memcmp(&slot->buttons, &next.buttons, sizeof(next) - offsetof(SPCControllerSlot, buttons)) == 0) return;
    uint32_t sequence = slot->sequence;
    slot->sequence = sequence + 1;
    atomic_thread_fence(memory_order_seq_cst);
    slot->connected = next.connected;
    slot->packet++;
    memcpy(&slot->buttons, &next.buttons, sizeof(next) - offsetof(SPCControllerSlot, buttons));
    atomic_thread_fence(memory_order_seq_cst);
    slot->sequence = sequence + 2;
}
