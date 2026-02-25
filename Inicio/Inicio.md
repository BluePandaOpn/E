# Inicio

## Objetivo

Levantar el entorno de Leng y validar que el runtime y la CLI funcionan.

## Build rapido

```powershell
cmake -S E++ -B E++/build
cmake --build E++/build --config Release --target epp
cmake --build E++/build --config Release --target did
```

## Probar un archivo

```powershell
E++/build/Release/epp.exe run ejemplo.epp
E++/build/Release/epp.exe check ejemplo.epp
```

## Flujo sugerido

1. Escribe un archivo `.epp` minimo.
2. Ejecuta `check` para validar sintaxis y parseo.
3. Ejecuta `run` para validar runtime.
4. Si usas paquetes, instala con `did`.

## Archivos clave para arrancar

- [`README.md`](../../README.md)
- [`E++/README.md`](../../E++/README.md)
- [`docs/CLI/CLI.md`](../CLI/CLI.md)
- [`docs/Stdlib/Stdlib.md`](../Stdlib/Stdlib.md)
