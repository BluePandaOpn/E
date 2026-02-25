# CLI

## `epp`

Comandos principales:

```powershell
epp run archivo.epp
epp check archivo.epp
epp compile archivo.epp salida.exe --backend cpp --opt 2
epp -V
epp doctor
```

## `did`

Comandos principales:

```powershell
did install
did install <lib>
did list
did stdlib
did remove <lib>
did doctor
```

## Diagnostico rapido

1. Ejecuta binario por ruta absoluta para evitar conflictos de `PATH`.
2. Usa `did --help` para validar version efectiva.
3. Si falla import, revisa rutas de paquetes (`EPP_PACKAGES`, `EPP_HOME`).

## Referencias

- [`E++/docs/04-cli.md`](../../E++/docs/04-cli.md)
- [`E++/README.md`](../../E++/README.md)
