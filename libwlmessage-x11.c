/* Copyright © 2014 Manuel Bachmann */

#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <cairo/cairo-xlib.h>

#include "libwlmessage.h"
#define MAX_LINES 6
#define MAX_BUTTONS 3


struct message_window {
	Widget window;
	cairo_surface_t *icon;
	Widget label;
	Widget entry;
	Widget form_b;
	struct button *button_list[MAX_BUTTONS];

	char *message;
	char *title;
	char *textfield;
	char *progress_text;
	float progress;
	int frame_type;	/* for titlebuttons */
	int resizable;
	int buttons_nb;
	int lines_nb;
	struct timeval creat_time;

	struct wlmessage *wlmessage;
};

struct button {
	Widget button;

	char *caption;
	int value;
	struct message_window *message_window;
};

struct wlmessage {
	Display *display;
	XtAppContext app;
	struct message_window *message_window;
	void (*progress_callback) (struct wlmessage *wlmessage, void *data);
	void *progress_data;
	int return_value;
	int timeout;
};

 /* ---------------------------------------- */

 /* -- HELPER FUNCTIONS AND STRUCTURES -- */

#define _MOTIF_WM_HINTS_FUNCTIONS   (1L << 0)
#define _MOTIF_WM_HINTS_DECORATIONS (1L << 1)

#define _MOTIF_WM_FUNC_ALL      (1L << 0)
#define _MOTIF_WM_FUNC_RESIZE   (1L << 1)
#define _MOTIF_WM_FUNC_MOVE     (1L << 2)
#define _MOTIF_WM_FUNC_MINIMIZE (1L << 3)
#define _MOTIF_WM_FUNC_MAXIMIZE (1L << 4)
#define _MOTIF_WM_FUNC_CLOSE    (1L << 5)

#define _MOTIF_WM_DECOR_ALL      (1L << 0)
#define _MOTIF_WM_DECOR_MINIMIZE (1L << 5)
#define _MOTIF_WM_DECOR_MAXIMIZE (1L << 6)

typedef struct {
	unsigned int flags;
	unsigned int functions;
	unsigned int decorations;
	int inputMode;
	unsigned int status;
} WMHints;

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
	struct button *button = (struct button *) data;
	struct wlmessage *wlmessage = button->message_window->wlmessage;

	wlmessage->return_value = button->value;

	XtAppSetExitFlag (wlmessage->app);
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
	struct message_window *message_window = (struct message_window *) data;
	struct wlmessage *wlmessage = message_window->wlmessage;
	Widget form = widget;
	cairo_surface_t *cs;
	cairo_t *cr;
	cairo_text_extents_t extents;
	XEvent xev;
	struct timeval cur_time;
	short width, height;
	int i;

	if (wlmessage->timeout) {
		gettimeofday (&cur_time, NULL);
		if ((cur_time.tv_sec - message_window->creat_time.tv_sec) >= wlmessage->timeout)
			exit (0);
	}

	if (event->type == ConfigureNotify) {
		XtConfigureWidget (message_window->label, (event->xconfigure.width - (event->xconfigure.width-100)) / 2,
		                                           (!message_window->icon ? 10 : 80),
		                                           event->xconfigure.width - 100,
		                                           message_window->lines_nb * 20,
		                                           1);
		if (message_window->entry)
			XtConfigureWidget (message_window->entry, (event->xconfigure.width - (event->xconfigure.width-150)) /2,
			                                           event->xconfigure.height - 80,
			                                           event->xconfigure.width - 150,
			                                           20,
			                                           1);
		XtConfigureWidget (message_window->form_b, ((event->xconfigure.width-280) / 2) - 10,
		                                            event->xconfigure.height - 55,
		                                            290, 50,
		                                            1);
		for (i = 0; i < message_window->buttons_nb; i++) {
			XtConfigureWidget (message_window->button_list[i]->button,
			                   (280-(message_window->buttons_nb*80))/(message_window->buttons_nb+1) + i*80 + (i+1)*10,
			                   10,
			                   80, 30,
			                   1);
		}

		 /* force redraw */
		event->type = Expose;
	}

	if (event->type == Expose) {
		XtVaGetValues (form, XtNwidth, &width,
		                     XtNheight, &height,
		                     NULL);
		cs = cairo_xlib_surface_create (XtDisplay(form), XtWindow(form),
		                                DefaultVisual(XtDisplay(form), 0),
		                                width, height);

		if (message_window->icon) {
			cr = cairo_create (cs);
			cairo_set_source_surface (cr, message_window->icon, (width - 64)/2, 10);
			cairo_paint (cr);
			cairo_destroy (cr);
		}

		if (message_window->progress_text) {
			cr = cairo_create (cs);
			cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle (cr, (width - (width - 120))/2, height-140,
			                     width - 120, 16);
			cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
			cairo_fill (cr);
			cairo_rectangle (cr, (width - (width - 120))/2, height-140,
			                     (width - 120) * message_window->progress, 16);
			cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
			cairo_fill (cr);
			cairo_set_line_width (cr, 1);
			cairo_rectangle (cr, (width - (width - 120))/2, height-140,
			                     width - 120, 16);
			cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
			cairo_stroke_preserve (cr);

			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
			cairo_select_font_face (cr, "sans", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_NORMAL);
			cairo_set_font_size (cr, 10);
			cairo_text_extents (cr, message_window->progress_text, &extents);
			cairo_move_to (cr, (width - 120)/2 + (120 - extents.width)/2,
			                   height-140 + ((16 - extents.height)/2) + 6);
			cairo_show_text (cr, message_window->progress_text);
			cairo_destroy (cr);
		}

		cairo_surface_destroy (cs);
	}

	 /* we launch the callback, and update the window to redraw again */
	if (wlmessage->progress_callback) {
		wlmessage->progress_callback (wlmessage,
		                              wlmessage->progress_data);
		memset (&xev, 0, sizeof(xev));
		xev.type = Expose;
		xev.xexpose.window = XtWindow(form);
		XSendEvent (XtDisplay(form), XtWindow(form), False, ExposureMask, &xev);
		XFlush (XtDisplay(form));
	}
}

static void
key_handler (Widget widget, XtPointer data, XEvent *event, Boolean *d)
{
	struct message_window *message_window = (struct message_window *) data;
	Widget entry = widget;
	KeySym sym;
	XawTextPosition pos;
	XawTextBlock buffer;

	if (event->type == KeyPress) {
		sym = XLookupKeysym (&event->xkey, 0);
		if (sym == XK_Return || sym == XK_KP_Enter) {
			XtAppSetExitFlag (message_window->wlmessage->app);
		} else {
			 /* prevent a text longer than 20 characters */
			XtVaGetValues (entry, XtNinsertPosition, &pos, NULL);
			if (pos > 30) {
				buffer.firstPos = 0; buffer.length = 1;
				buffer.ptr = "\n"; buffer.format = FMT8BIT;
				XawTextReplace (entry, pos-1, pos, &buffer);
				XtVaSetValues (entry, XtNinsertPosition, 30, NULL);
			}
		}
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

	message_window->frame_type = _MOTIF_WM_FUNC_MOVE;
	if (message_window->resizable)
		message_window->frame_type = message_window->frame_type | _MOTIF_WM_FUNC_RESIZE;
	
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_MINIMIZE)
		message_window->frame_type = message_window->frame_type | _MOTIF_WM_FUNC_MINIMIZE;
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_MAXIMIZE)
		message_window->frame_type = message_window->frame_type | _MOTIF_WM_FUNC_MAXIMIZE;
	if (titlebuttons && WLMESSAGE_TITLEBUTTON_CLOSE)
		message_window->frame_type = message_window->frame_type | _MOTIF_WM_FUNC_CLOSE;
}

enum wlmessage_titlebutton
wlmessage_get_titlebuttons (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return 0;

	struct message_window *message_window = wlmessage->message_window;
	enum wlmessage_titlebutton titlebuttons;

	titlebuttons = WLMESSAGE_TITLEBUTTON_NONE;

	if (message_window->frame_type && _MOTIF_WM_FUNC_MINIMIZE)
		titlebuttons = titlebuttons | WLMESSAGE_TITLEBUTTON_MINIMIZE;
	if (message_window->frame_type && _MOTIF_WM_FUNC_MAXIMIZE)
		titlebuttons = titlebuttons | WLMESSAGE_TITLEBUTTON_MAXIMIZE;
	if (message_window->frame_type && _MOTIF_WM_FUNC_CLOSE)
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
			if ((width != 64) && (height != 64)) {
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
	struct button *button;

	if (message_window->buttons_nb == MAX_BUTTONS)
		return;

	button = xzalloc (sizeof *button);
	button->caption = strdup (caption);
	button->value = index;
	button->message_window = message_window;

	message_window->button_list[message_window->buttons_nb] = button;
	message_window->buttons_nb++;
}

void
wlmessage_delete_button (struct wlmessage *wlmessage, unsigned int index)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;
	struct button *button;
	int total, i;

	total = message_window->buttons_nb;
	for (i = 0; i < total; i++) {
		button = message_window->button_list[i];
		if (button->value == index) {
			free (button->caption);
			free (button);
			message_window->buttons_nb--;
		}
	}
}

void
wlmessage_set_default_button (struct wlmessage *wlmessage, unsigned int index)
{
	if (!wlmessage)
		return;

	struct message_window *message_window = wlmessage->message_window;
	struct button *button;
	int i;

	for (i = 0; i < message_window->buttons_nb; i++) {
		button = message_window->button_list[i];
		if (button->value == index)
				wlmessage->return_value = button->value;
	}
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
wlmessage_set_progress_callback (struct wlmessage *wlmessage, void (*callback) (struct wlmessage *wlmessage, void *data), void *data)
{
	if (!wlmessage)
		return;

	wlmessage->progress_callback = callback;
	wlmessage->progress_data = data;
}

void
wlmessage_set_progress (struct wlmessage *wlmessage, float progress)
{
	if (!wlmessage)
		return;
	if ((progress < 0.0) || (progress > 1.0))
		return;

	struct message_window *message_window = wlmessage->message_window;

	if (message_window->progress_text)
		free (message_window->progress_text);

	message_window->progress_text = g_strdup_printf ("%d %%", (int)(progress*100.0));
	message_window->progress = progress;
}

float
wlmessage_get_progress (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return -1.0;

	struct message_window *message_window = wlmessage->message_window;

	if (!message_window->progress_text)
		return -1.0;
	else
		return message_window->progress;
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
	Widget form, label, entry, form_b;
	XEvent ev;
	XWindowChanges wc;
	XSizeHints sh;
	WMHints wm_hints;
	Atom hintsatom;
	int extended_width = 0;
	int lines_nb = 0;
	int argc_v = 3;
	gchar **argv_v = g_strsplit ("libwlmessage -geometry 320x200", " ", -1);
	char *p_textfield;

	message_window->window = XtVaAppInitialize (&wlmessage->app, "wlmessage", NULL, 0, &argc_v, argv_v, NULL, NULL);
	g_strfreev (argv_v);

	wlmessage->display = XtDisplay (message_window->window);
	if (!wlmessage->display) {
		fprintf (stderr, "Failed to get X11 display object !\n");
		return -1;
	}

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

	 /* do not draw the entry and buttons if there is a callback */
	if (wlmessage->progress_callback) {
		entry = 0;
		message_window->buttons_nb = 0;
		goto form;
	}

	 /* add entry */
	if (message_window->textfield) {
		asprintf (&p_textfield, "%s\n", message_window->textfield);
		entry = XtCreateManagedWidget ("entry", asciiTextWidgetClass, form, NULL, 0);
		XtVaSetValues (entry, XtNstring, (String) p_textfield,
		                      XtNeditType, XawtextEdit,
		                      XtNwrap, XawtextWrapWord,
		                      XtNautoFill, True,
		                      XtNleftColumn, 4,
		                      XtNrightColumn, 24,
		                      XtNjustify, XtJustifyCenter,
		                      XtNfromVert, label,
		                      NULL);
		free (p_textfield);
	} else {
		entry = 0;
	}

form:
	 /* add buttons form */
	form_b = XtCreateManagedWidget ("form_b", formWidgetClass, form, NULL, 0);
	XtVaSetValues (form_b, XtVaTypedArg, XtNbackground, XtRString, "light slate grey", strlen("light slate grey")+1, NULL);
	XtVaSetValues (form_b, XtVaTypedArg, XtNborderColor, XtRString, "light slate grey", strlen("light slate grey")+1, NULL);
	XtVaSetValues (form_b, XtNborderWidth, 0, NULL);
	if (message_window->textfield)
		XtVaSetValues (form_b, XtNfromVert, entry, NULL);
	else
		XtVaSetValues (form_b, XtNfromVert, label, NULL);

	 /* add buttons */
	struct button *button, *prev_button = NULL;
	int i;
	for (i = 0; i < message_window->buttons_nb; i++) {
		button = message_window->button_list[i];
		button->button = XtCreateManagedWidget (button->caption, commandWidgetClass, form_b, NULL, 0);
		XtVaSetValues (button->button, XtVaTypedArg, XtNbackground, XtRString, "light gray", strlen("light gray")+1, NULL);
		XtVaSetValues (button->button, XtNhighlightThickness, 0, NULL);
		if (prev_button) {
			XtVaSetValues (button->button, XtNfromHoriz, prev_button->button, NULL);
		}
		XtAddEventHandler (button->button, ButtonPressMask | ButtonReleaseMask |
	        	                   EnterWindowMask | LeaveWindowMask,
	                	           True, bt_on_pointer, NULL);
		XtAddCallback (button->button, XtNcallback, bt_on_validate, button);
		prev_button = button;
	}

	 /* global resize handler */
	extended_width = (get_max_length_of_lines (message_window->message)) - 35;
	 if (extended_width < 0) extended_width = 0;
	lines_nb = get_number_of_lines (message_window->message);
	 /* */
	message_window->label = label;
	message_window->entry = entry;
	message_window->form_b = form_b;
	message_window->lines_nb = lines_nb;
	gettimeofday (&message_window->creat_time, NULL);
	XtAddEventHandler (form, StructureNotifyMask | ExposureMask,
	                         True, resize_handler, message_window);

	 /* global keyboard handler */
	if (entry)
		XtAddEventHandler (entry, KeyPressMask, True, key_handler, message_window);

	 /* general actions (title, size, decorations...) */
	XtRealizeWidget (message_window->window);

	XStoreName (wlmessage->display, XtWindow(message_window->window), message_window->title);

	wc.width = 420 + extended_width*10;
	wc.height = 240 + lines_nb*20; /*+ (!message_window->entry ? 0 : 1)*32
	                               + (!message_window->buttons_nb ? 0 : 1)*32);*/
	XConfigureWindow (wlmessage->display, XtWindow(message_window->window), CWWidth | CWHeight, &wc);

	sh.flags = PMinSize;
	sh.min_width = 384;
	sh.min_height = 200;
	XSetWMNormalHints (wlmessage->display, XtWindow(message_window->window), &sh);

	hintsatom = XInternAtom (wlmessage->display, "_MOTIF_WM_HINTS", False);
	if (hintsatom) {
		wm_hints.flags = _MOTIF_WM_HINTS_FUNCTIONS;
		wm_hints.functions = message_window->frame_type;
		/*wm_hints.decorations = False; *//* disables the border */
		XChangeProperty (wlmessage->display, XtWindow(message_window->window),
		                 hintsatom, hintsatom, 32, PropModeReplace,
		                 (unsigned char *) &wm_hints, 5);
		/* refresh here ! */
	}

	 /* main loop */
	XtAppMainLoop (wlmessage->app);

	 /* last iteration to destroy the window */
	XtUnrealizeWidget (message_window->window);
	XtAppNextEvent (wlmessage->app, &ev);
	XtDispatchEvent (&ev);

	if (entry) {
		XawTextBlock buffer;
		XawTextSourceRead (XawTextGetSource(entry), 0, &buffer, 30);
		*input_text = strdup (g_strstrip(buffer.ptr));
	}

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
	wlmessage->message_window->frame_type = _MOTIF_WM_FUNC_ALL;
	wlmessage->message_window->resizable = 1;
	wlmessage->message_window->icon = NULL;
	wlmessage->message_window->message = NULL;
	wlmessage->message_window->textfield = NULL;
	wlmessage->message_window->progress_text = NULL;
	wlmessage->message_window->progress = 0.0;
	wlmessage->message_window->buttons_nb = 0;
	wlmessage->message_window->wlmessage = wlmessage;

	return wlmessage;
}

void
wlmessage_destroy (struct wlmessage *wlmessage)
{
	if (!wlmessage)
		return;
	if (wlmessage->progress_callback) {
		wlmessage->progress_callback = NULL;
		XtAppSetExitFlag (wlmessage->app);
		return;
	}

	struct message_window *message_window = wlmessage->message_window;

	if (message_window->icon)
		cairo_surface_destroy (message_window->icon);
	if (message_window->title)
		free (message_window->title);
	if (message_window->message)
		free (message_window->message);
	if (message_window->textfield)
		free (message_window->textfield);
	if (message_window->progress_text)
		free (message_window->progress_text);
	free (message_window);

	free (wlmessage);
}

 /* ---------------------------------------- */

