# Color Library (`stdlib.color`)

Libreria de colores para consola en E++.

## Uso

```epp
import stdlib.color

stdlib.color.console_init()
stdlib.color.printc(stdlib.color.rgb("Hola", 0, 255, 170))
stdlib.color.printc(stdlib.color.highlight("Alerta", "white", "red"))
```

## API principal

- `paint(text, fg)`
- `highlight(text, fg, bg)`
- `rgb(text, r, g, b)`
- `bg_rgb(text, r, g, b)`
- `rgb_style(text, fr, fg, fb, br, bg, bb)`
- `strip_ansi(text)`
- `console_init()`
- `stdout_is_console()`

## Notas

- Runtime nativo usado: `native_std_cpp`.
- Soporte completo 24-bit via RGB (`0..255` por canal).
