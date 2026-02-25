# Stdlib

La stdlib activa vive en `lib/libs`.

## Modulos disponibles

- `_native`: puente a runtime nativo.
- `asyncio`: utilidades de espera simple.
- `collections`: helpers de pares/tuplas.
- `color`: estilos ANSI para terminal.
- `csv`: helpers de filas y encabezados CSV.
- `datetime`: fecha/hora ISO.
- `http`: servidor HTTP basico.
- `json`: quoting y parse numerico.
- `math`: funciones matematicas base.
- `os`: rutas y sistema de archivos.
- `random`: aleatorios y probabilidad.
- `sqlite3`: helper de conexion SQLite.
- `sys`: informacion de plataforma/runtime.
- `threading`: informacion de concurrencia.
- `time`: reloj y sleep.
- `tkinter`: soporte GUI (segun runtime).
- `urllib`: encoding URL.

## Lectura por modulo (README)

- [lib/libs/README.md](../../lib/libs/README.md)
- [lib/libs/_native/README.md](../../lib/libs/_native/README.md)
- [lib/libs/asyncio/README.md](../../lib/libs/asyncio/README.md)
- [lib/libs/collections/README.md](../../lib/libs/collections/README.md)
- [lib/libs/color/README.md](../../lib/libs/color/README.md)
- [lib/libs/csv/README.md](../../lib/libs/csv/README.md)
- [lib/libs/datetime/README.md](../../lib/libs/datetime/README.md)
- [lib/libs/http/README.md](../../lib/libs/http/README.md)
- [lib/libs/json/README.md](../../lib/libs/json/README.md)
- [lib/libs/math/README.md](../../lib/libs/math/README.md)
- [lib/libs/os/README.md](../../lib/libs/os/README.md)
- [lib/libs/random/README.md](../../lib/libs/random/README.md)
- [lib/libs/sqlite3/README.md](../../lib/libs/sqlite3/README.md)
- [lib/libs/sys/README.md](../../lib/libs/sys/README.md)
- [lib/libs/threading/README.md](../../lib/libs/threading/README.md)
- [lib/libs/time/README.md](../../lib/libs/time/README.md)
- [lib/libs/tkinter/README.md](../../lib/libs/tkinter/README.md)
- [lib/libs/urllib/README.md](../../lib/libs/urllib/README.md)

## Ejemplo

```epp
import stdlib.time
import stdlib.math

print(stdlib.time.now())
print(stdlib.math.sqrt(9))
```

## Referencia base

- [E++/docs/05-stdlib.md](../../E++/docs/05-stdlib.md)
