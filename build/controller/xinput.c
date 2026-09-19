#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#include "ControllerState.h"

#ifndef SPC_INPUT_PATH
#define SPC_INPUT_PATH L"C:\\somethingpc-input.bin"
#endif

static SRWLOCK mapping_lock = SRWLOCK_INIT;
static const SPCControllerState *shared_state;
static volatile LONG input_enabled = TRUE;
static SRWLOCK keystroke_lock = SRWLOCK_INIT;
static WORD previous_buttons[SPC_CONTROLLER_COUNT];

static DWORD read_slot(DWORD index, SPCControllerSlot *result)
{
    if (index >= SPC_CONTROLLER_COUNT) return ERROR_DEVICE_NOT_CONNECTED;
    AcquireSRWLockExclusive(&mapping_lock);
    if (!shared_state) {
        HANDLE file = CreateFileW(SPC_INPUT_PATH, GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER size;
            if (GetFileSizeEx(file, &size) && size.QuadPart >= sizeof(SPCControllerState)) {
                HANDLE mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
                if (mapping) {
                    shared_state = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(SPCControllerState));
                    CloseHandle(mapping);
                }
            }
            CloseHandle(file);
        }
    }
    const SPCControllerState *state = shared_state;
    ReleaseSRWLockExclusive(&mapping_lock);
    if (!state || state->magic != SPC_INPUT_MAGIC || state->version != SPC_INPUT_VERSION)
        return ERROR_DEVICE_NOT_CONNECTED;
    const volatile SPCControllerSlot *slot = &state->slots[index];
    for (int attempt = 0; attempt < 8; attempt++) {
        DWORD sequence = slot->sequence;
        if (sequence & 1) continue;
        MemoryBarrier();
        CopyMemory(result, (const void *)slot, sizeof(*result));
        MemoryBarrier();
        if (sequence == slot->sequence)
            return result->connected ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetState(DWORD index, XINPUT_STATE *state)
{
    if (!state) return ERROR_BAD_ARGUMENTS;
    ZeroMemory(state, sizeof(*state));
    SPCControllerSlot slot;
    DWORD status = read_slot(index, &slot);
    if (status) return status;
    state->dwPacketNumber = slot.packet;
    if (input_enabled) CopyMemory(&state->Gamepad, &slot.buttons, sizeof(state->Gamepad));
    state->Gamepad.wButtons &= ~0x0400;
    return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetStateEx(DWORD index, XINPUT_STATE *state)
{
    DWORD status = XInputGetState(index, state);
    if (!status && input_enabled) {
        SPCControllerSlot slot;
        if (!read_slot(index, &slot)) state->Gamepad.wButtons = slot.buttons;
    }
    return status;
}

void WINAPI XInputEnable(BOOL enabled)
{
    InterlockedExchange(&input_enabled, enabled != FALSE);
}

DWORD WINAPI XInputGetCapabilities(DWORD index, DWORD flags, XINPUT_CAPABILITIES *caps)
{
    if (!caps || (flags & ~XINPUT_FLAG_GAMEPAD)) return ERROR_BAD_ARGUMENTS;
    ZeroMemory(caps, sizeof(*caps));
    SPCControllerSlot slot;
    DWORD status = read_slot(index, &slot);
    if (status) return status;
    caps->Type = XINPUT_DEVTYPE_GAMEPAD;
    caps->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
    caps->Gamepad.wButtons = 0xf3ff;
    caps->Gamepad.bLeftTrigger = caps->Gamepad.bRightTrigger = 255;
    caps->Gamepad.sThumbLX = caps->Gamepad.sThumbLY = 32767;
    caps->Gamepad.sThumbRX = caps->Gamepad.sThumbRY = 32767;
    return ERROR_SUCCESS;
}

DWORD WINAPI XInputSetState(DWORD index, XINPUT_VIBRATION *vibration)
{
    if (!vibration) return ERROR_BAD_ARGUMENTS;
    SPCControllerSlot slot;
    return read_slot(index, &slot);
}

DWORD WINAPI XInputGetCapabilitiesEx(DWORD version, DWORD index, DWORD flags, XINPUT_CAPABILITIES_EX *caps)
{
    if (version != 1 || !caps) return ERROR_BAD_ARGUMENTS;
    ZeroMemory(caps, sizeof(*caps));
    return XInputGetCapabilities(index, flags, &caps->Capabilities);
}

DWORD WINAPI XInputGetBatteryInformation(DWORD index, BYTE device_type, XINPUT_BATTERY_INFORMATION *battery)
{
    if (!battery || device_type > BATTERY_DEVTYPE_HEADSET) return ERROR_BAD_ARGUMENTS;
    ZeroMemory(battery, sizeof(*battery));
    SPCControllerSlot slot;
    DWORD status = read_slot(index, &slot);
    if (status) return status;
    if (device_type == BATTERY_DEVTYPE_GAMEPAD) {
        battery->BatteryType = slot.battery_type;
        battery->BatteryLevel = slot.battery_level;
    }
    return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetKeystroke(DWORD index, DWORD reserved, XINPUT_KEYSTROKE *key)
{
    if (!key || reserved) return ERROR_BAD_ARGUMENTS;
    ZeroMemory(key, sizeof(*key));
    if (!input_enabled) return ERROR_EMPTY;
    static const WORD masks[] = {0x1000,0x2000,0x4000,0x8000,0x0100,0x0200,1,2,4,8,0x10,0x20,0x40,0x80};
    static const WORD keys[] = {VK_PAD_A,VK_PAD_B,VK_PAD_X,VK_PAD_Y,VK_PAD_LSHOULDER,VK_PAD_RSHOULDER,
        VK_PAD_DPAD_UP,VK_PAD_DPAD_DOWN,VK_PAD_DPAD_LEFT,VK_PAD_DPAD_RIGHT,
        VK_PAD_START,VK_PAD_BACK,VK_PAD_LTHUMB_PRESS,VK_PAD_RTHUMB_PRESS};
    DWORD first = index == XUSER_INDEX_ANY ? 0 : index;
    DWORD limit = index == XUSER_INDEX_ANY ? SPC_CONTROLLER_COUNT : index + 1;
    if (first >= SPC_CONTROLLER_COUNT) return ERROR_DEVICE_NOT_CONNECTED;
    BOOL connected = FALSE;
    AcquireSRWLockExclusive(&keystroke_lock);
    for (DWORD player = first; player < limit; player++) {
        SPCControllerSlot slot;
        if (read_slot(player, &slot)) { previous_buttons[player] = 0; continue; }
        connected = TRUE;
        WORD changed = previous_buttons[player] ^ slot.buttons;
        for (unsigned int button = 0; button < sizeof(masks) / sizeof(masks[0]); button++) {
            if (!(changed & masks[button])) continue;
            previous_buttons[player] ^= masks[button];
            key->VirtualKey = keys[button];
            key->Flags = slot.buttons & masks[button] ? XINPUT_KEYSTROKE_KEYDOWN : XINPUT_KEYSTROKE_KEYUP;
            key->UserIndex = (BYTE)player;
            ReleaseSRWLockExclusive(&keystroke_lock);
            return ERROR_SUCCESS;
        }
    }
    ReleaseSRWLockExclusive(&keystroke_lock);
    return connected ? ERROR_EMPTY : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD index, GUID *render, GUID *capture)
{
    if (!render || !capture) return ERROR_BAD_ARGUMENTS;
    ZeroMemory(render, sizeof(*render));
    ZeroMemory(capture, sizeof(*capture));
    SPCControllerSlot slot;
    return read_slot(index, &slot);
}

DWORD WINAPI XInputGetAudioDeviceIds(DWORD index, WCHAR *render, UINT *render_count, WCHAR *capture, UINT *capture_count)
{
    if (!render_count || !capture_count) return ERROR_BAD_ARGUMENTS;
    if (render && *render_count) render[0] = 0;
    if (capture && *capture_count) capture[0] = 0;
    *render_count = *capture_count = 0;
    SPCControllerSlot slot;
    DWORD status = read_slot(index, &slot);
    return status ? status : ERROR_NOT_FOUND;
}
