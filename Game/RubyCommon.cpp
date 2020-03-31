#include "MemoryUtil.h"
#include "RubyCommon.h"
#include "SigScanner.h"

namespace Ruby {
Ruby::Common* Ruby::Common::instance = nullptr;
Common::Common() = default;

Ruby::Common* Common::Get() {
    if (!instance) {
        instance = new Ruby::Common();
    }
    return instance;
}

void Common::Initialize(const char* library_path) {
    MemoryUtil::SigScanner scanner(library_path);
    scanner.AddNewSignature("RegisterRectModule",
                            "\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x5D\xC3\xCC\xCC\xCC\x55",
                            "x????x????xxxxxx");

    scanner.AddNewSignature("rb_define_module",
                            "\xE8\x00\x00\x00\x00\x83\xC4\x04\x89\x45\xF8\x8D\x45\x0C",
                            "x????xxxxxxxxx");

    scanner.AddNewSignature("rb_define_module_function", "\xE8\x00\x00\x00\x00\x83\xC4\x10\x6A\x09",
                            "x????xxxxx");

    scanner.AddNewSignature("rb_define_const", "\xE8\x00\x00\x00\x00\x83\xC4\x0C\x6A\x08",
                            "x????xxxxx");

    scanner.AddNewSignature("rb_float_new", "\xE8\x00\x00\x00\x00\x83\xC4\x08\xEB\x54",
                            "x????xxxxx");
    scanner.AddNewSignature("rb_float", "\x55\x8B\xEC\x83\xEC\x10\x8B\x45\xF8", "xxxxxxxxx");
    scanner.AddNewSignature(
        "rb_define_class", "\xE8\x00\x00\x00\x00\x83\xC4\x08\x89\x45\xFC\x6A\x04", "x????xxxxxxxx");
    scanner.AddNewSignature("HandleF12Reset",
                            "\xE8\x00\x00\x00\x00\x83\x3D\x00\x00\x00\x00\x00\x74\x1E",
                            "x????xx?????xx");
    scanner.AddNewSignature("rb_define_singleton_method",
                            "\xE8\x00\x00\x00\x00\x83\xC4\x10\x8B\x4D\xF4", "x????xxxxxx");
    scanner.AddNewSignature("rb_num2long",
                            "\xE8\x00\x00\x00\x00\x83\xC4\x04\x89\x45\x9C\x8B\x45\x9C",
                            "x????xxxxxxxxx");
    scanner.AddNewSignature("rb_int2inum", "\xE8\x00\x00\x00\x00\x83\xC4\x04\xEB\x72",
                            "x????xxxxx");
    scanner.Scan();

    if (scanner.HasFoundAll()) {
        const auto register_rect_module_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("RegisterRectModule"));
        const auto rb_define_module_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("rb_define_module"));
        const auto rb_define_module_function_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("rb_define_module_function"));
        const auto rb_define_const_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("rb_define_const"));
        const auto rb_float_new_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("rb_float_new"));
        const auto rb_float_addr = scanner.GetScannedAddress("rb_float");
        const auto rb_define_class_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("rb_define_class"));
        const auto HandleF12Reset_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("HandleF12Reset"));
        const auto rb_define_singleton_method_addr = MemoryUtil::CallToDirectAddress(
            scanner.GetScannedAddress("rb_define_singleton_method"));
        const auto rb_num2long_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("rb_num2long"));
        const auto rb_int2inum_addr =
            MemoryUtil::CallToDirectAddress(scanner.GetScannedAddress("rb_int2inum"));

        rb_cObject = *reinterpret_cast<RB_VALUE*>(HandleF12Reset_addr + 0x2A + 2);

        O_RegisterRectModule = MemoryUtil::CreateDetour<RegisterRectModuleType>(
            register_rect_module_addr, reinterpret_cast<uintptr_t>(&Common::RegisterRectModule));

        rb_define_module = MemoryUtil::MakeCallable<RbDefineModuleType>(rb_define_module_addr);
        rb_define_module_function =
            MemoryUtil::MakeCallable<RbDefineModuleFunctionType>(rb_define_module_function_addr);
        rb_define_const = MemoryUtil::MakeCallable<RbDefineConstType>(rb_define_const_addr);
        rb_float_new = MemoryUtil::MakeCallable<RbFloatNewType>(rb_float_new_addr);
        rb_float = MemoryUtil::MakeCallable<RbFloatType>(rb_float_addr);
        rb_define_class = MemoryUtil::MakeCallable<RbDefineClassType>(rb_define_class_addr);
        rb_define_singleton_method =
            MemoryUtil::MakeCallable<RbDefineSingletonMethodType>(rb_define_singleton_method_addr);
        rb_num2long = MemoryUtil::MakeCallable<RbNum2LongType>(rb_num2long_addr);
        rb_int2inum = MemoryUtil::MakeCallable<RbInt2INumType>(rb_int2inum_addr);
    }
}

void Common::AddNewModule(std::function<void()> f) {
    module_registry.push_back(f);
}

RB_VALUE Common::DefineModule(const char* module_name) {
    return rb_define_module(module_name);
}

RB_VALUE Common::DefineClass(const char* name, RB_VALUE super) {
    return rb_define_class(name, super);
}

void Common::DefineSingletonMethod(RB_VALUE obj, const char* name, void* method, int argc) {
    rb_define_singleton_method(obj, name, method, argc);
}

void Common::DefineModuleFunction(RB_VALUE module_id, const char* method_name, void* method,
                                  int argc) {
    rb_define_module_function(module_id, method_name, method, argc);
}

void Common::DefineConst(RB_VALUE module_id, const char* const_name, RB_VALUE value) {
    rb_define_const(module_id, const_name, value);
}

long Common::NumToLong(RB_VALUE num) {
    return rb_num2long(num);
}

RB_VALUE Common::IntToNum(long num) {
    return rb_int2inum(num);
}

RB_VALUE Common::MakeFloat(double value) {
    return rb_float_new(value);
}

Ruby::Float* Common::GetFloat(RB_VALUE value) {
    return rb_float(value);
}

bool Common::ObjIsInstanceOf(RB_VALUE obj, RB_VALUE c) {
    return rb_obj_is_instance_of(obj, c) != 0;
}

RB_VALUE Common::GetcObject() const {
    return rb_cObject;
}

std::vector<std::function<void()>>& Common::GetModuleRegistry() {
    return module_registry;
}

void Common::RegisterRectModule() {
    Common::Get()->InternalRectCallback();
    for (auto f : Common::Get()->GetModuleRegistry()) {
        f();
    }
}

void Common::InternalRectCallback() {
    O_RegisterRectModule();
}

} // namespace Ruby
