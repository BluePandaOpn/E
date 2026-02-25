# tkinter

Paquete: `stdlib.tkinter`

## Objetivo
UI nativa para E++ usando backend C++ (Win32 en Windows).

## Integracion C++
Si. Esta libreria usa `_native` y funciones `tkinter_*` implementadas en el runtime C++.

## API principal
- `available()`
- `Tk()`
- `app(title)`
- `create_window(title, width, height)`
- `demo(title)`

### Clase `TkApp`
- `ok()`
- `create(width, height)`
- `title(text)`
- `label(text, x, y, w, h)`
- `button(text, x, y, w, h)`
- `button_clicked(button_id)`
- `mainloop()`

## Ejemplo
```epp
import stdlib.tkinter

var ui = app("Mi ventana E++")
if ui.create(720, 480) {
    ui.label("Hola desde C++ Win32", 16, 16, 260, 24)
    var b = ui.button("Click", 16, 52, 120, 28)
    ui.mainloop()
}
```

## Notas
- En Windows debe devolver `available() == true`.
- En plataformas sin backend GUI nativo disponible, devuelve `false`.
