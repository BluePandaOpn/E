# Arquitectura de `tkinter`

## Objetivo
Separar claramente:
- API E++ publica
- implementacion E++ interna
- puente nativo C++
- contratos/documentacion

## Estructura
- `src/epp/`: implementacion real E++
- `src/cpp/`: puente C++ para backend nativo
- `_native/`: contrato esperado por runtime/bindings
- `scripts/`: automatizacion administrativa
- `docs/`: documentacion funcional y tecnica

## Capas
1. API publica (raiz):
   - `__init__.epp`
   - `tk.epp`
   - `core.epp`
   - `helpers.epp`
   - `runtime.epp`
2. Implementacion E++ (`src/epp`):
   - `core.epp`: clase `TkApp` y operaciones base
   - `helpers.epp`: helpers, `TkNamespace`, API tipo Tk de Python
   - `runtime.epp`: metadata del runtime
   - `tk.epp`: exporta `var tk = TkNamespace()`
3. Nativo (`src/cpp`):
   - `tk_native_bridge.hpp`: contrato de funciones `tkinter_*`
   - `tk_native_bridge.cpp`: backend C++ en memoria con:
     - ventana + widgets (`Label/Button/Entry`)
     - lectura/escritura de texto (`get/set`)
     - tema de color (`bg/fg/accent`)
     - parser SCSS simple (variables `$x` + bloques `window/button`)
     - ciclo de interaccion basico por consola (`mainloop`)

## Patron recomendado
1. Crear pantalla: `tk.screen(...)`
2. Construir formulario con `input_line` y `output_line`
3. Aplicar tema: `theme(...)` y opcional `apply_scss(...)`
4. Ejecutar `mainloop`
5. Consultar accion con `clicked(...)`

## Import recomendado
```epp
import tkinter

var ui = tk.Tk()
```
