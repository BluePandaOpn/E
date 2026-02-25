#include "epp_native.h"

static double cpp_mul(const double* args, int argc) {
    if (argc < 2) return 0.0;
    return args[0] * args[1];
}

static double cpp_pow2(const double* args, int argc) {
    if (argc < 1) return 0.0;
    return args[0] * args[0];
}

extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
int epp_register(EppNativeEntry* out_entries, int max_entries) {
    if (!out_entries || max_entries < 2) return 0;
    out_entries[0].name = "cpp_mul";
    out_entries[0].arity = 2;
    out_entries[0].fn = cpp_mul;

    out_entries[1].name = "cpp_pow2";
    out_entries[1].arity = 1;
    out_entries[1].fn = cpp_pow2;
    return 2;
}
}
