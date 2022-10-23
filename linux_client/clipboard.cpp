//
// Created by victor on 23.10.2022.
//

#include "clipboard.h"

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/Xatom.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>

static Display* display;
static Window win;

int clip_board_init(){

    display = XOpenDisplay(NULL);
    XInitThreads();

    Window root = XDefaultRootWindow(display);

    win = XCreateSimpleWindow(display, root, -1, -1, 1, 1, 0, 0, 0);

    Atom sel = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);

    Atom target_property = XInternAtom(display, "BUFFER", False);

    XConvertSelection(display, sel, utf8, target_property, win, CurrentTime);

    return 1;
}

char* show_utf8_prop(Display *dpy, Window w, Atom p, int* error)
{
    Atom da, incr, type;
    int di;
    unsigned long size, dul;
    unsigned char *prop_ret = NULL;

    /* Dummy call to get type and size. */
    XGetWindowProperty(dpy, w, p, 0, 0, False, AnyPropertyType,
                       &type, &di, &dul, &size, &prop_ret);

    XFree(prop_ret);

    incr = XInternAtom(dpy, "INCR", False);
    if (type == incr)
    {
        *error = 1;
        printf("Data too large and INCR mechanism not implemented\n");
    }

    /* Read the data in one go. */
    XGetWindowProperty(dpy, w, p, 0, size, False, AnyPropertyType,
                       &da, &di, &dul, &dul, &prop_ret);

    char* result = NULL;
    if(prop_ret) {
        result = strdup((char*) prop_ret);
        XFree(prop_ret);
    }

    if(result == NULL){
        *error = 2;
    }

    if(size == 0){
        *error = 3;
    }

    /* Signal the selection owner that we have successfully read the
     * data. */
    XDeleteProperty(dpy, w, p);
    return result;
}

char* get_from_clipboard(){
    XEvent ev;
    XSelectionEvent* sev;
    Atom target_property = XInternAtom(display, "BUFFER", False);
    Atom sel = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
    for (;;)
    {
        XNextEvent(display, &ev);
        switch (ev.type)
        {
            case SelectionNotify:
                sev = (XSelectionEvent*)&ev.xselection;
                XConvertSelection(display, sel, utf8, target_property, win, CurrentTime);
                if (sev->property == None)
                {
                    return NULL;
                }
                else
                {
                    int error = 0;
                    char* tmp =show_utf8_prop(display, win, target_property,&error);
                    if(error == 0){
                        return tmp;
                    }
                }
        }
    }
}

static void XCopy(Atom selection, unsigned char * text, int size) {
    Atom targets_atom = XInternAtom(display, "TARGETS", 0);
    Atom text_atom = XInternAtom(display, "TEXT", 0);
    Atom UTF8 = XInternAtom(display, "UTF8_STRING", 1);

    XEvent event;
    XSetSelectionOwner (display, selection, win, 0);
    if (XGetSelectionOwner (display, selection) != win) return;
    while (1) {
        XNextEvent (display, &event);
        switch (event.type) {
            case SelectionClear:
                return;
            case SelectionRequest:
                if (event.xselectionrequest.selection != selection) break;
                XSelectionRequestEvent * xsr = &event.xselectionrequest;
                XSelectionEvent ev = {0};
                int R = 0;
                ev.type = SelectionNotify, ev.display = xsr->display, ev.requestor = xsr->requestor,
                ev.selection = xsr->selection, ev.time = xsr->time, ev.target = xsr->target, ev.property = xsr->property;
                if (ev.target == targets_atom) R = XChangeProperty (ev.display, ev.requestor, ev.property, XA_ATOM, 32,
                                                                    PropModeReplace, (unsigned char*)&UTF8, 1);
                else if (ev.target == XA_STRING || ev.target == text_atom)
                    R = XChangeProperty(ev.display, ev.requestor, ev.property, XA_STRING, 8, PropModeReplace, text, size);
                else if (ev.target == UTF8)
                    R = XChangeProperty(ev.display, ev.requestor, ev.property, UTF8, 8, PropModeReplace, text, size);
                else ev.property = None;
                if ((R & 2) == 0) XSendEvent (display, ev.requestor, 0, 0, (XEvent *)&ev);
                break;
        }
    }
}

void put_to_clipboard(const char* text){
    Atom clip = XInternAtom(display, "CLIPBOARD", False);
    XCopy(clip,(unsigned char*) text,(int) strlen(text));
}

void clipboard_notify_loop(void(*callback)(char* text)){
    Atom target_property = XInternAtom(display, "BUFFER", False);
    Atom sel = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
    Atom clip = XInternAtom(display, "CLIPBOARD", False);
    XEvent event;

    XFixesSelectSelectionInput(display, win, clip, XFixesSetSelectionOwnerNotifyMask);

    char* last_text = get_from_clipboard();
    Atom last_owner = XGetSelectionOwner(display,clip);
    int error = 0;

    XSelectInput(display,win,ExposureMask);

    while (1){

        XNextEvent(display,&event);
        if(event.type == 87){
            XSelectionEvent e = event.xselection;
            Atom owner = XGetSelectionOwner(display,clip);

            XConvertSelection(display, sel, utf8, target_property, win, CurrentTime);
            if (e.property == None)
            {
                free(last_text);
                last_text = NULL;
                printf("Conversion could not be performed.\n");
            }
            else
            {
                error = 0;
                char* text = show_utf8_prop(display, win, target_property,&error);
                if(error == 0 && text){
                    if(last_text == NULL || (strcmp(text,last_text) != 0 && owner != last_owner)){
                        free(last_text);
                        last_text = strdup(text);
                        callback(text);
                    }else{
                        free(text);
                    }
                }
            }
        }

        if(event.type == Expose){
            break;
        }
    }

    free(last_text);
}

void stop_loop(){
    XEvent event;
    event.type = Expose;

    XSendEvent(display,win,False,ExposureMask,&event);
    XFlush(display);
}