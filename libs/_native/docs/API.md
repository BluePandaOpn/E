# API de _native

## Funciones

### `builtin_mode() -> bool`
Retorna `true` cuando el runtime es builtin y no requiere carga dinamica.

### `runtime_name() -> string`
Nombre del runtime actual.

### `module_name() -> string`
Nombre del modulo: `_native`.

### `module_version() -> string`
Version del modulo.

### `api_version() -> string`
Version semantica de la API publica.

### `is_stable() -> bool`
Indica si el modulo esta marcado como estable.

### `build_target() -> string`
Define el destino de construccion (`builtin`).

### `health_check() -> string`
Chequeo rapido de estado. Retorna `ok` si la libreria esta operativa.

### `diagnostics_hint() -> string`
Comando recomendado para diagnostico de imports.

### `capabilities_text() -> string`
Lista de capacidades exportadas en formato texto separado por comas.
