#include <array>
#include <Windows.h>
#include "EnginePatches.h"
#include "Input.h"
#include "MemoryUtil.h"
#include "RPG_Game.h"

namespace Input {

bool KeyDown(int vkey) {
    return GetAsyncKeyState(vkey) < 0;
}

void Poll(Memory::CRxInput* input) {
    auto& bstate = input->button_state;
    auto& bassign = input->button_assignments;
    if (input->has_joy) {
        joyinfoex_tag pji{};
        pji.dwSize = 52;
        pji.dwFlags = 255;
        if (!joyGetPosEx(0, &pji)) {
            if (pji.dwXpos < input->left_deadzone) {
                bstate[4] = 1;
            }
            if (pji.dwYpos < input->up_deadzone) {
                bstate[8] = 1;
            }
            if (pji.dwXpos > input->right_deadzone) {
                bstate[6] = 1;
            }
            if (pji.dwYpos > input->down_deadzone) {
                bstate[2] = 1;
            }

            for (DWORD i = 0; i < 10; ++i) {
                if (pji.dwButtons & (1 << i)) {
                    bstate[bassign[i]] = 1;
                }
            }
        }
    }

    // Directional movement
    if (KeyDown(VK_NUMPAD4) || KeyDown(VK_LEFT)) {
        bstate[4] = 1;
    }
    if (KeyDown(VK_NUMPAD8) || KeyDown(VK_UP)) {
        bstate[8] = 1;
    }
    if (KeyDown(VK_NUMPAD6) || KeyDown(VK_RIGHT)) {
        bstate[6] = 1;
    }
    if (KeyDown(VK_NUMPAD2) || KeyDown(VK_DOWN)) {
        bstate[2] = 1;
    }

    auto CheckAssignment = [&](int key, int assignment) {
        if (KeyDown(key)) {
            bstate[bassign[assignment]] = 1;
        }
    };

    auto CheckButton = [&](int key, int assignment) {
        if (KeyDown(key)) {
            bstate[assignment] = 1;
        }
    };

    // RPG Maker Keys
    if (KeyDown(VK_SPACE) && !KeyDown(VK_MENU)) {
        bstate[bassign[10]] = 1;
    }
    if (KeyDown(VK_RETURN) && !KeyDown(VK_MENU)) {
        bstate[bassign[11]] = 1;
    }
    CheckAssignment(VK_ESCAPE, 12);
    CheckAssignment(VK_NUMPAD0, 13);
    CheckAssignment(VK_INSERT, 13);
    CheckAssignment(VK_SHIFT, 14);

    CheckButton(VK_PRIOR, 17);
    CheckButton(VK_NEXT, 18);

    CheckAssignment('Z', 15);
    CheckAssignment('X', 16);
    CheckAssignment('C', 17);
    CheckAssignment('V', 18);
    CheckAssignment('B', 19);
    CheckAssignment('A', 20);
    CheckAssignment('S', 21);
    CheckAssignment('D', 22);
    CheckAssignment('Q', 23);
    CheckAssignment('W', 24);

    CheckButton(VK_SHIFT, 21);
    CheckButton(VK_CONTROL, 22);
    CheckButton(VK_MENU, 23);

    CheckButton(VK_F5, 25);
    CheckButton(VK_F6, 26);
    CheckButton(VK_F7, 27);
    CheckButton(VK_F8, 28);
    CheckButton(VK_F9, 29);

    if (KeyDown('P')) {
        auto* game = Patches::RPGGameClass;
        if (timeGetTime() - game->last_fullscreen_time >= 1000) {
            MemoryUtil::AddressCall<MemoryUtil::CallConvention::ThisCall, void, Memory::CRxScreen*,
                                    int>(ChangeScreenModeAddress, game->RxScreen,
                                         game->RxScreen->is_full_screen == 0);
            game->last_fullscreen_time = timeGetTime();
        }
    }

    bstate[0] = 0;
}

bool KeyRepeat(Memory::CRxInput* input, int key) {
    if (key < 0 || key >= 30) {
        return false;
    }
    return input->first_press_on_frame == key && (input->first_press_hold_framecount == 1 ||
                                                  input->first_press_hold_framecount >= 0x10u &&
                                                      !(input->first_press_hold_framecount % 4u));
}

bool KeyTrigger(Memory::CRxInput* input, int key) {
    if (key < 0 || key >= 30) {
        return false;
    }
    return input->current_button_state[key] && !input->last_button_state[key];
}

bool KeyPress(Memory::CRxInput* input, int key) {
    if (key < 0 || key >= 30) {
        return false;
    }
    return input->current_button_state[key];
}

void Update(Memory::CRxInput* input) {
    Poll(input);
    std::memcpy(input->last_button_state.data(), input->current_button_state.data(),
                input->current_button_state.size());
    std::memcpy(input->current_button_state.data(), input->button_state.data(),
                input->current_button_state.size());
    std::memset(input->button_state.data(), 0, input->button_state.size());

    for (int i = 0; i < static_cast<int>(input->button_state.size()); i++) {
        if (KeyTrigger(input, i)) {
            input->first_press_on_frame = i;
            input->first_press_hold_framecount = 0;
        }
    }

    if (KeyPress(input, input->first_press_on_frame)) {
        input->first_press_hold_framecount++;
    } else {
        input->first_press_on_frame = 0;
        input->first_press_hold_framecount = 0;
    }

    int left_right_dir = 0;
    int up_down_dir = 0;

    if (KeyPress(input, 4)) {
        left_right_dir--;
    }
    if (KeyPress(input, 8)) {
        up_down_dir--;
    }
    if (KeyPress(input, 6)) {
        left_right_dir++;
    }
    if (KeyPress(input, 2)) {
        up_down_dir++;
    }

    // Get Dir8
    input->dir8 = 0;
    if (left_right_dir < 0) {
        if (up_down_dir < 0) {
            input->dir8 = 7;
        } else if (up_down_dir == 0) {
            input->dir8 = 4;
        } else {
            input->dir8 = 1;
        }
    } else if (left_right_dir == 0) {
        if (up_down_dir < 0) {
            input->dir8 = 8;
        } else if (up_down_dir > 0) {
            input->dir8 = 2;
        }
    } else if (left_right_dir > 0) {
        if (up_down_dir < 0) {
            input->dir8 = 9;
        } else if (up_down_dir == 0) {
            input->dir8 = 6;
        } else {
            input->dir8 = 3;
        }
    }

    if (left_right_dir && up_down_dir) {
        if (!input->is_moving_sideways) {
            up_down_dir = 0;
        } else if (input->is_moving_sideways == 1) {
            left_right_dir = 0;
        }
    } else if (left_right_dir) {
        input->is_moving_sideways = 1;
    } else if (up_down_dir) {
        input->is_moving_sideways = 0;
    }

    // Dir4
    input->dir4 = 0;
    if (left_right_dir < 0) {
        input->dir4 = 4;
    } else if (up_down_dir < 0) {
        input->dir4 = 8;
    } else if (left_right_dir > 0) {
        input->dir4 = 6;
    } else if (up_down_dir > 0) {
        input->dir4 = 2;
    }
}

} // namespace Input
