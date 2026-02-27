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
   - `helpers.epp`: helpers y `TkNamespace`
   - `runtime.epp`: metadata del runtime
   - `tk.epp`: exporta `var tk = TkNamespace()`
3. Nativo (`src/cpp`):
   - encabezado + stub de funciones `tkinter_*`

## Import recomendado
```epp
import tkinter

var ui = tk.Tk()
```
