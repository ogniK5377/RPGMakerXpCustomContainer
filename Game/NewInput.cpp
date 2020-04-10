#include "EnginePatches.h"
#include "Input.h"
#include "MemoryUtil.h"
#include "NewInput.h"
#include "RPG_Game.h"
#include "RubyCommon.h"

namespace RubyModule {

#define INPUT_ACTION(F, N)                                                                         \
    static RB_VALUE N(RB_VALUE klass, RB_VALUE key) {                                              \
        if (key & 1) {                                                                             \
            if (F(Patches::RPGGameClass->rx_input, key >> 1)) {                                    \
                return 2;                                                                          \
            }                                                                                      \
        } else {                                                                                   \
            if (F(Patches::RPGGameClass->rx_input, Ruby::Common::Get()->NumToLong(key))) {         \
                return 2;                                                                          \
            }                                                                                      \
        }                                                                                          \
        return 0;                                                                                  \
    }

INPUT_ACTION(Input::KeyPress, Input_Press)
INPUT_ACTION(Input::KeyTrigger, Input_Trigger)
INPUT_ACTION(Input::KeyRepeat, Input_Repeat)

static RB_VALUE Input_Update() {
    Input::Update(Patches::RPGGameClass->rx_input);
    return 4;
}

static RB_VALUE Input_Dir4() {
    auto* common = Ruby::Common::Get();
    return common->IntToNum(Patches::RPGGameClass->rx_input->dir4);
}

static RB_VALUE Input_Dir8() {
    auto* common = Ruby::Common::Get();
    return common->IntToNum(Patches::RPGGameClass->rx_input->dir8);
}

void RegisterCustomInput() {
    if (!(*InputModule)) {
        RB_VALUE input_module{};
        auto* common = Ruby::Common::Get();
        input_module = common->DefineModule("Input");
        common->DefineSingletonMethod(input_module, "update", &RubyModule::Input_Update, 0);
        common->DefineSingletonMethod(input_module, "press?", &RubyModule::Input_Press, 1);
        common->DefineSingletonMethod(input_module, "trigger?", &RubyModule::Input_Trigger, 1);
        common->DefineSingletonMethod(input_module, "repeat?", &RubyModule::Input_Repeat, 1);
        common->DefineSingletonMethod(input_module, "dir4", &RubyModule::Input_Dir4, 0);
        common->DefineSingletonMethod(input_module, "dir8", &RubyModule::Input_Dir8, 0);

        // Consts
        common->DefineConst(input_module, "LEFT", common->IntToNum(4));
        common->DefineConst(input_module, "UP", common->IntToNum(8));
        common->DefineConst(input_module, "RIGHT", common->IntToNum(6));
        common->DefineConst(input_module, "DOWN", common->IntToNum(2));

        common->DefineConst(input_module, "A", common->IntToNum(11));
        common->DefineConst(input_module, "B", common->IntToNum(12));
        common->DefineConst(input_module, "C", common->IntToNum(13));
        common->DefineConst(input_module, "X", common->IntToNum(14));
        common->DefineConst(input_module, "Y", common->IntToNum(15));
        common->DefineConst(input_module, "Z", common->IntToNum(16));
        common->DefineConst(input_module, "L", common->IntToNum(17));
        common->DefineConst(input_module, "R", common->IntToNum(18));

        common->DefineConst(input_module, "SHIFT", common->IntToNum(21));
        common->DefineConst(input_module, "CTRL", common->IntToNum(22));
        common->DefineConst(input_module, "ALT", common->IntToNum(23));

        common->DefineConst(input_module, "F5", common->IntToNum(25));
        common->DefineConst(input_module, "F6", common->IntToNum(26));
        common->DefineConst(input_module, "F7", common->IntToNum(27));
        common->DefineConst(input_module, "F8", common->IntToNum(28));
        common->DefineConst(input_module, "F9", common->IntToNum(29));

        *InputModule = input_module;
    }
}

} // namespace RubyModule
