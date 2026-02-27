# tkinter (tkinter)

Libreria UI para E++ con integracion C++ y modo runtime compatible por consola.

## Estructura
- `__init__.epp`: entrada publica (`import tkinter`)
- `core.epp`, `helpers.epp`, `runtime.epp`, `tk.epp`: fachada compatible
- `src/epp/`: implementacion real de la API
- `src/cpp/`: puente C++ (`tkinter_*`) con ventana/widgets/estilo
- `docs/`: arquitectura + API
- `scripts/`: utilidades administrativas
- `test/`: ejemplos y tema SCSS

## Flujo recomendado (input + salida)
```epp
import tkinter

func main() {
    var ui = tk.screen("Demo", 760, 460)
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

## Capacidades
- Widgets: `Label`, `Button`, `Entry`
- I/O: `get`, `set`, `input_line`, `output_line`, `print_line`
- Estilo: `bg`, `fg`, `accent`, `theme`, `apply_scss`
- Integracion C++: backend en `src/cpp/tk_native_bridge.cpp`
