# Lenguaje (Sintaxis y Semantica)

## Tipos y literales

- Numero: `1`, `3.14`
- Texto: `"hola"`
- Booleano: `true`, `false`
- Nulo: `null`
- Lista: `[1, 2, 3]`, `[]`

## Variables

```epp
var x = 10
var nombre = "ana"
var vacio
```

## Operadores

- Aritmetica: `+ - * /`
- Comparacion: `== != > >= < <=`
- Logicos: `and or not` (y `!` como negacion)
- Asignacion: `=`

## Control de flujo

### `if/else` estilo bloque con llaves

```epp
if x > 0 {
    print("positivo")
} else {
    print("no positivo")
}
```

### `if/else` estilo `:` + `end`

```epp
if x > 0:
    print("positivo")
else:
    print("no positivo")
end
```

### `while`

```epp
var i = 0
while i < 3:
    print(i)
    i = i + 1
end
```

## Funciones

```epp
func suma(a, b) {
    return a + b
}
```

Tambien se permite `:` + `end`:

```epp
func suma(a, b):
    return a + b
end
```

## Llamadas y acceso a propiedades

```epp
var r = suma(2, 3)
obj.campo = 10
print(obj.campo)
```

## Listas (`[]`) estilo Python

```epp
var datos = [10, 20, 30]
print(datos[0])     // 10
print(datos[-1])    // 30

datos[1] = 25
datos.append(40)
print(datos.len())  // 4
print(datos.pop())  // 40
datos.clear()
```

## Comentarios

- Linea: `// comentario`
- Bloque: `/* comentario */`

## Limitaciones actuales (importantes)

- No hay `for` nativo en parser.
- No hay diccionarios/mapas nativos como primitivo de lenguaje.
- El frontend de parser/lexer es pequeno y estricto: errores sintacticos caen en `E-PARSE-*`.
