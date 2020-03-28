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

        O_RegisterRectModule = MemoryUtil::CreateDetour<RegisterRectModuleType>(
            register_rect_module_addr, reinterpret_cast<uintptr_t>(&Common::RegisterRectModule));

        rb_define_module = MemoryUtil::MakeCallable<RbDefineModuleType>(rb_define_module_addr);
        rb_define_module_function =
            MemoryUtil::MakeCallable<RbDefineModuleFunctionType>(rb_define_module_function_addr);
        rb_define_const = MemoryUtil::MakeCallable<RbDefineConstType>(rb_define_const_addr);
        rb_float_new = MemoryUtil::MakeCallable<RbFloatNewType>(rb_float_new_addr);
        rb_float = MemoryUtil::MakeCallable<RbFloatType>(rb_float_addr);
    }
}

void Common::AddNewModule(std::function<void()> f) {
    module_registry.push_back(f);
}

RB_VALUE Common::DefineModule(const char* module_name) {
    return rb_define_module(module_name);
}

void Common::DefineModuleFunction(RB_VALUE module_id, const char* method_name, void* method,
                                  int argc) {
    rb_define_module_function(module_id, method_name, method, argc);
}

void Common::DefineConst(RB_VALUE module_id, const char* const_name, RB_VALUE value) {
    rb_define_const(module_id, const_name, value);
}

RB_VALUE Common::MakeFloat(double value) {
    return rb_float_new(value);
}

Ruby::Float* Common::GetFloat(RB_VALUE value) {
    return rb_float(value);
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
