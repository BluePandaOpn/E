# color

Paquete: `color`

Libreria avanzada de colores para E++ con enfoque de uso ultra simple estilo Python.

## Que trae esta version

- API clasica compatible (`rojo`, `verde`, `alerta`, etc.).
- API nueva de alto nivel (`colorize`, `print_color`, `Color()`).
- Soporte RGB TrueColor ANSI (`rgb`, `bg_rgb`, `rgb_code`, `bg_rgb_code`).
- Helpers semanticos para logs (`error`, `success`, `warning`).
- Punto de extension para C/C++ con `loadlib`.

## Estructura modular

- `__init__.epp`: fachada publica del paquete.
- `constants.epp`: codigos ANSI base.
- `color.epp`: clase `ConsolaColor`.
- `palette.epp`: API clasica con instancia global.
- `styles.epp`: estilos semanticos clasicos.
- `rgb.epp`: funciones RGB/TrueColor.
- `names.epp`: mapeo de nombres de color a codigos ANSI.
- `compose.epp`: composicion de estilos (`colorize`, `paint`, `highlight`).
- `printing.epp`: salida directa por consola.
- `semantic.epp`: helpers semanticos para logs.
- `api.epp`: clase `ColorAPI` y constructor `Color()`.
- `easy.epp`: capa de compatibilidad/reexport de API facil.
- `runtime.epp`: runtime actual + puente a extensiones nativas.

## Uso rapido

```epp
import color

print(rojo("Error clasico"))
print(success("Operacion OK"))
print_rgb("RGB real", 120, 40, 255)
```

## API estilo Python

### Colorear texto por nombre

```epp
import color

print(colorize("Hola", "green", "", true, false, false))
print(paint("Solo color", "cyan"))
print(highlight("Texto + fondo", "white", "blue"))
```

### Impresion directa

```epp
import color

print_color("mensaje", "magenta")
print_style("titulo", "white", "blue", true, false, false)
```

### RGB TrueColor

```epp
import color

print(rgb("Texto RGB", 12, 200, 90))
print(bg_rgb("Fondo RGB", 20, 20, 20))
print(rgb_style("FG/BG RGB", 255, 200, 0, 20, 20, 20))
```

Notas:

- `clamp255(v)` limita cualquier valor a `0..255`.
- `rgb_code` y `bg_rgb_code` devuelven solo el codigo ANSI (sin texto).

## API semantica para logs

```epp
import color

log_error("fallo de conexion")
log_success("deploy terminado")
log_warning("latencia alta")
```

## Clase de alto nivel

```epp
import color

var c = Color()
c.out("hola", "green")
print(c.text_rgb("ok", 80, 220, 120))
```

## Compatibilidad mantenida

Todo lo anterior sigue funcionando:

- `rojo`, `verde`, `azul`, `amarillo`, `magenta`, `cyan`, `blanco`
- `bg_rojo`, `bg_verde`, `bg_azul`, `bg_amarillo`, `bg_magenta`, `bg_cyan`, `bg_blanco`
- `negrita`, `subrayado`, `tenue`, `alerta`, `exito`, `info`
- `ConsolaColor`, `init`, `instancia`, `colorear`, `estilo`

## Extender con C/C++ (alto rendimiento)

Cuando una capacidad no exista en E++, puedes moverla a nativo:

```epp
import color

var ok = load_native_backend("build/color_native.dll")
print("cargado:", ok)
```

Funciones de runtime:

- `native_runtime()`
- `native_builtin_mode()`
- `load_native_backend(path)`
- `native_extension_hint()`

### Recomendacion para extension nativa

Implementa en C/C++:

- generador de gradientes grandes,
- parseo HEX avanzado,
- conversiones masivas de color,
- render de paletas precomputadas.

## Consejos de mantenimiento

- Nuevos codigos ANSI: agregar en `constants.epp`.
- Nuevas funciones de usuario final: reexportar en `__init__.epp`.
- Evitar imports circulares entre modulos.

## Diagnostico

- `epp doctor --imports`
