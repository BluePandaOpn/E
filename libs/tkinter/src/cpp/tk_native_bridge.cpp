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
int tkinter_entry_add(int window_id, const char* text, int x, int y, int w, int h) {
    (void)window_id;
    (void)text;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    return -1;
}
int tkinter_widget_set_text(int window_id, int widget_id, const char* text) {
    (void)window_id;
    (void)widget_id;
    (void)text;
    return 0;
}
const char* tkinter_widget_get_text(int window_id, int widget_id) {
    (void)window_id;
    (void)widget_id;
    return "";
}
int tkinter_window_set_bg(int window_id, const char* color_hex) {
    (void)window_id;
    (void)color_hex;
    return 0;
}
int tkinter_window_set_fg(int window_id, const char* color_hex) {
    (void)window_id;
    (void)color_hex;
    return 0;
}
int tkinter_window_set_accent(int window_id, const char* color_hex) {
    (void)window_id;
    (void)color_hex;
    return 0;
}
int tkinter_window_apply_scss(int window_id, const char* scss_path) {
    (void)window_id;
    (void)scss_path;
    return 0;
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
