/* Copyright © 2014 Manuel Bachmann */

#ifndef _WLMESSAGE_H_
#define _WLMESSAGE_H_

struct wlmessage;

enum wlmessage_titlebutton {
	WLMESSAGE_TITLEBUTTON_NONE,
	WLMESSAGE_TITLEBUTTON_MINIMIZE,
	WLMESSAGE_TITLEBUTTON_MAXIMIZE,
	WLMESSAGE_TITLEBUTTON_CLOSE,
	WLMESSAGE_TITLEBUTTON_ALL
};

void
wlmessage_set_title (struct wlmessage *wlmessage, char *title);

char *
wlmessage_get_title (struct wlmessage *wlmessage);

void
wlmessage_set_titlebuttons (struct wlmessage *wlmessage, enum wlmessage_titlebutton titlebuttons);

enum wlmessage_titlebutton
wlmessage_get_titlebuttons (struct wlmessage *wlmessage);

void
wlmessage_set_noresize (struct wlmessage *wlmessage, unsigned int not_resizable);

unsigned int
wlmessage_get_noresize (struct wlmessage *wlmessage);

void
wlmessage_set_icon (struct wlmessage *wlmessage, char *icon_path);

void
wlmessage_set_message (struct wlmessage *wlmessage, char *message);

char *
wlmessage_get_message (struct wlmessage *wlmessage);

void
wlmessage_set_message_file (struct wlmessage *wlmessage, char *file_path);

void
wlmessage_add_button (struct wlmessage *wlmessage, unsigned int index, char *caption);

void
wlmessage_delete_button (struct wlmessage *wlmessage, unsigned int index);

void
wlmessage_set_default_button (struct wlmessage *wlmessage, unsigned int index);

void
wlmessage_set_textfield (struct wlmessage *wlmessage, char *default_text);

char *
wlmessage_get_textfield (struct wlmessage *wlmessage);

void
wlmessage_set_progress_callback (struct wlmessage *wlmessage, void (*callback) (struct wlmessage *wlmessage, void *data), void *data);

void
wlmessage_set_progress (struct wlmessage *wlmessage, float progress);

float
wlmessage_get_progress (struct wlmessage *wlmessage);

void
wlmessage_set_timeout (struct wlmessage *wlmessage, unsigned int timeout);

unsigned int
wlmessage_get_timeout (struct wlmessage *wlmessage);

int
wlmessage_show (struct wlmessage *wlmessage, char **input_text);

struct wlmessage *
wlmessage_create ();

void
wlmessage_destroy (struct wlmessage *wlmessage);

#endif
