# API rapida

## Namespace recomendado
`tk`

## Creacion de app
- `tk.Tk()`
- `tk.app(title)`
- `tk.window(title)`
- `tk.create_window(title, width, height)`

## Ventana
- `tk.title(ui, text)`
- `tk.geometry(ui, width, height)`
- `tk.show(ui)`
- `tk.mainloop(ui)`

## Widgets
- `tk.Label(ui, text, x, y, w, h)`
- `tk.Button(ui, text, x, y, w, h)`
- `tk.label_line(ui, text)`
- `tk.button_line(ui, text)`

## Utilidades
- `tk.available()`
- `tk.can_use()`
- `tk.runtime()`
- `tk.builtin_mode()`

## Ejemplo
```epp
import tkinter

var ui = tk.app("Panel")
if ui.ensure(640, 360) {
    tk.label_line(ui, "Estado: listo")
    var b = tk.button_line(ui, "Cerrar")
    tk.mainloop(ui)
}
```
