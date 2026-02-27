# stdlib.http API

## Clases
- `HTTPRequest(client_id)`
- `HTTPResponse(client_id)`
- `HTTPServer(port)`

## Funciones
- `server(port)`
- `create_server(port)`
- `request(client_id)`
- `response(client_id)`
- `serve_once(port, body)`
- `serve_once_status(port, body, status)`
- `serve(port, body, max_requests)`
- `serve_status(port, body, max_requests, status)`
- `serve_forever(port, body)`
- `serve_forever_status(port, body, status)`
- `text_ok(client_id, body)`
- `json_ok(client_id, body)`
- `html_ok(client_id, body)`
- `module_name()`
- `module_version()`
- `native_runtime()`
- `native_builtin_mode()`

## Utilidades PowerShell
- `Invoke-HttpLibCheck`
- `Invoke-HttpLibPackage`
