# http

Paquete: `stdlib.http`

## Objetivo
Libreria HTTP para E++ con estructura fusionada EPP + C++.

## Estructura
- `__init__.epp`: entrada del paquete.
- `http.epp`: entrypoint y compatibilidad.
- `src/http_core.epp`: API principal en EPP.
- `src/native/http_bridge.cpp`: puente nativo C++ (integracion runtime).
- `scripts/http-tools.psm1`: checks y empaquetado.
- `docs/`: documentacion tecnica (`ARCHITECTURE.md`, `API.md`).

## Integracion 50/50
- Capa EPP: modelos HTTP, wrappers y helpers.
- Capa C++: inicializacion/extension nativa para backend runtime.

## Uso rapido
```epp
import stdlib.http
```

## Script PowerShell
```powershell
Import-Module .\scripts\http-tools.psm1
Invoke-HttpLibCheck
Invoke-HttpLibPackage
```

