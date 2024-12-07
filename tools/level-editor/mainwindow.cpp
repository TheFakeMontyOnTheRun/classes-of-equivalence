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
int cursorX, cursorY;

struct Map map;
Widget topLevel;
Widget draw_area;
enum EDrawingMode drawingMode = kPicker;

void PopupDialog(Widget widget, XtPointer client_data, XtPointer call_data);

void SaveAsDialog(Widget widget, XtPointer client_data, XtPointer call_data);

void SaveFile(Widget widget, XtPointer client_data, XtPointer call_data);

void QuitButton(Widget widget, XtPointer client_data, XtPointer call_data);

void LoadDialog(Widget widget, XtPointer client_data, XtPointer call_data);

void LoadFile(Widget widget, XtPointer client_data, XtPointer call_data);

void DrawSquare(Widget widget, XtPointer client_data, XtPointer call_data);

void DrawLine(Widget widget, XtPointer client_data, XtPointer call_data);

void ClosePopup(Widget widget, XtPointer client_data, XtPointer call_data);

void ExposeCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch);

void ClickCallback(Widget widget, XtPointer client_data, XEvent *event, Boolean *dispatch);

void handle_motion(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch);

int main(int argc, char **argv) {
    XtAppContext app_context;
    Widget form, button, save_button, load_button, viewport, quit_button;
    Widget draw_line_button, draw_square_button;
    Display *display;
    int screen;
    Window window;
    map = initMap(32, 32);
    topLevel = XtVaAppInitialize(&app_context, "XawExample", NULL, 0, &argc, argv, NULL, NULL);

    form = XtVaCreateManagedWidget("form", formWidgetClass, topLevel, NULL);

    button = XtVaCreateManagedWidget("button", commandWidgetClass, form,
                                     XtNlabel, "About",
                                     XtNfromVert, NULL,
                                     XtNfromHoriz, NULL,
                                     NULL);

    save_button = XtVaCreateManagedWidget("save_button", commandWidgetClass, form,
                                          XtNlabel, "Save As",
                                          XtNfromVert, button,
                                          XtNfromHoriz, NULL,
                                          NULL);

    load_button = XtVaCreateManagedWidget("load_button", commandWidgetClass, form,
                                          XtNlabel, "Load",
                                          XtNfromVert, save_button,
                                          XtNfromHoriz, NULL,
                                          NULL);

    quit_button = XtVaCreateManagedWidget("quit_button", commandWidgetClass, form,
                                          XtNlabel, "Quit",
                                          XtNfromVert, load_button,
                                          XtNfromHoriz, NULL,
                                          NULL);

    draw_square_button = XtVaCreateManagedWidget("draw_square_button", commandWidgetClass, form,
                                                 XtNlabel, "Draw Square",
                                                 XtNfromVert, NULL,
                                                 XtNfromHoriz, button,
                                                 NULL);

    draw_line_button = XtVaCreateManagedWidget("draw_line_button", commandWidgetClass, form,
                                               XtNlabel, "Draw Line",
                                               XtNfromVert, draw_square_button,
                                               XtNfromHoriz, save_button,
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
    XtAddCallback(draw_line_button, XtNcallback, DrawLine, (XtPointer) topLevel);
    XtAddCallback(draw_square_button, XtNcallback, DrawSquare, (XtPointer) topLevel);
    XtAddEventHandler(draw_area, ExposureMask, False, ExposeCallback, NULL);
    XtAddEventHandler(draw_area, PointerMotionMask, False, handle_motion, NULL);
    XtAddEventHandler(draw_area, ButtonPressMask, False, ClickCallback, NULL);

    XtRealizeWidget(topLevel);

    display = XtDisplay(topLevel);
    window = XtWindow(draw_area);
    screen = DefaultScreen(display);

    XMapWindow(display, window);

    XtAppMainLoop(app_context);

    return 0;
}

void handle_motion(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch) {
    if (event->type == MotionNotify && drawingMode == kSquare && pivotX != -1) {
        Display *display = XtDisplay(draw_area);
        Window window = XtWindow(draw_area);
        XMotionEvent *motion_event = (XMotionEvent *) event;
        XWindowAttributes attr;
        XGetWindowAttributes(display, window, &attr);

        cursorX = motion_event->x * (attr.width / GRID_SIZE);
        cursorY = motion_event->y * (attr.height / GRID_SIZE);

        int y0 = (pivotY < cursorY) ? pivotY : cursorY;
        int y1 = (pivotY < cursorY) ? cursorY : pivotY;
        int x0 = (pivotX < cursorX) ? pivotX : cursorX;
        int x1 = (pivotX < cursorX) ? cursorX : pivotX;

        XClearArea(display, window,
                   x0 * (GRID_SIZE / attr.width),
                   y0 * (GRID_SIZE / attr.height),
                   (x1 - x0) * (GRID_SIZE / attr.width),
                   (y1 - y0) * (GRID_SIZE / attr.height),
                   True);
    }
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

void DrawLine(Widget widget, XtPointer client_data, XtPointer call_data) {
    drawingMode = kLine;
    pivotX = pivotY = -1;
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
        XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
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
            case kLine: {
                if (pivotX == -1) {
                    pivotX = x;
                    pivotY = y;
                    printf("Pivot at: %d, %d\n", pivotX, pivotY);
                } else {

                    if (x == pivotX) {
                        int y0 = (pivotY < y) ? pivotY : y;
                        int y1 = (pivotY < y) ? y : pivotY;
                        for (int _y = y0; _y <= y1; ++_y) {
                            printf("filling %d, %d\n", pivotX, _y);
                            setFlags(&map, pivotX, _y, VERTICAL_LINE);
                        }
                    } else if (y == pivotY) {
                        int x0 = (pivotX < x) ? pivotX : x;
                        int x1 = (pivotX < x) ? x : pivotX;

                        for (int _x = x0; _x <= x1; ++_x) {
                            printf("filling %d, %d\n", _x, pivotY);
                            setFlags(&map, _x, pivotY, HORIZONTAL_LINE);
                        }
                    }


                    pivotX = pivotY = -1;
                    drawingMode = kPicker;
                }
            }
                break;
            case kSquare : {
                if (pivotX == -1) {
                    pivotX = x;
                    pivotY = y;
                    printf("Pivot at: %d, %d\n", pivotX, pivotY);
                } else {
                    int y0 = (pivotY < y) ? pivotY : y;
                    int y1 = (pivotY < y) ? y : pivotY;
                    int x0 = (pivotX < x) ? pivotX : x;
                    int x1 = (pivotX < x) ? x : pivotX;

                    uint32_t flag = event->xbutton.button == 1 ? CELL_FLOOR : CELL_VOID;

                    for (int _y = y0; _y < y1; ++_y) {
                        for (int _x = x0; _x < x1; ++_x) {
                            printf("filling %d, %d\n", _x, _y);
                            setFlags(&map, _x, _y, flag);
                        }
                    }

                    pivotX = pivotY = -1;
                    drawingMode = kPicker;
                }
            }
                break;
            case kPicker: {
                uint32_t flags;
                if (event->xbutton.button == 1) {
                    flags = getFlags(&map, x, y);

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
                } else {
                    flags = getFlags(&map, x, y);
                    if ((flags & CELL_FLOOR) == CELL_FLOOR) {
                        flags &= ~CELL_FLOOR;
                        flags |= CELL_VOID;
                    } else {
                        flags = CELL_VOID;
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
