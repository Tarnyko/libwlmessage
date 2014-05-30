/* Copyright © 2014 Manuel Bachmann */

#include <linux/input.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "window.h"
#include "libwlmessage.h"
#define MAX_LINES 6


struct message_window {
	struct window *window;
	struct widget *widget;
	cairo_surface_t *surface;

	char *message;
	char *title;
	int frame_type;	/* for titlebuttons */
	int resizable;

	cairo_surface_t *icon;
	struct entry *entry;
	int buttons_nb;
	struct wl_list button_list;

	struct wlmessage *wlmessage;
};

struct button {
	struct widget *widget;
	int focused, pressed;
	struct wl_list link;

	char *caption;
	int value;

	struct message_window *message_window;
};

struct entry {
	struct widget *widget;
	int active;

	struct wl_text_input *text_input;
	char *text;
	int cursor_pos;
	int cursor_anchor;
	int last_vkb_len;

	struct message_window *message_window;
};

struct wlmessage {
	struct message_window *message_window;
	int default_value;
	int timeout;
};

 /* ---------------------------------------- */

 /* -- HELPER FUNCTIONS -- */

int
get_number_of_lines (char *text)
{
	int lines_num = 0;

	gchar **lines = g_strsplit (text, "\n", -1);

	while ((lines[lines_num] != NULL) && (lines_num < MAX_LINES))
		lines_num++;

	g_strfreev (lines);

	return lines_num;
}

int
get_max_length_of_lines (char *text)
{
	int lines_num = 0;
	int length = 0;

	gchar **lines = g_strsplit (text, "\n", -1);

	while ((lines[lines_num] != NULL) && (lines_num < MAX_LINES)) {
		if (strlen (lines[lines_num]) > length)
			length = strlen (lines[lines_num]);
		lines_num++;
	}

	g_strfreev (lines);

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


static void
entry_click_handler(struct widget *widget,
		struct input *input, uint32_t time,
		uint32_t button,
		enum wl_pointer_button_state state, void *data)
{
	struct entry *entry = data;

	widget_schedule_redraw (widget);

	if (state == WL_POINTER_BUTTON_STATE_PRESSED && button == BTN_LEFT) {
#if 0
		if (!entry->text_input) {
			entry->text_input = wl_text_input_manager_create_text_input (text_input_manager);
			wl_text_input_add_listener (entry->text_input, &text_input_listener, entry);
		}

		struct wl_seat *seat = input_get_seat (input);
		struct wl_surface *surface = window_get_wl_surface (message_window->window);
		wl_text_input_show_input_panel (entry->text_input);
		wl_text_input_activate (entry->text_input, seat, surface);
#endif

		entry->active = 1;
	}
}

static void
entry_touch_handler(struct widget *widget, struct input *input,
		 uint32_t serial, uint32_t time, int32_t id,
		 float tx, float ty, void *data)
{
	struct entry *entry = data;

	widget_schedule_redraw (widget);
#if 0
	if (!entry->text_input) {
		entry->text_input = wl_text_input_manager_create_text_input (text_input_manager);
		wl_text_input_add_listener (entry->text_input, &text_input_listener, entry);
	}

	struct wl_seat *seat = input_get_seat (input);
	struct wl_surface *surface = window_get_wl_surface (message_window->window);
	wl_text_input_show_input_panel (entry->text_input);
	wl_text_input_activate (entry->text_input, seat, surface);
#endif
	entry->active = 1;
}

static int
entry_motion_handler(struct widget *widget,
		struct input *input, uint32_t time,
		float x, float y, void *data)
{
	return CURSOR_IBEAM;
}

static void
entry_redraw_handler (struct widget *widget, void *data)
{
	struct entry *entry = data;
	struct rectangle allocation;
	cairo_t *cr;
	cairo_text_extents_t extents;
	cairo_text_extents_t leftp_extents;
	char *leftp_text;
	int char_pos;

	widget_get_allocation (widget, &allocation);

	cr = widget_cairo_create (entry->message_window->widget);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr,
	                allocation.x,
	                allocation.y,
	                allocation.width,
	                allocation.height);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_fill (cr);
	cairo_set_line_width (cr, 1);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	if (entry->active)
		cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
	else
		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke_preserve(cr);

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
	cairo_select_font_face (cr, "sans",
	                        CAIRO_FONT_SLANT_NORMAL,
	                        CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 14);
	cairo_text_extents (cr, entry->text, &extents);
			char_pos = strlen(entry->text) -1;						/* for spaces at the end */
			while (char_pos >= 0 && entry->text[char_pos] == ' ') {
				extents.width += 5.0;
				char_pos--;
			}
	cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
	        	   allocation.y + (allocation.height - extents.height)/2 + 10);
	cairo_show_text (cr, entry->text);

	if (entry->active) {
		leftp_text = malloc (entry->cursor_pos + 1);
		strncpy (leftp_text, entry->text, entry->cursor_pos);
		leftp_text[entry->cursor_pos] = '\0';
		cairo_text_extents (cr, leftp_text, &leftp_extents);
			char_pos = strlen(leftp_text) -1;
			while (char_pos >= 0 && leftp_text[char_pos] == ' ') {
				leftp_extents.width += 5.0;
				char_pos--;
			}
		free (leftp_text);

		cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2 + leftp_extents.width,
		        	   allocation.y + (allocation.height - extents.height)/2 + 15);
		cairo_line_to(cr, allocation.x + (allocation.width - extents.width)/2 + leftp_extents.width,
		        	   allocation.y + (allocation.height - extents.height)/2 - 5);
		cairo_stroke(cr);
	}

	cairo_destroy (cr);
}

void
button_send_activate (struct button *button, int value)
{
	if (button->message_window->entry)
		fprintf (stdout, button->message_window->entry->text);
	wlmessage_destroy (button->message_window->wlmessage);
	exit (value);
}

static void
button_click_handler(struct widget *widget,
		struct input *input, uint32_t time,
		uint32_t butt,
		enum wl_pointer_button_state state, void *data)
{
	struct button *button = data;

	widget_schedule_redraw (widget);

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		button->pressed = 1;
	} else {
		button->pressed = 0;
		button_send_activate (button, button->value);
	}
}

static void
button_touch_down_handler(struct widget *widget, struct input *input,
		 uint32_t serial, uint32_t time, int32_t id,
		 float tx, float ty, void *data)
{
	struct button *button = data;

	button->focused = 1;
	widget_schedule_redraw (widget);
}

static void
button_touch_up_handler(struct widget *widget, struct input *input,
		 uint32_t serial, uint32_t time, int32_t id,
		 void *data)
{
	struct button *button = data;

	button->focused = 0;
	widget_schedule_redraw (widget);

	button_send_activate (button, button->value);
}

static int
button_enter_handler(struct widget *widget, struct input *input,
			     float x, float y, void *data)
{
	struct button *button = data;

	button->focused = 1;
	widget_schedule_redraw (widget);

	return CURSOR_LEFT_PTR;
}

static void
button_leave_handler(struct widget *widget,
			     struct input *input, void *data)
{
	struct button *button = data;

	button->focused = 0;
	widget_schedule_redraw (widget);
}

static void
button_redraw_handler (struct widget *widget, void *data)
{
	struct button *button = data;
	struct rectangle allocation;
	cairo_t *cr;
	cairo_text_extents_t extents;

	widget_get_allocation (widget, &allocation);
	if (button->pressed) {
		allocation.x++;
		allocation.y++;
	}

	cr = widget_cairo_create (button->message_window->widget);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	if (button->focused)
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	else
		cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
	cairo_fill (cr);
	cairo_set_line_width (cr, 1);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke_preserve(cr);
	cairo_select_font_face (cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 14);
	cairo_text_extents (cr, button->caption, &extents);
	cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
                           allocation.y + (allocation.height - extents.height)/2 + 10);
	cairo_show_text (cr, button->caption);
	cairo_destroy (cr);
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

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	if (sym == XKB_KEY_Return || sym == XKB_KEY_KP_Enter) {
#if 0
		if (entry)
			fprintf (stdout, entry->text);
#endif
		wlmessage_destroy (message_window->wlmessage);
		exit (message_window->wlmessage->default_value);
	}

	if (entry && entry->active) {
		switch (sym) {
			case XKB_KEY_BackSpace:
				if (entry->cursor_pos == 0)
					break;
				new_text = malloc (strlen(entry->text));
				strncpy (new_text, entry->text, entry->cursor_pos - 1);
				strcpy (new_text+entry->cursor_pos-1, entry->text+entry->cursor_pos);
				free (entry->text);
				entry->text = new_text;
				entry->cursor_pos--;
				break;
			case XKB_KEY_Delete:
				if (entry->cursor_pos == strlen (entry->text))
					break;
				new_text = malloc (strlen(entry->text));
				strncpy (new_text, entry->text, entry->cursor_pos);
				strcpy (new_text+entry->cursor_pos, entry->text+entry->cursor_pos+1);
				free (entry->text);
				entry->text = new_text;
				break;
			case XKB_KEY_Left:
				if (entry->cursor_pos != 0)
					entry->cursor_pos--;
				break;
			case XKB_KEY_Right:
				if (entry->cursor_pos != strlen (entry->text))
					entry->cursor_pos++;
				break;
			case XKB_KEY_Tab:
				break;
			default:
				if (strlen(entry->text) >= 18)
					break;
				if (xkb_keysym_to_utf8 (sym, text, sizeof(text)) <= 0)
					break;
				if (strlen(text) > 1)	/* dismiss non-ASCII characters for now */
					break;
				new_text = malloc (strlen(entry->text) + strlen(text) + 1);
				strncpy (new_text, entry->text, entry->cursor_pos);
				strcpy (new_text+entry->cursor_pos, text);
				strcpy (new_text+entry->cursor_pos+strlen(text), entry->text+entry->cursor_pos);
				free (entry->text);
				entry->text = new_text;
				entry->cursor_pos++;
		}
		widget_schedule_redraw(entry->widget);
	}
}

static void
resize_handler (struct widget *widget, int32_t width, int32_t height, void *data)
{
	struct message_window *message_window = data;
	struct entry *entry;
	struct button *button;
	struct rectangle allocation;
	int buttons_width, extended_width;
	int x;

	widget_get_allocation (widget, &allocation);

	x = allocation.x + (width - 240)/2;

	if (message_window->entry) {
		entry = message_window->entry;
		widget_set_allocation (entry->widget, x, allocation.y + height - 16*2 - 32*2,
		                                      240, 32);
	}

	buttons_width = 0;
	wl_list_for_each (button, &message_window->button_list, link) {
		extended_width = strlen(button->caption) - 5;
		if (extended_width < 0) extended_width = 0;
		buttons_width += 60 + extended_width*10;
	}

	x = allocation.x + (width - buttons_width)/2
	                 - (message_window->buttons_nb-1)*10;

	wl_list_for_each (button, &message_window->button_list, link) {
		extended_width = strlen(button->caption) - 5;
		if (extended_width < 0) extended_width = 0;
		widget_set_allocation (button->widget, x, allocation.y + height - 16 - 32,
		                                       60 + extended_width*10, 32); 
		x += 60 + extended_width*10 + 10;
	}
}

static void
redraw_handler (struct widget *widget, void *data)
{
	struct message_window *message_window = data;
	struct rectangle allocation;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_text_extents_t extents;
	int lines_nb;
	char **lines;

	widget_get_allocation (message_window->widget, &allocation);

	surface = window_get_surface (message_window->window);
	cr = cairo_create (surface);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 1.0);
	cairo_fill (cr);

	if (message_window->icon) {
			cairo_set_source_surface (cr, message_window->icon,
			                              allocation.x + (allocation.width - 64.0)/2,
			                              allocation.y + 10);
			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			cairo_paint (cr);
			cairo_set_source_surface (cr, surface, 0.0, 0.0);
	}

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
	cairo_select_font_face (cr, "sans",
	                        CAIRO_FONT_SLANT_NORMAL,
	                        CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 18);

	lines_nb = get_number_of_lines (message_window->message);
	lines = get_lines (message_window->message);

	int i;
	for (i = 0; i < lines_nb; i++) {
		cairo_text_extents (cr, lines[i], &extents);
		cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
	        	           allocation.y + (allocation.height - lines_nb * extents.height)/2
		                                + i*(extents.height+10)
		                                + (!message_window->icon ? 0 : 32)
                                                - (!message_window->entry ? 0 : 32)
                                                - (!message_window->buttons_nb ? 0 : 32));
		cairo_show_text (cr, lines[i]);
	}

	g_strfreev (lines);

	cairo_destroy (cr);
}

static void
global_handler(struct display *display, uint32_t name,
	       const char *interface, uint32_t version, void *data)
{
	if (!strcmp(interface, "wl_text_input_manager")) {
		text_input_manager = display_bind (display, name, &wl_text_input_manager_interface, 1);
	}
}

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

	message_window->frame_type = FRAME_NONE;
	
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_MINIMIZE)
		message_window->frame_type = message_window->frame_type | FRAME_MINIMIZE;
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_MAXIMIZE)
		message_window->frame_type = message_window->frame_type | FRAME_MAXIMIZE;
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_CLOSE)
		message_window->frame_type = message_window->frame_type | FRAME_CLOSE;
}

enum wlmessage_titlebutton
wlmessage_get_titlebuttons (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return 0;

	struct message_window *message_window = wlmessage->message_window;
	enum wlmessage_titlebutton titlebuttons;

	titlebuttons = WLMESSAGE_TITLEBUTTON_NONE;

	if (message_window->frame_type && FRAME_MINIMIZE)
		titlebuttons = titlebuttons | WLMESSAGE_TITLEBUTTON_MINIMIZE;
	if (message_window->frame_type && FRAME_MAXIMIZE)
		titlebuttons = titlebuttons | WLMESSAGE_TITLEBUTTON_MAXIMIZE;
	if (message_window->frame_type && FRAME_CLOSE)
		titlebuttons = titlebuttons | WLMESSAGE_TITLEBUTTON_CLOSE;

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

unsigned
wlmessage_get_noresize (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return;

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
wlmessage_add_button (struct wlmessage *wlmessage, int index, char *caption)
{
	if ((!wlmessage) || (!caption))
		return;

	struct message_window *message_window = wlmessage->message_window;
	struct button *button;

	button = xzalloc (sizeof *button);
	button->caption = strdup (caption);
	button->value = index;
	button->message_window = message_window;

	message_window->buttons_nb++;
	wl_list_insert (message_window->button_list.prev, &button->link);
}

void
wlmessage_delete_button (struct wlmessage *wlmessage, int index)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;

	struct button *button, *tmp;
	wl_list_for_each_safe (button, tmp, &message_window->button_list, link) {
		if (button->value == index) {
			wl_list_remove (&button->link);
			widget_destroy (button->widget);
			free (button->caption);
			free (button);
			message_window->buttons_nb--;
		}
	}
}

void
wlmessage_set_default_button (struct wlmessage *wlmessage, int index)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;
	struct button *button;

	wl_list_for_each (button, &message_window->button_list, link) {
		if (button->value == index)
				wlmessage->default_value = button->value;
	}
}

void
wlmessage_set_textfield (struct wlmessage *wlmessage, char *default_text)
{
	if ((!wlmessage) || (!default_text))
		return;

	struct message_window *message_window = wlmessage->message_window;
	struct entry *entry;

	entry = xzalloc (sizeof *entry);
	entry->text = strdup (default_text);
	entry->cursor_pos = strlen (entry->text);
	entry->cursor_anchor = entry->cursor_pos;
	entry->last_vkb_len = 0;
	entry->active = 0;
	entry->message_window = message_window;

	message_window->entry = entry;
}

char *
wlmessage_get_textfield (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return NULL;

	struct message_window *message_window = wlmessage->message_window;

	if (!message_window->entry)
		return NULL;
	else
		return message_window->entry->text;
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
	struct display *display = NULL;
	struct entry *entry;
	struct button *button;
	int extended_width = 0;
	int lines_nb = 0;

	display = display_create (NULL, NULL);
	if (!display) {
		fprintf (stderr, "Failed to connect to a Wayland compositor !\n");
		return -1;	// HUH ?
	}

	if (wlmessage->timeout)
		display_set_timeout (display, wlmessage->timeout);

	message_window->window = window_create (display);
	message_window->widget = window_frame_create (message_window->window,
	                                              message_window->frame_type,
	                                              message_window->resizable, message_window);
	window_set_title (message_window->window, message_window->title);

	 /* add entry */
	if (message_window->entry) {
		entry = message_window->entry;
		entry->widget = widget_add_widget (message_window->widget, entry);
		widget_set_redraw_handler (entry->widget, entry_redraw_handler);
		widget_set_motion_handler (entry->widget, entry_motion_handler);
		widget_set_button_handler (entry->widget, entry_click_handler);
		widget_set_touch_down_handler (entry->widget, entry_touch_handler);
	}

	 /* add buttons */
	wl_list_for_each (button, &message_window->button_list, link) {
		button->widget = widget_add_widget (message_window->widget, button);
		widget_set_redraw_handler (button->widget, button_redraw_handler);
		widget_set_enter_handler (button->widget, button_enter_handler);
		widget_set_leave_handler (button->widget, button_leave_handler);
		widget_set_button_handler (button->widget, button_click_handler);
		widget_set_touch_down_handler (button->widget, button_touch_down_handler);
		widget_set_touch_up_handler (button->widget, button_touch_up_handler);
	}

	extended_width = (get_max_length_of_lines (message_window->message)) - 35;
	 if (extended_width < 0) extended_width = 0;
	lines_nb = get_number_of_lines (message_window->message);

	window_set_user_data (message_window->window, message_window);
	window_set_key_handler (message_window->window, key_handler);
	widget_set_redraw_handler (message_window->widget, redraw_handler);
	widget_set_resize_handler (message_window->widget, resize_handler);

	window_schedule_resize (message_window->window,
	                        480 + extended_width*10,
	                        280 + lines_nb*16 + (!message_window->entry ? 0 : 1)*32
	                                          + (!message_window->buttons_nb ? 0 : 1)*32);

	display_set_global_handler (display, global_handler);
	display_run (display);

	return 0;
}

struct wlmessage *
wlmessage_create ()
{
	struct wlmessage *wlmessage;

	wlmessage = xzalloc (sizeof *wlmessage);
	wlmessage->default_value = 0;
	wlmessage->timeout = 0;

	wlmessage->message_window = xzalloc (sizeof *wlmessage->message_window);
	wlmessage->message_window->title = strdup ("wlmessage");
	wlmessage->message_window->frame_type = FRAME_ALL;
	wlmessage->message_window->resizable = 1;
	wlmessage->message_window->icon = NULL;
	wlmessage->message_window->message = NULL;
	wlmessage->message_window->entry = NULL;
	wlmessage->message_window->buttons_nb = 0;
	wl_list_init (&wlmessage->message_window->button_list);
	wlmessage->message_window->wlmessage = wlmessage;

	return wlmessage;
}

void
wlmessage_destroy (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;

	if (message_window->surface)
		cairo_surface_destroy (message_window->surface);
	if (message_window->icon)
		cairo_surface_destroy (message_window->icon);
	widget_destroy (message_window->widget);
	window_destroy (message_window->window);
	free (message_window->title);
	free (message_window->message);
	free (message_window);

	free (wlmessage);
}

 /* ---------------------------------------- */

