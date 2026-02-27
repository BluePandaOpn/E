# _native contract

Este directorio documenta el contrato nativo que consume la capa E++.

## Funciones requeridas por `src/epp/core.epp`
- `tkinter_available() -> int`
- `tkinter_window_create(title, width, height) -> int`
- `tkinter_window_set_title(window_id, title) -> int`
- `tkinter_label_add(window_id, text, x, y, w, h) -> int`
- `tkinter_button_add(window_id, text, x, y, w, h) -> int`
- `tkinter_entry_add(window_id, text, x, y, w, h) -> int`
- `tkinter_widget_set_text(window_id, widget_id, text) -> int`
- `tkinter_widget_get_text(window_id, widget_id) -> string`
- `tkinter_window_set_bg(window_id, color_hex) -> int`
- `tkinter_window_set_fg(window_id, color_hex) -> int`
- `tkinter_window_set_accent(window_id, color_hex) -> int`
- `tkinter_window_apply_scss(window_id, scss_path) -> int`
- `tkinter_button_clicked(window_id, button_id) -> int`
- `tkinter_window_show(window_id) -> void`
- `tkinter_mainloop(window_id) -> int`

## Runtime helpers requeridos por `src/epp/runtime.epp`
- `_native.runtime_name() -> string`
- `_native.builtin_mode() -> string`

Puedes tomar como base `src/cpp/tk_native_bridge.hpp` y `src/cpp/tk_native_bridge.cpp`.
