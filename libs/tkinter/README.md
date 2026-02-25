# tkinter

Paquete: `stdlib.tkinter`

## Objetivo
UI nativa para E++ usando backend C++ (Win32 en Windows).

## Integracion C++
Si. Esta libreria usa `_native` y funciones `tkinter_*` implementadas en el runtime C++.

## Estructura modular
- `__init__.epp`: fachada publica del paquete.
- `tkinter.epp`: alias de compatibilidad que reexporta la API.
- `core.epp`: disponibilidad + clase `TkApp` (nucleo UI).
- `helpers.epp`: factories y helpers de uso rapido.
- `runtime.epp`: utilidades de diagnostico de runtime.

## API principal
- `tk` (uso principal estilo Python)
- `tk.Tk()`
- `tk.app(title)`
- `tk.title(ui, text)`
- `tk.geometry(ui, width, height)`
- `tk.Label(ui, text, x, y, w, h)`
- `tk.Button(ui, text, x, y, w, h)`
- `tk.clicked(ui, button_id)`
- `tk.mainloop(ui)`

### Namespace recomendado `tk`
- `tk.available()`
- `tk.can_use()`
- `tk.Tk()`
- `tk.app(title)`
- `tk.window(title)`
- `tk.create_window(title, width, height)`
- `tk.run(title, width, height)`
- `tk.simple(title, text)`
- `tk.demo(title)`
- `tk.title(ui, text)`
- `tk.geometry(ui, width, height)`
- `tk.label(ui, text, x, y, w, h)`
- `tk.Label(ui, text, x, y, w, h)`
- `tk.button(ui, text, x, y, w, h)`
- `tk.Button(ui, text, x, y, w, h)`
- `tk.click(ui, button_id)`
- `tk.clicked(ui, button_id)`
- `tk.label_line(ui, text)`
- `tk.button_line(ui, text)`
- `tk.show(ui)`
- `tk.mainloop(ui)`

### Clase `TkApp`
- `ok()`
- `created()`
- `create(width, height)`
- `ensure(width, height)`
- `title(text)`
- `set_title(text)`
- `geometry(width, height)`
- `label(text, x, y, w, h)`
- `Label(text, x, y, w, h)`
- `lbl(text, x, y, w, h)`
- `button(text, x, y, w, h)`
- `Button(text, x, y, w, h)`
- `btn(text, x, y, w, h)`
- `button_clicked(button_id)`
- `clicked(button_id)`
- `click(button_id)`
- `show()`
- `line_break(h)`
- `label_line(text)`
- `lbl_line(text)`
- `button_line(text)`
- `btn_line(text)`
- `run(width, height)`
- `mainloop()`

## Ejemplo
```epp
import stdlib.tkinter

var ui = tk.Tk()
tk.title(ui, "Mi ventana E++")
if tk.geometry(ui, 720, 480) {
    tk.Label(ui, "Hola desde C++ Win32", 16, 16, 260, 24)
    var b = tk.Button(ui, "Click", 16, 52, 120, 28)
    tk.mainloop(ui)
}
```

## Ejemplo rapido (API facil)
```epp
import stdlib.tkinter

var ui = tk.app("Panel rapido")
if ui.ensure(640, 360) {
    tk.label_line(ui, "Estado: activo")
    var btn = tk.button_line(ui, "Aceptar")
    tk.mainloop(ui)
}
```

## Ejemplo one-liner
```epp
import stdlib.tkinter

tk.simple("Aviso", "Operacion completada")
```

## Notas
- En Windows debe devolver `available() == true`.
- En plataformas sin backend GUI nativo disponible, devuelve `false`.
- La API nueva mantiene compatibilidad total con la API anterior.
- Puedes importar por capas: `stdlib.tkinter.core`, `stdlib.tkinter.helpers`, `stdlib.tkinter.runtime`.
- Uso recomendado en proyectos nuevos: prefijo unico `tk.*`.
- La API global (`app`, `run`, `simple`, etc.) sigue disponible por compatibilidad.
