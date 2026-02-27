# Tests / ejemplos

Archivo de ejemplo:
- `example_tk.epp`
- `example_tk_advanced.epp`
- `theme.scss`

Uso:
1. Importa la libreria con `import tkinter`
2. Usa `example_tk.epp` para flujo base input + salida
3. Usa `example_tk_advanced.epp` para formulario completo con tema SCSS
4. Carga estilo con `tk.apply_scss(ui, "libs/tkinter/test/theme.scss")`
5. Ejecuta `mainloop`, captura el click con `tk.clicked(ui, btn_id)` y procesa la salida
