# tkinter (tkinter)

Libreria UI para E++ con backend nativo C++.

## Estructura reorganizada
- `src/epp/`: implementacion E++ real
- `src/cpp/`: puente C++ nativo (`tkinter_*`)
- `_native/`: contrato del runtime/bindings
- `docs/`: documentacion tecnica y API
- `scripts/`: scripts admin PowerShell (`.psm1`)
- raiz: fachada publica y compatibilidad

## Import recomendado
```epp
import tkinter

var ui = tk.Tk()
tk.title(ui, "Mi app")
if tk.geometry(ui, 720, 480) {
    tk.Label(ui, "Hola", 16, 16, 200, 24)
    tk.Button(ui, "Cerrar", 16, 52, 120, 28)
    tk.mainloop(ui)
}
```

`tk` se exporta desde `tk.epp` y se incluye en `__init__.epp`.

## Archivos clave
- `__init__.epp`: entrada del paquete (`import tkinter`)
- `tk.epp`: namespace corto `tk`
- `core.epp`, `helpers.epp`, `runtime.epp`: wrappers compatibles
- `src/epp/*.epp`: logica real
- `src/cpp/tk_native_bridge.*`: base C++ para integrar `_native`

## Documentacion
- `docs/ARCHITECTURE.md`
- `docs/API.md`
- `_native/README.md`

## Script de administracion
Modulo: `scripts/tk-admin.psm1`

Funciones:
- `Get-TkinterTree`
- `Test-TkinterLayout`
