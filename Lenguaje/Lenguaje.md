# Lenguaje

## Tipos basicos

- Numero: `1`, `3.14`
- Texto: `"hola"`
- Booleano: `true`, `false`
- Nulo: `null`
- Lista: `[1, 2, 3]`

## Variables y funciones

```epp
var x = 10

func suma(a, b) {
    return a + b
}
```

## Flujo de control

- `if/else` con llaves
- `if/else` con `:` + `end`
- `while`

## Clases

```epp
class Persona {
    func init(nombre) {
        this.nombre = nombre
    }
}
```

## Import y paquetes

```epp
import stdlib.time
from stdlib.time import now
import "ruta/modulo.epp"
```

## Referencias

- [`docs/Paquetes/Paquetes.md`](../Paquetes/Paquetes.md)
- [`docs/Modulos/Modulos.md`](../Modulos/Modulos.md)
- [`E++/docs/01-lenguaje.md`](../../E++/docs/01-lenguaje.md)
