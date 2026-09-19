#define SPC_INPUT_PATH L"controller-state.bin"
#include "xinput.c"
#include <stdio.h>

static int failures;
#define CHECK(condition) do { if (!(condition)) { printf("FAIL line %d: %s\n", __LINE__, #condition); failures++; } } while (0)

int main(void)
{
    HANDLE file = CreateFileW(SPC_INPUT_PATH, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, NULL);
    CHECK(file != INVALID_HANDLE_VALUE);
    if (file == INVALID_HANDLE_VALUE) return 1;
    HANDLE mapping = CreateFileMappingW(file, NULL, PAGE_READWRITE, 0, sizeof(SPCControllerState), NULL);
    CHECK(mapping != NULL);
    if (!mapping) return 1;
    SPCControllerState *state = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, sizeof(*state));
    CHECK(state != NULL);
    if (!state) return 1;
    ZeroMemory(state, sizeof(*state));
    state->magic = SPC_INPUT_MAGIC;
    state->version = SPC_INPUT_VERSION;
    XINPUT_STATE result;
    CHECK(XInputGetState(0, &result) == ERROR_DEVICE_NOT_CONNECTED);
    CHECK(XInputGetState(4, &result) == ERROR_DEVICE_NOT_CONNECTED);
    CHECK(XInputGetState(0, NULL) == ERROR_BAD_ARGUMENTS);
    SPCControllerSlot *slot = &state->slots[0];
    slot->sequence = 1;
    slot->connected = 1;
    slot->packet = 7;
    slot->buttons = XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_DPAD_UP;
    slot->left_trigger = 127;
    slot->right_trigger = 255;
    slot->left_x = -32767;
    slot->left_y = 16384;
    slot->right_x = 42;
    slot->right_y = -12345;
    slot->battery_type = BATTERY_TYPE_UNKNOWN;
    MemoryBarrier();
    slot->sequence = 2;
    CHECK(XInputGetState(0, &result) == ERROR_SUCCESS);
    CHECK(result.dwPacketNumber == 7);
    CHECK(result.Gamepad.wButtons == (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_DPAD_UP));
    CHECK(result.Gamepad.sThumbLX == -32767 && result.Gamepad.sThumbRY == -12345);
    CHECK(result.Gamepad.bLeftTrigger == 127 && result.Gamepad.bRightTrigger == 255);
    XInputEnable(FALSE);
    CHECK(XInputGetState(0, &result) == ERROR_SUCCESS && result.Gamepad.wButtons == 0 && result.Gamepad.sThumbLX == 0);
    XInputEnable(TRUE);
    XINPUT_CAPABILITIES caps;
    CHECK(XInputGetCapabilities(0, 0, &caps) == ERROR_SUCCESS);
    CHECK(caps.Type == XINPUT_DEVTYPE_GAMEPAD && !(caps.Flags & XINPUT_CAPS_FFB_SUPPORTED));
    XINPUT_BATTERY_INFORMATION battery;
    CHECK(XInputGetBatteryInformation(0, BATTERY_DEVTYPE_GAMEPAD, &battery) == ERROR_SUCCESS);
    CHECK(battery.BatteryType == BATTERY_TYPE_UNKNOWN);
    XINPUT_KEYSTROKE key;
    CHECK(XInputGetKeystroke(0, 0, &key) == ERROR_SUCCESS && key.VirtualKey == VK_PAD_A && key.Flags == XINPUT_KEYSTROKE_KEYDOWN);
    CHECK(XInputGetKeystroke(0, 0, &key) == ERROR_SUCCESS && key.VirtualKey == VK_PAD_DPAD_UP);
    CHECK(XInputGetKeystroke(0, 0, &key) == ERROR_EMPTY);
    slot->sequence = 3;
    CHECK(XInputGetState(0, &result) == ERROR_DEVICE_NOT_CONNECTED);
    slot->buttons = 0;
    slot->sequence = 4;
    CHECK(XInputGetKeystroke(0, 0, &key) == ERROR_SUCCESS && key.Flags == XINPUT_KEYSTROKE_KEYUP);
    state->slots[3].connected = 1;
    CHECK(XInputGetState(3, &result) == ERROR_SUCCESS);
    slot->connected = 0;
    CHECK(XInputGetState(0, &result) == ERROR_DEVICE_NOT_CONNECTED);
    state->version = 999;
    CHECK(XInputGetState(3, &result) == ERROR_DEVICE_NOT_CONNECTED);
    UnmapViewOfFile(state);
    CloseHandle(mapping);
    CloseHandle(file);
    DeleteFileW(SPC_INPUT_PATH);
    printf("Controller bridge checks: %s\n", failures ? "FAILED" : "PASSED");
    return failures ? 1 : 0;
}
