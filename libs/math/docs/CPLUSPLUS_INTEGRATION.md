# Integracion C++ para math

La libreria usa funciones del runtime nativo mediante `_native`.

## Flujo recomendado
1. Implementar funciones `math_*` en el backend C++ del runtime.
2. Exponerlas al entorno E++.
3. Consumirlas desde `src/math.epp`.

## Simbolos esperados por E++
- `math_pi()`
- `math_e()`
- `math_sqrt(x)`
- `math_pow(a, b)`
- `math_floor(x)`
- `math_ceil(x)`
- `math_sin(x)`
- `math_cos(x)`
- `math_tan(x)`
- `math_log(x)`
- `math_exp(x)`

## Convencion sugerida en C++
```cpp
double math_pi();
double math_e();
double math_sqrt(double x);
double math_pow(double a, double b);
```

## Verificacion
Usa el modulo PowerShell `scripts/math-native-tools.psm1` para validar estructura y generar un archivo base para el bridge C++.

