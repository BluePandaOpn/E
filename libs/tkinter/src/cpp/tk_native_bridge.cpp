#include "tk_native_bridge.hpp"

// Implementacion base (stub) para organizar el proyecto.
// Reemplaza por backend real Win32/GTK/Qt segun tu runtime.
extern "C" {
int tkinter_available() { return 0; }
int tkinter_window_create(const char* title, int width, int height) {
    (void)title;
    (void)width;
    (void)height;
    return -1;
}
int tkinter_window_set_title(int window_id, const char* title) {
    (void)window_id;
    (void)title;
    return 0;
}
int tkinter_label_add(int window_id, const char* text, int x, int y, int w, int h) {
    (void)window_id;
    (void)text;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    return -1;
}
int tkinter_button_add(int window_id, const char* text, int x, int y, int w, int h) {
    (void)window_id;
    (void)text;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    return -1;
}
int tkinter_button_clicked(int window_id, int button_id) {
    (void)window_id;
    (void)button_id;
    return 0;
}
void tkinter_window_show(int window_id) { (void)window_id; }
int tkinter_mainloop(int window_id) {
    (void)window_id;
    return 0;
}
}
