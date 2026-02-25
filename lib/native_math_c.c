#include "epp_native.h"

static double c_add(const double* args, int argc) {
    if (argc < 2) return 0.0;
    return args[0] + args[1];
}

static double c_sub(const double* args, int argc) {
    if (argc < 2) return 0.0;
    return args[0] - args[1];
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int epp_register(EppNativeEntry* out_entries, int max_entries) {
    if (!out_entries || max_entries < 2) return 0;
    out_entries[0].name = "c_add";
    out_entries[0].arity = 2;
    out_entries[0].fn = c_add;

    out_entries[1].name = "c_sub";
    out_entries[1].arity = 2;
    out_entries[1].fn = c_sub;
    return 2;
}
