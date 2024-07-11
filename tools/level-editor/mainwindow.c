#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Label.h>
#include <X11/Xlib.h>
#include <X11/Shell.h>

#define GRID_SIZE 32

typedef struct {
    int x;
    int y;
    bool is_horizontal;
    int thickness;
} Segment;

Widget draw_area;
Segment segments[GRID_SIZE * GRID_SIZE * 2]; // Enough space for all possible segments
int segment_count = 0;

void PopupDialog(Widget widget, XtPointer client_data, XtPointer call_data);
void SaveAsDialog(Widget widget, XtPointer client_data, XtPointer call_data);
void SaveFile(Widget widget, XtPointer client_data, XtPointer call_data);
void ClosePopup(Widget widget, XtPointer client_data, XtPointer call_data);
void ExposeCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch);
void ClickCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch);

int main(int argc, char **argv) {
    XtAppContext app_context;
    Widget topLevel, form, button, save_button, viewport;
    Display *display;
    int screen;
    Window window;

    topLevel = XtVaAppInitialize(&app_context, "XawExample", NULL, 0, &argc, argv, NULL, NULL);
    
    form = XtVaCreateManagedWidget("form", formWidgetClass, topLevel, NULL);
    
    button = XtVaCreateManagedWidget("button", commandWidgetClass, form,
                                     XtNlabel, "About",
                                     XtNfromVert, NULL,
                                     XtNwidth, 100,
                                     XtNheight, 30,
                                     NULL);
    
    save_button = XtVaCreateManagedWidget("save_button", commandWidgetClass, form,
                                          XtNlabel, "Save As",
                                          XtNfromVert, button,
                                          XtNwidth, 100,
                                          XtNheight, 30,
                                          NULL);
    
    viewport = XtVaCreateManagedWidget("viewport", viewportWidgetClass, form,
                                       XtNfromVert, save_button,
                                       XtNwidth, 200,
                                       XtNheight, 200,
                                       NULL);
    
    draw_area = XtVaCreateManagedWidget("", labelWidgetClass, viewport,
                                        XtNwidth, 200,
                                        XtNheight, 200,
                                        NULL);
    
    XtAddCallback(button, XtNcallback, PopupDialog, (XtPointer)topLevel);
    XtAddCallback(save_button, XtNcallback, SaveAsDialog, (XtPointer)topLevel);
    XtAddEventHandler(draw_area, ExposureMask, False, ExposeCallback, NULL);
    XtAddEventHandler(draw_area, ButtonPressMask, False, ClickCallback, NULL);
    
    XtRealizeWidget(topLevel);
    
    display = XtDisplay(topLevel);
    window = XtWindow(draw_area);
    screen = DefaultScreen(display);
    
    XMapWindow(display, window);
    
    XtAppMainLoop(app_context);
    
    return 0;
}

void PopupDialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget popup, dialog, popup_button;
    XtAppContext app_context = XtWidgetToApplicationContext(widget);

    popup = XtVaCreatePopupShell("popup", transientShellWidgetClass, (Widget)client_data,
                                 XtNwidth, 200,
                                 XtNheight, 100,
                                 XtNlabel, "Popup",
                                 NULL);

    dialog = XtVaCreateManagedWidget("dialog", dialogWidgetClass, popup,
                                     XtNlabel, "Level editor by Daniel \"MontyOnTheRun\" Monteiro",
                                     NULL);
    
    popup_button = XtVaCreateManagedWidget("popup_button", commandWidgetClass, dialog,
                                           XtNlabel, "OK",
                                           NULL);

    XtAddCallback(popup_button, XtNcallback, ClosePopup, (XtPointer)popup);

    XtPopup(popup, XtGrabNone);
}

void SaveAsDialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget popup, dialog, save_button;
    XtAppContext app_context = XtWidgetToApplicationContext(widget);

    popup = XtVaCreatePopupShell("save_popup", transientShellWidgetClass, (Widget)client_data,
                                 XtNwidth, 300,
                                 XtNheight, 150,
                                 XtNlabel, "Save As",
                                 NULL);

    dialog = XtVaCreateManagedWidget("save_dialog", dialogWidgetClass, popup,
                                     XtNlabel, "Enter filename:",
                                     XtNvalue, "",
                                     NULL);
    
    save_button = XtVaCreateManagedWidget("save_button", commandWidgetClass, dialog,
                                          XtNlabel, "Save",
                                          NULL);

    XtAddCallback(save_button, XtNcallback, SaveFile, (XtPointer)dialog);
    XtAddCallback(save_button, XtNcallback, ClosePopup, (XtPointer)popup);

    XtPopup(popup, XtGrabNone);
}

void SaveFile(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget dialog = (Widget)client_data;
    char *filename = XawDialogGetValueString(dialog);

    if (filename != NULL && *filename != '\0') {
        FILE *file = fopen(filename, "w");
        if (file != NULL) {
            fclose(file);
        }
    }
}

void ClosePopup(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget popup = (Widget)client_data;
    XtDestroyWidget(popup);
}

void ExposeCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch) {
    if (event->type == Expose) {
        Display *display = XtDisplay(widget);
        Window window = XtWindow(widget);
        XWindowAttributes attr;
        XGetWindowAttributes(display, window, &attr);

        GC gc;
        XGCValues values;

        gc = XCreateGC(display, window, 0, &values);
        XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));

        for (int y = 1; y <= GRID_SIZE; ++y) {
            XDrawLine(display, window, gc, 0, y * (attr.height / GRID_SIZE), attr.width, y * (attr.height / GRID_SIZE));
        }

        for (int x = 1; x <= GRID_SIZE; ++x) {
            XDrawLine(display, window, gc, x * (attr.width / GRID_SIZE), 0, x * (attr.width / GRID_SIZE), attr.height);
        }

        for (int i = 0; i < segment_count; ++i) {
            XSetLineAttributes(display, gc, segments[i].thickness, LineSolid, CapButt, JoinMiter);

            if (segments[i].is_horizontal) {
                XDrawLine(display, window, gc,
                          segments[i].x * (attr.width / GRID_SIZE), segments[i].y * (attr.height / GRID_SIZE),
                          (segments[i].x + 1) * (attr.width / GRID_SIZE), segments[i].y * (attr.height / GRID_SIZE));
            } else {
                XDrawLine(display, window, gc,
                          segments[i].x * (attr.width / GRID_SIZE), segments[i].y * (attr.height / GRID_SIZE),
                          segments[i].x * (attr.width / GRID_SIZE), (segments[i].y + 1) * (attr.height / GRID_SIZE));
            }
        }

        XFreeGC(display, gc);
    }
}

void ClickCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch) {
    if (event->type == ButtonPress) {
        Display *display = XtDisplay(draw_area);
        XButtonEvent *button_event = (XButtonEvent *)event;
        Window window = XtWindow(draw_area);
        XWindowAttributes attr;
        XGetWindowAttributes(display, window, &attr);

        int grid_width = attr.width / GRID_SIZE;
        int grid_height = attr.height / GRID_SIZE;

        int x = button_event->x / grid_width;
        int y = button_event->y / grid_height;

        int x_offset = button_event->x % grid_width;
        int y_offset = button_event->y % grid_height;

        if (x_offset <= grid_width / 2) {
            // Vertical segment clicked
            segments[segment_count].x = x;
            segments[segment_count].y = y;
            segments[segment_count].is_horizontal = false;
        } else if (y_offset <= grid_height / 2) {
            // Horizontal segment clicked
            segments[segment_count].x = x;
            segments[segment_count].y = y;
            segments[segment_count].is_horizontal = true;
        } else {
            return; // Click is too far from any segment center
        }

        segments[segment_count].thickness = 4; // Example thickness, can be changed
        segment_count++;

        XClearArea(display, window, 0, 0, 0, 0, True);
    }
}
