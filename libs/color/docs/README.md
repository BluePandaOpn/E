# Color Library (`stdlib.color`)

Libreria de colores para consola en E++.

## Uso principal

```epp
import color

console_init()

var Azul = rgb(6, 63, 250)
print(Azul("Hola"))
```

## Reglas de uso optimizadas

- `rgb(r, g, b)` devuelve una funcion aplicadora de color.
- `bg_rgb(r, g, b)` devuelve una funcion aplicadora de fondo.
- `print(...)` es la salida principal.

## Ejemplos

### Foreground reusable

```epp
var Verde = rgb(20, 220, 80)
print(Verde("Sesion iniciada"))
```

### Background reusable

```epp
var FondoRojo = bg_rgb(180, 20, 20)
print(FondoRojo("Peligro"))
```

### Combinacion avanzada

```epp
var Azul = rgb(6, 63, 250)
print(paint_codes("Titulo", fg_code("white"), bg_rgb_code(20, 20, 20)))
print(Azul("Detalle"))
```

## API principal

- `rgb(r, g, b)` -> funcion colorizable
- `bg_rgb(r, g, b)` -> funcion colorizable de fondo
- `paint(text, fg)`
- `highlight(text, fg, bg)`
- `rgb_style(text, fr, fg, fb, br, bg, bb)`
- `paint_code(text, fg_ansi)`
- `paint_codes(text, fg_ansi, extra_ansi)`
- `rgb_code(r, g, b)`
- `bg_rgb_code(r, g, b)`
- `strip_ansi(text)`
- `console_init()` / `console_setup()`
- `stdout_is_console()` / `supports_ansi()`

## Compatibilidad heredada

- `printc(...)`, `print_with(...)` y `print_with_style(...)` siguen disponibles.
- `rgb_color(...)` y `bg_color(...)` siguen disponibles para codigo ANSI directo.

## Notas

- Runtime nativo usado: `native_std_cpp`.
- Soporte completo 24-bit via RGB (`0..255` por canal).
