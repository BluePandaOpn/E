# Contribuir

## Flujo recomendado

1. Crea rama corta por cambio.
2. Haz commits pequenos y atomicos.
3. Compila `epp`/`did`.
4. Ejecuta checks del archivo tocado.
5. Actualiza `docs/` si cambia API o comportamiento.

## Checklist minimo

```powershell
cmake -S E++ -B E++/build
cmake --build E++/build --config Release --target epp
cmake --build E++/build --config Release --target did
E++/build/Release/epp.exe check archivo.epp
```

## Buenas practicas

- Mantener mensajes de error con codigo estable.
- Documentar cambios de CLI y stdlib.
- No mezclar refactor grande con cambio funcional en el mismo PR.

## Referencias

- [`E++/CONTRIBUTING.md`](../../E++/CONTRIBUTING.md)
- [`E++/docs/07-contribuir.md`](../../E++/docs/07-contribuir.md)
