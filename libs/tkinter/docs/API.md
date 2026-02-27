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
- `tk.Entry(ui, text, x, y, w, h)`
- `tk.label_line(ui, text)`
- `tk.button_line(ui, text)`
- `tk.entry_line(ui, text)`

## Entrada / salida
- `tk.get(ui, widget_id)`
- `tk.set(ui, widget_id, text)`
- `tk.get_value(ui, widget_id)`
- `tk.set_value(ui, widget_id, text)`

## Estilos y colores
- `tk.bg(ui, color_hex)`
- `tk.fg(ui, color_hex)`
- `tk.accent(ui, color_hex)`
- `tk.apply_scss(ui, path)`

## Utilidades
- `tk.available()`
- `tk.can_use()`
- `tk.runtime()`
- `tk.builtin_mode()`

## Ejemplo
```epp
import tkinter

var root = tk.Tk()
if root.ensure(720, 420) {
    tk.bg(root, "#111827")
    tk.fg(root, "#E5E7EB")
    tk.accent(root, "#22C55E")
    tk.apply_scss(root, "test/theme.scss")

    var out = tk.Label(root, "Salida: listo", 16, 16, 320, 24)
    var inp = tk.Entry(root, "", 16, 48, 300, 28)
    var btn = tk.Button(root, "Procesar", 16, 86, 120, 30)

    tk.set(root, inp, "Fox")
    if tk.clicked(root, btn) {
        tk.set(root, out, "Salida: hola " + tk.get(root, inp))
    }
    tk.mainloop(root)
}
```
