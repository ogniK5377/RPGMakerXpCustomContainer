#pragma once
#include <functional>
#include <vector>
#include <Windows.h>
using RB_VALUE = unsigned long;

namespace Ruby {
struct Float {
    double GetValue() const {
        return value;
    }
    int unknown0x0{};
    double value{};
};

class Common {
public:
    static Ruby::Common* Get();

    void Initialize(const char* library_path);
    void AddNewModule(std::function<void()> f);

    RB_VALUE DefineModule(const char* module_name);
    void DefineModuleFunction(RB_VALUE module_id, const char* method_name, void* method, int argc);
    void DefineConst(RB_VALUE module_id, const char* const_name, RB_VALUE value);
    RB_VALUE MakeFloat(double value);
    Ruby::Float* GetFloat(RB_VALUE value);

    std::vector<std::function<void()>>& GetModuleRegistry();
    static void RegisterRectModule();
    void InternalRectCallback();

    DWORD rb_define_module_function_addr_abs{};

private:
    static Ruby::Common* instance;
    Common();

    using RegisterRectModuleType = void (*)();
    using RbDefineModuleType = RB_VALUE(__cdecl*)(const char* module_name);
    using RbDefineModuleFunctionType = void(__cdecl*)(RB_VALUE module_id, const char* method_name,
                                                      void* method, int argument_count);
    using RbDefineConstType = void(__cdecl*)(RB_VALUE module_id, const char* name, RB_VALUE value);
    using RbFloatNewType = RB_VALUE(__cdecl*)(double value);
    using RbFloatType = Ruby::Float*(__cdecl*)(RB_VALUE value);

    RegisterRectModuleType O_RegisterRectModule;
    RbDefineModuleType rb_define_module;
    RbDefineModuleFunctionType rb_define_module_function;
    RbDefineConstType rb_define_const;
    RbFloatNewType rb_float_new;
    RbFloatType rb_float;

    std::vector<std::function<void()>> module_registry;
};
} // namespace Ruby
