#include <string>

// Puente C++ de referencia para integrar stdlib.http con el runtime E++.
// Este archivo documenta la capa nativa esperada por src/http_core.epp.
// El runtime real puede proporcionar estas funciones desde otro modulo.
extern "C" {

const char* http_bridge_name() {
    return "stdlib.http.native";
}

const char* http_bridge_version() {
    return "2.1.0";
}

// Punto de extension para inicializar recursos HTTP nativos.
int http_bridge_init() {
    return 1;
}

// Punto de extension para liberar recursos HTTP nativos.
int http_bridge_shutdown() {
    return 1;
}

}
