# API rapida

## Namespace
`tk`

## Creacion
- `tk.Tk()`
- `tk.app(title)`
- `tk.window(title)`
- `tk.create_window(title, width, height)`
- `tk.screen(title, width, height)`

## Ventana
- `tk.title(ui, text)`
- `tk.geometry(ui, width, height)`
- `tk.show(ui)`
- `tk.mainloop(ui)`
- `tk.interact(ui, button_id)`

## Widgets base
- `tk.Label(ui, text, x, y, w, h)`
- `tk.Button(ui, text, x, y, w, h)`
- `tk.Entry(ui, text, x, y, w, h)`
- `tk.clicked(ui, button_id)`

## Layout por lineas (pantalla rapida)
- `tk.label_line(ui, text)`
- `tk.button_line(ui, text)`
- `tk.button_line_ex(ui, text, w)`
- `tk.entry_line(ui, text)`
- `tk.input_line(ui, label_text, default_text)`
- `tk.print_line(ui, text)`
- `tk.output_line(ui, text)`

## Entrada / salida
- `tk.get(ui, widget_id)`
- `tk.set(ui, widget_id, text)`
- `tk.get_input(ui, widget_id)`
- `tk.set_output(ui, widget_id, text)`

## Estilo
- `tk.bg(ui, color_hex)`
- `tk.fg(ui, color_hex)`
- `tk.accent(ui, color_hex)`
- `tk.theme(ui, bg_hex, fg_hex, accent_hex)`
- `tk.apply_scss(ui, path)`

## Runtime
- `tk.available()`
- `tk.can_use()`
- `tk.runtime()`
- `tk.builtin_mode()`

## Ejemplo
```epp
import tkinter

func main() {
    var ui = tk.screen("Input + Print", 760, 460)
    if not ui.ensure(760, 460) { return }

    tk.theme(ui, "#0f172a", "#e2e8f0", "#38bdf8")
    tk.apply_scss(ui, "libs/tkinter/test/theme.scss")

    var name = tk.input_line(ui, "Nombre:", "Admin")
    var out = tk.output_line(ui, "Salida: listo")
    var ok = tk.button_line(ui, "Procesar")

    tk.mainloop(ui)
    if tk.clicked(ui, ok) {
        tk.set_output(ui, out, "Salida: hola " + tk.get(ui, name))
        print(tk.get(ui, out))
    }
}
```
