# stdlib.math

Libreria matematica para E++ con base modular (`src/`), documentacion completa (`docs/`) y utilidades de integracion (`scripts/`).

## Estructura recomendada
```text
math/
  __init__.epp
  math.epp
  src/
    main.epp
    math.epp
  scripts/
    math-native-tools.psm1
  docs/
    README.es.md
    API.md
    CPLUSPLUS_INTEGRATION.md
```

## Uso rapido
```epp
import stdlib.math
```

## Entrada principal
- `__init__.epp` exporta toda la API publica.
- `src/main.epp` actua como punto de entrada interno.
- `src/math.epp` contiene la implementacion real.

## Compatibilidad
- `math.epp` en raiz se mantiene como wrapper para proyectos existentes.

## Documentacion
- Guia general: `docs/README.es.md`
- API completa: `docs/API.md`
- Integracion con runtime C++: `docs/CPLUSPLUS_INTEGRATION.md`

## Script PowerShell
- `scripts/math-native-tools.psm1` incluye funciones para validar la estructura y generar un esqueleto de bridge C++.

## Ejemplo integrado
### 1) Codigo E++
```epp
import stdlib.math

func main() {
    var radio = 5
    var area = stdlib.math.pi * stdlib.math.pow(radio, 2)
    var diag = stdlib.math.hypot2(3, 4)
    var ok = stdlib.math.is_close(diag, 5, 0.000001, 0.000001)

    print("area = " + area)
    print("diag = " + diag)
    print("is_close = " + ok)
}
```

### 2) Generar stub C++ del bridge nativo
```powershell
Import-Module .\scripts\math-native-tools.psm1 -Force
New-MathNativeBridgeStub -OutFile .\math_bridge_stub.cpp
```

### 3) Validar estructura de la libreria
```powershell
Test-MathLibLayout
```

