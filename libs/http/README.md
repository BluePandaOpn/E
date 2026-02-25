# http

Paquete: `stdlib.http`

## Objetivo
Libreria estandar para E++ ubicada en `lib/libs/http`.

## Integracion C++
Si. Esta libreria usa `_native` (backend C++ del runtime).

## API publica (funciones)
- `init(...)`
- `header(...)`
- `send(...)`
- `text(...)`
- `json(...)`
- `html(...)`
- `send_status(...)`
- `close(...)`
- `not_found(...)`
- `bad_request(...)`
- `listen(...)`
- `ok(...)`
- `accept_client(...)`
- `accept(...)`
- `reopen(...)`
- `server(...)`
- `create_server(...)`
- `request(...)`
- `response(...)`
- `get(...)`
- `serve_once(...)`
- `serve_once_status(...)`
- `serve(...)`
- `serve_status(...)`
- `serve_forever(...)`
- `serve_forever_status(...)`
- `text_ok(...)`
- `json_ok(...)`
- `html_ok(...)`
- `native_runtime(...)`
- `native_builtin_mode(...)`

## Clases
- `HTTPRequest`
- `HTTPResponse`
- `HTTPServer`

## Uso rapido
```epp
import stdlib.http
```

## Archivos
- `__init__.epp`: entrada del paquete.
- `*.epp`: implementacion adicional de la libreria.

## Notas
- Para diagnosticar carga de librerias usa: `epp doctor --imports`.
- Para reparar paquetes DID con estructura invalida usa: `did repair`.

