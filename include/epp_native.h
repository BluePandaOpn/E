#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*EppNativeFn)(const double* args, int argc);

typedef struct EppNativeEntry {
    const char* name;
    int arity; // -1 variadica
    EppNativeFn fn;
} EppNativeEntry;

typedef enum EppNativeType {
    EPP_NATIVE_NULL = 0,
    EPP_NATIVE_NUMBER = 1,
    EPP_NATIVE_BOOL = 2,
    EPP_NATIVE_STRING = 3
} EppNativeType;

typedef struct EppNativeValue {
    int type; // EppNativeType
    double number_value;
    int bool_value; // 0/1
    const char* string_value; // runtime copies string immediately
} EppNativeValue;

typedef EppNativeValue (*EppNativeFnV2)(const EppNativeValue* args, int argc);

typedef struct EppNativeEntryV2 {
    const char* name;
    int arity; // -1 variadica
    EppNativeFnV2 fn;
} EppNativeEntryV2;

// Debe retornar la cantidad real registrada en out_entries.
// max_entries limita cuantas puede escribir.
typedef int (*EppRegisterFn)(EppNativeEntry* out_entries, int max_entries);
typedef int (*EppRegisterFnV2)(EppNativeEntryV2* out_entries, int max_entries);

#ifdef __cplusplus
}
#endif
