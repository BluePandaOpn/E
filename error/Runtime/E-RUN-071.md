# E-RUN-071

## Fase

Runtime

## Causa

No se pudo cargar libreria compartida.

## Accion recomendada

Revisa ruta y dependencias del binario nativo.

## Ejemplo que lo puede disparar

```epp
loadlib("x.dll")
```

## Referencia de implementacion

- `E++/src/core/runtime.cpp`
