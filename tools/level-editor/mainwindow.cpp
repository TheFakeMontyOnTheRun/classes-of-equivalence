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

#include "level-editor.h"

#define GRID_SIZE 32

enum EDrawingMode {
    kPicker,
    kLine,
    kSquare
};

int pivotX, pivotY;

struct Map map;

Widget draw_area;
enum EDrawingMode drawingMode = kPicker;

void PopupDialog(Widget widget, XtPointer client_data, XtPointer call_data);

void SaveAsDialog(Widget widget, XtPointer client_data, XtPointer call_data);

void SaveFile(Widget widget, XtPointer client_data, XtPointer call_data);

void QuitButton(Widget widget, XtPointer client_data, XtPointer call_data);

void LoadDialog(Widget widget, XtPointer client_data, XtPointer call_data);

void LoadFile(Widget widget, XtPointer client_data, XtPointer call_data);

void DrawSquare(Widget widget, XtPointer client_data, XtPointer call_data);


void ClosePopup(Widget widget, XtPointer client_data, XtPointer call_data);

void ExposeCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch);

void ClickCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch);

int main(int argc, char **argv) {
    XtAppContext app_context;
    Widget topLevel, form, button, save_button, load_button, viewport, quit_button;
    Widget pick_square_button, draw_square_button;
    Display *display;
    int screen;
    Window window;
    map = initMap(32, 32);
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
    
    load_button = XtVaCreateManagedWidget("load_button", commandWidgetClass, form,
                                          XtNlabel, "Load",
                                          XtNfromVert, save_button,
                                          XtNwidth, 100,
                                          XtNheight, 30,
                                          NULL);

    quit_button = XtVaCreateManagedWidget("quit_button", commandWidgetClass, form,
                                          XtNlabel, "Quit",
                                          XtNfromVert, load_button,
                                          XtNwidth, 100,
                                          XtNheight, 30,
                                          NULL);

    draw_square_button = XtVaCreateManagedWidget("draw_square_button", commandWidgetClass, form,
                                                 XtNlabel, "Draw Square",
                                                 XtNfromHoriz, button,
                                                 XtNwidth, 100,
                                                 XtNheight, 30,
                                                 NULL);

    viewport = XtVaCreateManagedWidget("viewport", viewportWidgetClass, form,
                                       XtNfromVert, quit_button,
                                       XtNwidth, 200,
                                       XtNheight, 200,
                                       NULL);
    
    draw_area = XtVaCreateManagedWidget("", labelWidgetClass, viewport,
                                        XtNwidth, 200,
                                        XtNheight, 200,
                                        NULL);

    XtAddCallback(button, XtNcallback, PopupDialog, (XtPointer) topLevel);
    XtAddCallback(save_button, XtNcallback, SaveAsDialog, (XtPointer) topLevel);
    XtAddCallback(load_button, XtNcallback, LoadDialog, (XtPointer) topLevel);
    XtAddCallback(quit_button, XtNcallback, QuitButton, (XtPointer) topLevel);
    XtAddCallback(draw_square_button, XtNcallback, DrawSquare, (XtPointer) topLevel);
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

    popup = XtVaCreatePopupShell("popup", transientShellWidgetClass, (Widget) client_data,
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

    XtAddCallback(popup_button, XtNcallback, ClosePopup, (XtPointer) popup);

    XtPopup(popup, XtGrabNone);
}

void SaveAsDialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget popup, dialog, save_button;
    XtAppContext app_context = XtWidgetToApplicationContext(widget);

    popup = XtVaCreatePopupShell("save_popup", transientShellWidgetClass, (Widget) client_data,
                                 XtNwidth, 300,
                                 XtNheight, 150,
                                 XtNlabel, "Save As",
                                 NULL);

    dialog = XtVaCreateManagedWidget("save_dialog", dialogWidgetClass, popup,
                                     XtNlabel, "Enter filename:",
                                     XtNvalue, "mapfile.txt",
                                     NULL);

    save_button = XtVaCreateManagedWidget("save_button", commandWidgetClass, dialog,
                                          XtNlabel, "Save",
                                          NULL);

    XtAddCallback(save_button, XtNcallback, SaveFile, (XtPointer) dialog);
    XtAddCallback(save_button, XtNcallback, ClosePopup, (XtPointer) popup);

    XtPopup(popup, XtGrabNone);
}

void QuitButton(Widget widget, XtPointer client_data, XtPointer call_data) {
    exit(0);
}

void LoadDialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget popup, dialog, load_button;
    XtAppContext app_context = XtWidgetToApplicationContext(widget);

    popup = XtVaCreatePopupShell("load_popup", transientShellWidgetClass, (Widget) client_data,
                                 XtNwidth, 300,
                                 XtNheight, 150,
                                 XtNlabel, "Load",
                                 NULL);
    
    dialog = XtVaCreateManagedWidget("load_dialog", dialogWidgetClass, popup,
                                     XtNlabel, "Enter filename:",
                                     XtNvalue, "mapfile.txt",
                                     NULL);
    
    load_button = XtVaCreateManagedWidget("load_button", commandWidgetClass, dialog,
                                          XtNlabel, "Load",
                                          NULL);

    XtAddCallback(load_button, XtNcallback, LoadFile, (XtPointer) dialog);
    XtAddCallback(load_button, XtNcallback, ClosePopup, (XtPointer) popup);

    XtPopup(popup, XtGrabNone);
}

void LoadFile(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget dialog = (Widget) client_data;
    char *filename = XawDialogGetValueString(dialog);

    if (filename != NULL && *filename != '\0') {
        Display *display = XtDisplay(draw_area);
        Window window = XtWindow(draw_area);

        map = loadMap(filename);

        XClearArea(display, window, 0, 0, 0, 0, True);
    }
}

void DrawSquare(Widget widget, XtPointer client_data, XtPointer call_data) {
    drawingMode = kSquare;
    pivotX = pivotY = -1;
}

void SaveFile(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget dialog = (Widget) client_data;
    char *filename = XawDialogGetValueString(dialog);
    
    if (filename != NULL && *filename != '\0') {
        saveMap(filename, &map);
    }
}

void ClosePopup(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget popup = (Widget) client_data;
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

        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int x = 0; x < GRID_SIZE; ++x) {
                uint32_t flags = getFlags(&map, x, y);

                if ((flags & CELL_VOID) == CELL_VOID) {
                    XSetForeground(display, gc, 0x555555);

                    XFillRectangle(display, window, gc,
                                   x * (attr.width / GRID_SIZE),
                                   y * (attr.height / GRID_SIZE),
                                   (attr.width / GRID_SIZE),
                                   (attr.height / GRID_SIZE)
                    );
                } else if ((flags & CELL_FLOOR) == CELL_FLOOR) {
                    XSetForeground(display, gc, 0xFFFFFFFF);

                    XFillRectangle(display, window, gc,
                                   x * (attr.width / GRID_SIZE),
                                   y * (attr.height / GRID_SIZE),
                                   (attr.width / GRID_SIZE),
                                   (attr.height / GRID_SIZE)
                    );

                }
            }
        }

        XSetForeground(display, gc, 0);

        for (int y = 1; y <= GRID_SIZE; ++y) {
            XDrawLine(display, window, gc, 0, y * (attr.height / GRID_SIZE), attr.width, y * (attr.height / GRID_SIZE));
        }

        for (int x = 1; x <= GRID_SIZE; ++x) {
            XDrawLine(display, window, gc, x * (attr.width / GRID_SIZE), 0, x * (attr.width / GRID_SIZE), attr.height);
        }
        
        
        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int x = 0; x < GRID_SIZE; ++x) {
                uint32_t flags = getFlags(&map, x, y);
                XSetLineAttributes(display, gc, 4, LineSolid, CapButt, JoinMiter);
                
                if ((flags & HORIZONTAL_LINE) == HORIZONTAL_LINE) {
                    XDrawLine(display, window, gc,
                              x * (attr.width / GRID_SIZE),
                              y * (attr.height / GRID_SIZE),
                              (x + 1) * (attr.width / GRID_SIZE),
                              y * (attr.height / GRID_SIZE)
                    );
                }
                
                if ((flags & VERTICAL_LINE) == VERTICAL_LINE) {
                    XDrawLine(display, window, gc,
                              x * (attr.width / GRID_SIZE),
                              y * (attr.height / GRID_SIZE),
                              x * (attr.width / GRID_SIZE),
                              (y + 1) * (attr.height / GRID_SIZE)
                    );
                }
                
                if ((flags & LEFT_NEAR_LINE) == LEFT_NEAR_LINE) {
                    XDrawLine(display, window, gc,
                              (x + 1) * (attr.width / GRID_SIZE),
                              y * (attr.height / GRID_SIZE),
                              (x) * (attr.width / GRID_SIZE),
                              (y + 1) * (attr.height / GRID_SIZE)
                    );
                }
                
                if ((flags & LEFT_FAR_LINE) == LEFT_FAR_LINE) {
                    XDrawLine(display, window, gc,
                              x * (attr.width / GRID_SIZE),
                              y * (attr.height / GRID_SIZE),
                              (x + 1) * (attr.width / GRID_SIZE),
                              (y + 1) * (attr.height / GRID_SIZE)
                    );
                }
                XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
            }
        }
        XFreeGC(display, gc);
    }
}

void ClickCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch) {
    Display *display = XtDisplay(draw_area);
    XButtonEvent *button_event = (XButtonEvent *) event;
    Window window = XtWindow(draw_area);
    XWindowAttributes attr;
    XGetWindowAttributes(display, window, &attr);

    int grid_width = attr.width / GRID_SIZE;
    int grid_height = attr.height / GRID_SIZE;

    int x = button_event->x / grid_width;
    int y = button_event->y / grid_height;

    int x_offset = button_event->x % grid_width;
    int y_offset = button_event->y % grid_height;

    if (event->type == ButtonPress) {
        switch (drawingMode) {
            case kSquare : {
                if (pivotX == -1) {
                    pivotX = x;
                    pivotY = y;
                    printf("Pivot at: %d, %d\n", pivotX, pivotY);
                } else {
                    for (int _y = pivotY; _y < y; ++_y) {
                        for (int _x = pivotX; _x < x; ++_x) {
                            printf("filling %d, %d\n", _x, _y);
                            setFlags(&map, _x, _y, CELL_FLOOR);
                        }
                    }

                    pivotX = pivotY = -1;
                    drawingMode = kPicker;
                }
            }
                break;
            case kPicker: {
                uint32_t flags = getFlags(&map, x, y);

                if (y_offset <= grid_height / 2) {
                    if (x_offset <= grid_width / 2) {
                        if ((flags & VERTICAL_LINE) == VERTICAL_LINE) {
                            flags &= ~VERTICAL_LINE;
                        } else {
                            flags |= VERTICAL_LINE;
                        }
                    } else {
                        if ((flags & HORIZONTAL_LINE) == HORIZONTAL_LINE) {
                            flags &= ~HORIZONTAL_LINE;
                        } else {
                            flags |= HORIZONTAL_LINE;
                        }
                    }
                } else {
                    if (x_offset <= grid_width / 2) {
                        if ((flags & LEFT_NEAR_LINE) == LEFT_NEAR_LINE) {
                            flags &= ~LEFT_NEAR_LINE;
                        } else {
                            flags |= LEFT_NEAR_LINE;
                        }
                    } else {
                        if ((flags & LEFT_FAR_LINE) == LEFT_FAR_LINE) {
                            flags &= ~LEFT_FAR_LINE;
                        } else {
                            flags |= LEFT_FAR_LINE;
                        }
                    }
                }
                setFlags(&map, x, y, flags);
            }
                break;
        }
    }


    expose:
    XClearArea(display, window, 0, 0, 0, 0, True);
}
