# _native contract

Este directorio documenta el contrato nativo que consume la capa E++.

## Funciones requeridas por `src/epp/core.epp`
- `tkinter_available() -> int`
- `tkinter_window_create(title, width, height) -> int`
- `tkinter_window_set_title(window_id, title) -> int`
- `tkinter_label_add(window_id, text, x, y, w, h) -> int`
- `tkinter_button_add(window_id, text, x, y, w, h) -> int`
- `tkinter_button_clicked(window_id, button_id) -> int`
- `tkinter_window_show(window_id) -> void`
- `tkinter_mainloop(window_id) -> int`

## Runtime helpers requeridos por `src/epp/runtime.epp`
- `_native.runtime_name() -> string`
- `_native.builtin_mode() -> string`

Puedes tomar como base `src/cpp/tk_native_bridge.hpp` y `src/cpp/tk_native_bridge.cpp`.
