#pragma once
#include <array>
#include <cstdint>
#include <Windows.h>

namespace Memory {
struct CRxScreen;
struct CNxSurface;
struct CRxSprite;
struct CRxFontList;
struct CRxInput;

struct Game {
    void(__thiscall** vtable)(Game*, signed int);
    HACCEL accelerator_0x66;
    HWND HWND;
    std::array<char, 256> window_name{};
    char field_10E;
    char field_10F;
    char field_110;
    char field_111;
    char field_112;
    char field_113;
    int is_window_active;
    DWORD last_system_time;
    char field_11C;
    char field_11D;
    char field_11E;
    char field_11F;
    char field_120;
    char field_121;
    char field_122;
    char field_123;
    int field_124;
    char field_128;
    char field_129;
    char field_12A;
    char field_12B;
    char field_12C;
    char field_12D;
    char field_12E;
    char field_12F;
    char field_130;
    char field_131;
    char field_132;
    char field_133;
    CRxScreen* RxScreen;
    CNxSurface* surface_1;
    CNxSurface* surface_2;
    CRxSprite* sprite_1;
    CNxSurface* surface_3;
    CRxSprite* sprite_2;
    CRxInput* rx_input;
    CRxFontList* font_list;
    TIMECAPS time_device_compatabilities;
    int smooth_mode;
};
static_assert(sizeof(Game) == 0x160, "Game is an invalid size");

struct CRxInput {
    uintptr_t vtable{};
    int has_joy{};
    unsigned int left_deadzone{};
    unsigned int right_deadzone{};
    unsigned int down_deadzone{};
    unsigned int up_deadzone{};
    std::array<unsigned char, 25> button_assignments{};
    std::array<unsigned char, 30> button_state{};
    std::array<unsigned char, 30> current_button_state{};
    std::array<unsigned char, 30> last_button_state{};
    char field_8B{};
    int first_press_on_frame{};
    int first_press_hold_framecount{};
    int dir4{};
    int dir8{};
    int is_moving_sideways{};
};
static_assert(sizeof(CRxInput) == 0xa0, "CRxInput is an invalid size.");

} // namespace Memory
