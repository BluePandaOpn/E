#pragma once

// Contrato minimo esperado por la capa E++ para stdlib.tkinter.
// Integra estas firmas con tu runtime real (_native).
extern "C" {
int tkinter_available();
int tkinter_window_create(const char* title, int width, int height);
int tkinter_window_set_title(int window_id, const char* title);
int tkinter_label_add(int window_id, const char* text, int x, int y, int w, int h);
int tkinter_button_add(int window_id, const char* text, int x, int y, int w, int h);
int tkinter_button_clicked(int window_id, int button_id);
void tkinter_window_show(int window_id);
int tkinter_mainloop(int window_id);
}
