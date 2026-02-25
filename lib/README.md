# Librerias Nativas (C/C++)

E++ puede cargar librerias nativas con:

```epp
loadlib("lib/native_math_c.dll")
```

## API de integracion

Incluye `include/epp_native.h` y exporta:

```c
int epp_register(EppNativeEntry* out_entries, int max_entries);
```

Cada entrada registra nombre, aridad y puntero de funcion.

API nueva recomendada (soporta `numero`, `bool`, `texto`, `null`):

```c
int epp_register_v2(EppNativeEntryV2* out_entries, int max_entries);
```

El runtime intenta `epp_register_v2` primero y, si no existe, cae a `epp_register`.

## Ejemplo C (Windows)

```bash
cl /LD /I include lib/native_math_c.c /link /OUT:lib/native_math_c.dll
```

## Ejemplo C++ (Windows)

```bash
cl /LD /EHsc /I include lib/native_math_cpp.cpp /link /OUT:lib/native_math_cpp.dll
```

Libreria estandar base:

```bash
cl /LD /EHsc /I include lib/native_std_cpp.cpp /link /OUT:lib/native_std_cpp.dll
```

## Uso desde E++

```epp
loadlib("lib/native_math_c.dll")
print(c_add(5, 7))
```

Con stdlib:

```epp
import "libs/stdlib/time.epp"
import "libs/stdlib/math.epp"
print("epoch " + time())
print("sqrt(81) " + sqrt(81))
```

Notas:

- `epp_register` (v1) solo maneja `numero`.
- `epp_register_v2` maneja `numero`, `bool`, `texto`, `null`.
- Para C++ usa wrappers `extern "C"` para exportar funciones de registro.
