# _native

Libreria base de runtime para E++, pensada para funcionar en modo builtin (sin `loadlib`).

## Estado
- Version del modulo: `2.1.0`
- API: `2026.1`
- Implementacion: `100% E++`

## Estructura recomendada
- `__init__.epp`: entrada publica del paquete.
- `_native.epp`: capa de compatibilidad legacy.
- `src/`: fuente modular de la libreria.
- `scripts/`: automatizacion y validaciones.
- `docs/`: documentacion tecnica.
- `archives/`: historial o respaldos de soporte.

## API publica
- `builtin_mode()`
- `runtime_name()`
- `module_name()`
- `module_version()`
- `api_version()`
- `is_stable()`
- `build_target()`
- `health_check()`
- `diagnostics_hint()`
- `capabilities_text()`

## Uso rapido
```epp
import _native

print(_native.module_name())
print(_native.module_version())
print(_native.health_check())
```

## Validacion
Ejecuta:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/validate.ps1
```

## Diagnostico
- Imports: `epp doctor --imports`
- Reparacion DID (si aplica): `did repair`

