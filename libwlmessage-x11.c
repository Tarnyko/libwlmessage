/* Copyright © 2014 Manuel Bachmann */

#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <cairo/cairo-xlib.h>

#include "libwlmessage.h"
#define MAX_LINES 6


struct message_window {
	Widget window;

	char *message;
	char *title;
	char *textfield;
	int frame_type;	/* for titlebuttons */
	int resizable;
	cairo_surface_t *icon;
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

struct widget_map {
	cairo_surface_t *icon;
	Widget label;
	Widget entry;
	Widget button;
	int lines_nb;
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

void
bt_on_validate (Widget widget, XtPointer data, XtPointer callback_data)
{
	//button->message_window->wlmessage->return_value = button->value;

	//exit (0);
}

static void
bt_on_pointer (Widget widget, XtPointer data, XEvent *event, Boolean *d)
{
	Widget button = widget;
	int x, y;
	
	switch (event->type) {
		case ButtonPress:
			XtVaGetValues (button, XtNx, &x, XtNy, &y, NULL);
			XtMoveWidget (button, x+1, y+1);
			XtVaSetValues (button, XtVaTypedArg, XtNbackground, XtRString, "black", strlen("black")+1, NULL);
			XtVaSetValues (button, XtVaTypedArg, XtNforeground, XtRString, "white", strlen("white")+1, NULL);
			break;
		case ButtonRelease:
			XtVaGetValues (button, XtNx, &x, XtNy, &y, NULL);
			XtMoveWidget (button, x-1, y-1);
			XtVaSetValues (button, XtVaTypedArg, XtNbackground, XtRString, "white", strlen("white")+1, NULL);
			XtVaSetValues (button, XtVaTypedArg, XtNforeground, XtRString, "black", strlen("black")+1, NULL);
			break;
		case EnterNotify:
			XtVaSetValues (button, XtVaTypedArg, XtNbackground, XtRString, "white", strlen("white")+1, NULL);
			break;
		case LeaveNotify:
			XtVaSetValues (button, XtVaTypedArg, XtNbackground, XtRString, "light gray", strlen("light gray")+1, NULL);
			break;
		default:
			break;
	}
}

static void
resize_handler (Widget widget, XtPointer data, XEvent *event, Boolean *d)
{
	Widget form = widget;
	struct widget_map *map = (struct widget_map *)data;
	short width, height;

	if (event->type == ConfigureNotify) {
		XtConfigureWidget (map->label, (event->xconfigure.width - (event->xconfigure.width-100)) / 2,
		                               (!map->icon ? 10 : 80),
		                               event->xconfigure.width - 100,
		                               map->lines_nb * 20,
		                               1);
		if (map->entry)
			XtConfigureWidget (map->entry, (event->xconfigure.width - (event->xconfigure.width-150)) /2,
			                               event->xconfigure.height - 80,
			                               event->xconfigure.width - 150,
			                               20,
			                               1);
		XtConfigureWidget (map->button, (event->xconfigure.width - 80) /2,
		                                event->xconfigure.height - 50,
		                                80, 30,
		                                1);

		event->type = Expose;
	}

	if (event->type = Expose && map->icon) {
		XtVaGetValues (form, XtNwidth, &width,
		                     XtNheight, &height,
		                     NULL);
		cairo_surface_t *cs = cairo_xlib_surface_create (XtDisplay(form), XtWindow(form), DefaultVisual(XtDisplay(form), 0), width, height);
		cairo_t *cr = cairo_create (cs);
		cairo_set_source_surface (cr, map->icon, (width - 64)/2, 10);
		cairo_paint (cr);
		cairo_destroy (cr);
		cairo_surface_destroy (cs);
	}
}

#if 0
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

	if (message_window->textfield)
		free (message_window->textfield);

	message_window->textfield = strdup (default_text);
}

char *
wlmessage_get_textfield (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return NULL;

	struct message_window *message_window = wlmessage->message_window;
	
	return message_window->textfield;
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
	int argc_v = 3;
	gchar **argv_v = g_strsplit ("libwlmessage -geometry 320x200", " ", -1);

	message_window->window = XtVaAppInitialize (&wlmessage->app, "wlmessage", NULL, 0, &argc_v, argv_v, NULL, NULL);

	wlmessage->display = XtDisplay (message_window->window);
	if (!wlmessage->display) {
		fprintf (stderr, "Failed to get X11 display object !\n");
		return -1;
	}

	/*if (wlmessage->timeout)
		display_set_timeout (wlmessage->display, wlmessage->timeout);*/

	 /* add main form */
	form = XtCreateManagedWidget ("form", formWidgetClass, message_window->window, NULL, 0);
	XtVaSetValues (form, XtVaTypedArg, XtNbackground, XtRString, "light slate grey", strlen("light slate grey")+1, NULL);

	 /* add message */
	label = XtCreateManagedWidget ("message", labelWidgetClass, form, NULL, 0);
	XtVaSetValues (label, XtVaTypedArg, XtNbackground, XtRString, "light slate grey", strlen("light slate grey")+1, NULL);
	XtVaSetValues (label, XtVaTypedArg, XtNborderColor, XtRString, "light slate grey", strlen("light slate grey")+1, NULL);
	XtVaSetValues (label, XtNlabel, (String) message_window->message,
	                      XtNborderWidth, 0,
	                      NULL);

	 /* add entry */
	if (message_window->textfield) {
		entry = XtCreateManagedWidget ("entry", asciiTextWidgetClass, form, NULL, 0);
		XtVaSetValues (entry, XtNstring, (String) message_window->textfield,
		                      XtNeditType, XawtextEdit,
		                      XtNwrap, XawtextWrapWord,
		                      XtNautoFill, True,
		                      XtNleftColumn, 10,
		                      XtNrightColumn, 20,
		                      XtNjustify, XtJustifyCenter,
		                      XtNfromVert, label,
		                      NULL);
	}

	 /* add buttons */
	button = XtCreateManagedWidget ("OK", commandWidgetClass, form, NULL, 0);
	XtVaSetValues (button, XtVaTypedArg, XtNbackground, XtRString, "light gray", strlen("light gray")+1, NULL);
	XtVaSetValues (button, XtNhighlightThickness, 0, NULL);
	if (message_window->textfield)
		XtVaSetValues (button, XtNfromVert, entry, NULL);
	else
		XtVaSetValues (button, XtNfromVert, label, NULL);
	XtAddEventHandler (button, ButtonPressMask | ButtonReleaseMask |
	                           EnterWindowMask | LeaveWindowMask,
	                           True, bt_on_pointer, NULL);
	XtAddCallback (button, XtNcallback, bt_on_validate, NULL);

	/*
	wl_list_for_each (button, &message_window->button_list, link) {
		button->widget = widget_add_widget (message_window->widget, button);
	}*/

	 /* global resize handler */
	extended_width = (get_max_length_of_lines (message_window->message)) - 35;
	 if (extended_width < 0) extended_width = 0;
	lines_nb = get_number_of_lines (message_window->message);

	struct widget_map *map = xzalloc (sizeof *map);
	map->icon = message_window->icon;
	map->label = label;
	map->entry = entry;
	map->button = button;
	map->lines_nb = lines_nb;
	XtAddEventHandler (form, StructureNotifyMask | ExposureMask,
	                         True, resize_handler, map);

	 /* general actions (title, size,...) */
	XtRealizeWidget (message_window->window);
	XStoreName (wlmessage->display, XtWindow(message_window->window), message_window->title);
	wc.width = 420 + extended_width*10;
	wc.height = 240 + lines_nb*20; /*+ (!message_window->entry ? 0 : 1)*32
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
	wlmessage->message_window->textfield = NULL;
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
	if (message_window->textfield)
		free (message_window->textfield);
	free (message_window);

	free (wlmessage);
}

 /* ---------------------------------------- */

