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
    RB_VALUE DefineClass(const char* name, RB_VALUE super);
    void DefineSingletonMethod(RB_VALUE obj, const char* name, void* method, int argc);
    void DefineModuleFunction(RB_VALUE module_id, const char* method_name, void* method, int argc);
    void DefineConst(RB_VALUE module_id, const char* const_name, RB_VALUE value);
    long NumToLong(RB_VALUE num);
    RB_VALUE IntToNum(long num);
    RB_VALUE MakeFloat(double value);
    Ruby::Float* GetFloat(RB_VALUE value);
    bool ObjIsInstanceOf(RB_VALUE obj, RB_VALUE c);
    RB_VALUE GetcObject() const;

    std::vector<std::function<void()>>& GetModuleRegistry();
    static void RegisterRectModule();
    void InternalRectCallback();

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
    using RbObjIsInstanceOf = RB_VALUE(__cdecl*)(RB_VALUE obj, RB_VALUE c);
    using RbDefineClassType = RB_VALUE(__cdecl*)(const char* name, RB_VALUE super);
    using RbDefineSingletonMethodType = void(__cdecl*)(RB_VALUE obj, const char* name, void* method,
                                                       int argc);
    using RbNum2LongType = long(__cdecl*)(RB_VALUE num);
    using RbInt2INumType = RB_VALUE(__cdecl*)(long num);

    RegisterRectModuleType O_RegisterRectModule{};
    RbDefineModuleType rb_define_module{};
    RbDefineModuleFunctionType rb_define_module_function{};
    RbDefineConstType rb_define_const{};
    RbFloatNewType rb_float_new{};
    RbFloatType rb_float{};
    RbObjIsInstanceOf rb_obj_is_instance_of{};
    RbDefineClassType rb_define_class{};
    RbDefineSingletonMethodType rb_define_singleton_method{};
    RbNum2LongType rb_num2long{};
    RbInt2INumType rb_int2inum{};
    RB_VALUE rb_cObject{};

    std::vector<std::function<void()>> module_registry;
};
} // namespace Ruby
