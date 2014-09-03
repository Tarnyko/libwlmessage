/* Copyright © 2014 Manuel Bachmann */

#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <X11/Intrinsic.h>
#include <cairo/cairo-xlib.h>

#include "libwlmessage.h"
#define MAX_LINES 6


struct message_window {
	Widget window;

	char *message;
	char *title;
	int frame_type;	/* for titlebuttons */
	int resizable;
	cairo_surface_t *icon;
	Widget entry;
	int buttons_nb;
	//struct wl_list button_list;

	struct wlmessage *wlmessage;
};

struct wlmessage {
	Display *display;
	XtAppContext app;
	struct message_window *message_window;
	int return_value;
	int timeout;
};

 /* ---------------------------------------- */

 /* -- HELPER FUNCTIONS -- */

void *
xzalloc(size_t size)
{
	return calloc(1, size);
}

int
get_number_of_lines (char *text)
{
	int lines_num = 0;

	gchar **lines = g_strsplit (text, "\n", -1);

	if (lines) {
		while ((lines[lines_num] != NULL) && (lines_num < MAX_LINES))
			lines_num++;
		g_strfreev (lines);
	}

	return lines_num;
}

int
get_max_length_of_lines (char *text)
{
	int lines_num = 0;
	int length = 0;

	gchar **lines = g_strsplit (text, "\n", -1);

	if (lines) {
		while ((lines[lines_num] != NULL) && (lines_num < MAX_LINES)) {
			if (strlen (lines[lines_num]) > length)
				length = strlen (lines[lines_num]);
			lines_num++;
		}
		g_strfreev (lines);
	}

	return length;
}

char **
get_lines (char *text)
{
	gchar **lines = g_strsplit (text, "\n", -1);

	return lines;
}

 /* ---------------------------------------- */

 /* -- HANDLERS -- */

#if 0
void
button_send_activate (struct button *button)
{
	button->message_window->wlmessage->return_value = button->value;

	exit (0);
}

static void
key_handler (struct window *window, struct input *input, uint32_t time,
		 uint32_t key, uint32_t sym, enum wl_keyboard_key_state state,
		 void *data)
{
	struct message_window *message_window = data;
	struct entry *entry = message_window->entry;
	char *new_text;
	char text[16];

	if (sym == XKB_KEY_Return || sym == XKB_KEY_KP_Enter)
		exit (0);
}

static void
redraw_handler (struct widget *widget, void *data)
{
	struct message_window *message_window = data;
	int lines_nb;
	char **lines;


	if (message_window->icon) {
			cairo_set_source_surface (cr, message_window->icon,
			                              allocation.x + (allocation.width - 64.0)/2,
			                              allocation.y + 10);
			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			cairo_paint (cr);
			cairo_set_source_surface (cr, surface, 0.0, 0.0);
	}

	g_strfreev (lines);
}
#endif
 /* ---------------------------------------- */

void
wlmessage_set_title (struct wlmessage *wlmessage, char *title)
{
	if ((!wlmessage) || (!title))
		return;

	struct message_window *message_window = wlmessage->message_window;

	if (message_window->title)
		free (message_window->title);

	message_window->title = strdup (title);
}

char *
wlmessage_get_title (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return NULL;

	struct message_window *message_window = wlmessage->message_window;

	return message_window->title;
}

void
wlmessage_set_titlebuttons (struct wlmessage *wlmessage, enum wlmessage_titlebutton titlebuttons)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;

	/*message_window->frame_type = FRAME_NONE;
	
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_MINIMIZE)
		message_window->frame_type = message_window->frame_type | FRAME_MINIMIZE;
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_MAXIMIZE)
		message_window->frame_type = message_window->frame_type | FRAME_MAXIMIZE;
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_CLOSE)
		message_window->frame_type = message_window->frame_type | FRAME_CLOSE;*/
}

enum wlmessage_titlebutton
wlmessage_get_titlebuttons (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return 0;

	struct message_window *message_window = wlmessage->message_window;
	enum wlmessage_titlebutton titlebuttons;

	titlebuttons = WLMESSAGE_TITLEBUTTON_NONE;

	/*if (message_window->frame_type && FRAME_MINIMIZE)
		titlebuttons = titlebuttons | WLMESSAGE_TITLEBUTTON_MINIMIZE;
	if (message_window->frame_type && FRAME_MAXIMIZE)
		titlebuttons = titlebuttons | WLMESSAGE_TITLEBUTTON_MAXIMIZE;
	if (message_window->frame_type && FRAME_CLOSE)
		titlebuttons = titlebuttons | WLMESSAGE_TITLEBUTTON_CLOSE;*/

	return titlebuttons;
}

void
wlmessage_set_noresize (struct wlmessage *wlmessage, unsigned int not_resizable)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;

	message_window->resizable = !not_resizable;
}

unsigned int
wlmessage_get_noresize (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return 0;

	struct message_window *message_window = wlmessage->message_window;

	return !message_window->resizable;
}

void
wlmessage_set_icon (struct wlmessage *wlmessage, char *icon_path)
{
	if ((!wlmessage) || (!icon_path))
		return;

	struct message_window *message_window = wlmessage->message_window;
	cairo_surface_t *icon;
	cairo_status_t status;

	icon = cairo_image_surface_create_from_png (icon_path);
	status = cairo_surface_status (icon);

	if (status == CAIRO_STATUS_SUCCESS) {
			message_window->icon = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
			cairo_t *icon_cr = cairo_create (message_window->icon);
			 /* rescale to 64x64 */
			int width = cairo_image_surface_get_width (icon);
			int height = cairo_image_surface_get_height (icon);
			if (width != height != 64) {
				double ratio = ((64.0/width) < (64.0/height) ? (64.0/width) : (64.0/height));
				cairo_scale (icon_cr, ratio, ratio);
			}
			cairo_set_source_surface (icon_cr, icon, 0.0, 0.0);
			cairo_paint (icon_cr);
			cairo_destroy (icon_cr);
			cairo_surface_destroy (icon);
	}

}

void
wlmessage_set_message (struct wlmessage *wlmessage, char *message)
{
	if ((!wlmessage) || (!message))
		return;

	struct message_window *message_window = wlmessage->message_window;

	if (message_window->message)
		free (message_window->message);

	message_window->message = strdup (message);
}

char *
wlmessage_get_message (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return NULL;

	struct message_window *message_window = wlmessage->message_window;

	return message_window->message;
}

void
wlmessage_set_message_file (struct wlmessage *wlmessage, char *file_path)
{
	if ((!wlmessage) || (!file_path))
		return;

	struct message_window *message_window = wlmessage->message_window;
	FILE *file = NULL;
	char *text = NULL;
	int i, c;

	file = fopen (file_path, "r");
	if (!file) {
		return;
	} else {
		i = 0;
		text = malloc (sizeof(char));
		while (c != EOF) {
			c = fgetc (file);
			if (c != EOF) {
				realloc (text, (i+1)*sizeof(char));
				text[i] = c;
				i++;
			}
		}
		realloc (text, (i+1)*sizeof(char));
		text[i] = '\0';
		fclose (file);
	}

	if (message_window->message)
		free (message_window->message);

	message_window->message = text;
}

void
wlmessage_add_button (struct wlmessage *wlmessage, unsigned int index, char *caption)
{
	if ((!wlmessage) || (!caption))
		return;

	struct message_window *message_window = wlmessage->message_window;
	/*struct button *button;

	button = xzalloc (sizeof *button);
	button->caption = strdup (caption);
	button->value = index;
	button->message_window = message_window;

	message_window->buttons_nb++;
	wl_list_insert (message_window->button_list.prev, &button->link);*/
}

void
wlmessage_delete_button (struct wlmessage *wlmessage, unsigned int index)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;

	/*struct button *button, *tmp;
	wl_list_for_each_safe (button, tmp, &message_window->button_list, link) {
		if (button->value == index) {
			wl_list_remove (&button->link);
			widget_destroy (button->widget);
			free (button->caption);
			free (button);
			message_window->buttons_nb--;
		}
	}*/
}

void
wlmessage_set_default_button (struct wlmessage *wlmessage, unsigned int index)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;
	/*struct button *button;

	wl_list_for_each (button, &message_window->button_list, link) {
		if (button->value == index)
				wlmessage->return_value = button->value;
	}*/
}

void
wlmessage_set_textfield (struct wlmessage *wlmessage, char *default_text)
{
	if ((!wlmessage) || (!default_text))
		return;

	struct message_window *message_window = wlmessage->message_window;
	/*struct entry *entry;

	entry = xzalloc (sizeof *entry);
	entry->text = strdup (default_text);
	entry->cursor_pos = strlen (entry->text);
	entry->cursor_anchor = entry->cursor_pos;
	entry->last_vkb_len = 0;
	entry->active = 0;
	entry->message_window = message_window;

	message_window->entry = entry;*/
}

char *
wlmessage_get_textfield (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return NULL;

	struct message_window *message_window = wlmessage->message_window;

	/*if (!message_window->entry)
		return NULL;
	else
		return message_window->entry->text;*/
	return NULL;
}

void
wlmessage_set_timeout (struct wlmessage *wlmessage, unsigned int timeout)
{
	if (!wlmessage)
		return;

	wlmessage->timeout = timeout;
}

unsigned int
wlmessage_get_timeout (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return 0;

	return wlmessage->timeout;
}

int
wlmessage_show (struct wlmessage *wlmessage, char **input_text)
{
	if (!wlmessage)
		return 0;

	struct message_window *message_window = wlmessage->message_window;
	Widget form, label, entry, button;
	XWindowChanges wc;
	int extended_width = 0;
	int lines_nb = 0;

	message_window->window = XtVaAppInitialize (&wlmessage->app, "wlmessage", NULL, 0, NULL, NULL, NULL, NULL);

	wlmessage->display = XtDisplay (message_window->window);
	if (!wlmessage->display) {
		fprintf (stderr, "Failed to get X11 display object !\n");
		return -1;
	}

	/*if (wlmessage->timeout)
		display_set_timeout (wlmessage->display, wlmessage->timeout);*/

	/*window_set_title (message_window->window, message_window->title);*/

	 /* add main form */


	 /* add entry */
	/*
	if (message_window->entry) {
		entry = message_window->entry;
		entry->widget = widget_add_widget (message_window->widget, entry);
		widget_set_redraw_handler (entry->widget, entry_redraw_handler);
		widget_set_motion_handler (entry->widget, entry_motion_handler);
		widget_set_button_handler (entry->widget, entry_click_handler);
		widget_set_touch_down_handler (entry->widget, entry_touch_handler);
	}*/

	 /* add buttons */
	/*
	wl_list_for_each (button, &message_window->button_list, link) {
		button->widget = widget_add_widget (message_window->widget, button);
		widget_set_redraw_handler (button->widget, button_redraw_handler);
		widget_set_enter_handler (button->widget, button_enter_handler);
		widget_set_leave_handler (button->widget, button_leave_handler);
		widget_set_button_handler (button->widget, button_click_handler);
		widget_set_touch_down_handler (button->widget, button_touch_down_handler);
		widget_set_touch_up_handler (button->widget, button_touch_up_handler);
	}*/

	extended_width = (get_max_length_of_lines (message_window->message)) - 35;
	 if (extended_width < 0) extended_width = 0;
	lines_nb = get_number_of_lines (message_window->message);

	XtRealizeWidget (message_window->window);
	wc.width = 480 + extended_width*10;
	wc.height = 280 + lines_nb*16; /*+ (!message_window->entry ? 0 : 1)*32
	                               + (!message_window->buttons_nb ? 0 : 1)*32);*/
	XConfigureWindow (wlmessage->display, XtWindow(message_window->window), CWWidth | CWHeight, &wc);

	XtAppMainLoop (wlmessage->app);

	return wlmessage->return_value;
}

struct wlmessage *
wlmessage_create ()
{
	struct wlmessage *wlmessage;

	wlmessage = xzalloc (sizeof *wlmessage);
	wlmessage->return_value = 0;
	wlmessage->timeout = 0;

	wlmessage->message_window = xzalloc (sizeof *wlmessage->message_window);
	wlmessage->message_window->title = strdup ("wlmessage");
	//wlmessage->message_window->frame_type = FRAME_ALL;
	wlmessage->message_window->resizable = 1;
	wlmessage->message_window->icon = NULL;
	wlmessage->message_window->message = NULL;
	//wlmessage->message_window->entry = NULL;
	wlmessage->message_window->buttons_nb = 0;
	//wl_list_init (&wlmessage->message_window->button_list);
	wlmessage->message_window->wlmessage = wlmessage;

	return wlmessage;
}

void
wlmessage_destroy (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;

	if (message_window->icon)
		cairo_surface_destroy (message_window->icon);
	//if (message_window->window)
	//	window_destroy (message_window->window);
	if (message_window->title)
		free (message_window->title);
	if (message_window->message)
		free (message_window->message);
	free (message_window);

	free (wlmessage);
}

 /* ---------------------------------------- */

