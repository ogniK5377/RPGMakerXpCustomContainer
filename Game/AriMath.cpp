#include "AriMath.h"
#include "RubyCommon.h"

namespace RubyModule {
VALUE ARI_MATH{};
constexpr double PI = 3.141592653589793238463;
constexpr double PI2 = PI * 2.0;
constexpr double BOUNCE_CONSTANT = PI2 / 3.0;

static VALUE clamp(VALUE object, VALUE _x, VALUE _min, VALUE _max) {
    auto* common = Ruby::Common::Get();

    auto x = common->GetFloat(_x);
    auto min = common->GetFloat(_min);
    auto max = common->GetFloat(_max);
    if (x->GetValue() > max->GetValue()) {
        return _max;
    } else if (x->GetValue() < min->GetValue()) {
        return _min;
    }
    return _x;
}

static VALUE min(VALUE object, VALUE _lhs, VALUE _rhs) {
    auto* common = Ruby::Common::Get();
    auto lhs = common->GetFloat(_lhs);
    auto rhs = common->GetFloat(_rhs);
    if (lhs->GetValue() > rhs->GetValue()) {
        return _rhs;
    }
    return _lhs;
}

static VALUE max(VALUE object, VALUE _lhs, VALUE _rhs) {
    auto* common = Ruby::Common::Get();
    auto lhs = common->GetFloat(_lhs);
    auto rhs = common->GetFloat(_rhs);
    if (lhs->GetValue() > rhs->GetValue()) {
        return _lhs;
    }
    return _rhs;
}

static VALUE ease_out_elastic(VALUE object, VALUE _t) {
    auto* common = Ruby::Common::Get();
    auto t = common->GetFloat(_t)->GetValue();

    if (t < 0.0) {
        return common->MakeFloat(0.0);
    } else if (t > 1.0) {
        return common->MakeFloat(1.0);
    }

    const double bounce = pow(2.0, (-10.0 * t)) * sin((t * 10.0 - 0.75) * BOUNCE_CONSTANT) + 1.0;
    return common->MakeFloat(bounce);
}

static VALUE lerp(VALUE object, VALUE _v0, VALUE _v1, VALUE _t) {
    auto* common = Ruby::Common::Get();
    const auto v0 = common->GetFloat(_v0)->GetValue();
    const auto v1 = common->GetFloat(_v1)->GetValue();
    const auto t = common->GetFloat(_t)->GetValue();

    return common->MakeFloat((1.0 - t) * v0 + t * v1);
}

static VALUE remap_range(VALUE object, VALUE _a_low, VALUE _a_high, VALUE _b_low, VALUE _b_high,
                         VALUE _x) {
    auto* common = Ruby::Common::Get();
    const auto a_low = common->GetFloat(_a_low)->GetValue();
    const auto a_high = common->GetFloat(_a_high)->GetValue();
    const auto b_low = common->GetFloat(_b_low)->GetValue();
    const auto b_high = common->GetFloat(_a_high)->GetValue();
    const auto x = common->GetFloat(_x)->GetValue();

    return common->MakeFloat(b_low + ((x - a_low) * (b_high - b_low) / (a_high - a_low)));
}

static VALUE norm_to_circle(VALUE object, VALUE _angle) {
    auto* common = Ruby::Common::Get();
    auto angle = common->GetFloat(_angle)->GetValue();

    angle = fmod(angle, 360.0);
    while (angle < 0.0) {
        angle += 360.0;
    }

    return common->MakeFloat(angle);
}

void RegisterAriMath() {
    auto* common = Ruby::Common::Get();
    if (!ARI_MATH) {
        ARI_MATH = common->DefineModule("AriMath");

        // Consts
        common->DefineConst(ARI_MATH, "BOUNCE_CONST", common->MakeFloat(BOUNCE_CONSTANT));
        common->DefineConst(ARI_MATH, "PI", common->MakeFloat(PI));
        common->DefineConst(ARI_MATH, "PI2", common->MakeFloat(PI2));

        // Methods
        common->DefineModuleFunction(ARI_MATH, "clamp", &RubyModule::clamp, 3);
        common->DefineModuleFunction(ARI_MATH, "min", &RubyModule::min, 2);
        common->DefineModuleFunction(ARI_MATH, "max", &RubyModule::max, 2);
        common->DefineModuleFunction(ARI_MATH, "ease_out_elastic", &RubyModule::ease_out_elastic,
                                     1);
        common->DefineModuleFunction(ARI_MATH, "lerp", &RubyModule::lerp, 3);
        common->DefineModuleFunction(ARI_MATH, "remap_range", &RubyModule::remap_range, 5);
        common->DefineModuleFunction(ARI_MATH, "norm_to_circle", &RubyModule::norm_to_circle, 1);
    }
}

} // namespace RubyModule
