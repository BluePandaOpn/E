# API de math

## Constantes
- `pi`
- `e`
- `tau`

## Funciones nativas
- `sqrt(x)`
- `pow(a, b)`
- `floor(x)`
- `ceil(x)`
- `sin(x)`
- `cos(x)`
- `tan(x)`
- `log(x)`
- `exp(x)`

## Funciones de utilidad
- `abs(x)`
- `min(a, b)`
- `max(a, b)`
- `clamp(x, low, high)`
- `sign(x)`
- `square(x)`
- `cube(x)`
- `radians(deg)`
- `degrees(rad)`
- `hypot2(x, y)`
- `distance2d(x1, y1, x2, y2)`
- `average2(a, b)`
- `average3(a, b, c)`
- `frac(x)`
- `lerp(a, b, t)`
- `inv_lerp(a, b, value)`
- `map_range(value, in_min, in_max, out_min, out_max)`
- `is_close(a, b, rel_tol, abs_tol)`
- `round(x)`
- `between(x, low, high)`
- `clamp01(x)`
- `normalize_range(value, low, high)`

## Metadata/modo runtime
- `module_name()`
- `module_version()`
- `native_runtime()`
- `native_builtin_mode()`

## Notas
- `radians` y `degrees` ayudan a unificar API trigonometrica.
- `is_close` evita errores comunes de comparacion exacta en flotantes.

