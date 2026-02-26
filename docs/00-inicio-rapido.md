# Inicio Rapido

## 1. Compilar herramientas

```powershell
cmake -S . -B build
cmake --build build --config Release --target epp
cmake --build build --config Release --target did
```

## 2. Ejecutar y validar un programa

```powershell
build/Release/epp.exe run ejemplo.epp
build/Release/epp.exe check ejemplo.epp
```

Tambien existe el modo directo:

```powershell
build/Release/epp.exe ejemplo.epp
build/Release/epp.exe ejemplo.epp --check
```

## 3. Instalar entorno de paquetes con DID

```powershell
build/Release/did.exe install
build/Release/did.exe stdlib
build/Release/did.exe install time
build/Release/did.exe list
```

Notas:
- `did` usa por defecto el repo `BluePandaOpn/E`, rama `Lib`.
- Si una libreria esta en la lista estandar, se toma de `libs/<lib>`.
- Si no es estandar, se busca en `Extra/<lib>`.

## 4. Primer archivo E++

```epp
import stdlib.time

func main() {
    print("hola desde E++")
    print(stdlib.time.now())
}

main()
```
