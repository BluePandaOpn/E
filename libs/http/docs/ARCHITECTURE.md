# stdlib.http architecture

## Objetivo
Separar la libreria HTTP en una capa EPP y una capa nativa C++ para mantener un esquema 50/50.

## Capas
- `http.epp`: punto de entrada y compatibilidad.
- `src/http_core.epp`: API principal en EPP (request/response/server).
- `src/native/http_bridge.cpp`: puente nativo C++ para integracion runtime.
- `scripts/http-tools.psm1`: validacion y empaquetado.

## Flujo
1. Usuario importa `stdlib.http`.
2. `__init__.epp` exporta la API de `http.epp`.
3. `http.epp` reexporta `src/http_core.epp`.
4. `src/http_core.epp` usa funciones HTTP nativas del runtime.
