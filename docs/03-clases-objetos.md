# Clases y Objetos

## Declaracion de clase

La clase usa llaves y metodos `func`:

```epp
class Persona {
    func init(nombre, edad) {
        this.nombre = nombre
        this.edad = edad
    }

    func saludar() {
        return "Hola, soy " + this.nombre
    }
}
```

## Instanciacion

```epp
var p = Persona("Ana", 28)
print(p.saludar())
```

Reglas relevantes:
- Si la clase recibe argumentos, debe existir `init(...)`.
- Si no hay `init`, solo puede construirse sin argumentos.

## Propiedades

```epp
p.ciudad = "Lima"
print(p.ciudad)
```

Errores comunes:
- Acceder propiedad inexistente: `E-RUN-051`.
- Asignar propiedad sobre algo que no es instancia: `E-RUN-052`.
- Leer propiedad sobre algo que no es instancia: `E-RUN-050`.

## Recomendaciones de diseno

- Mantener `init` como constructor unico de invariantes.
- Exponer metodos cortos y composables.
- Evitar mezclar logica de IO con logica de dominio en la misma clase.

