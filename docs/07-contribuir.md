# Contribuir

## Flujo recomendado

1. Crear rama corta por feature/fix.
2. Hacer cambios pequenos y atomicos.
3. Ejecutar checks locales.
4. Actualizar docs si cambia API/comportamiento.
5. Abrir PR con contexto tecnico.

## Checklist minimo antes de PR

- `cmake --build build --config Release --target epp`
- `cmake --build build --config Release --target did`
- `build/Release/epp.exe check <archivo_modificado.epp>`
- Validar salida de errores si tocaste parser/runtime/compiler.

## Convenciones practicas

- Mensajes de error claros con hint accionable.
- Evitar features grandes sin tests manuales reproducibles.
- Mantener compatibilidad de CLI salvo versionado explicito.
- No mezclar refactor masivo con cambio funcional en un mismo PR.

## Donde documentar cambios

- API de lenguaje: `docs/01-lenguaje.md`, `docs/02-paquetes-modulos.md`
- OOP: `docs/03-clases-objetos.md`
- CLI: `docs/04-cli.md`
- Errores: `docs/errors/*`

