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
static char* current_text = nullptr;
static int text_size = 0;
static int accept = 0;

int clip_board_init(){

    display = XOpenDisplay(NULL);
    XInitThreads();

    Window root = XDefaultRootWindow(display);

    win = XCreateSimpleWindow(display, root, -1, -1, 1, 1, 0, 0, 0);
    XInitThreads();
//    Atom sel = XInternAtom(display, "CLIPBOARD", False);
//    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);

//    Atom target_property = XInternAtom(display, "BUFFER", False);

//    XConvertSelection(display, sel, utf8, target_property, win, CurrentTime);

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
    printf("size: %ld\n",size);
    XFree(prop_ret);

    incr = XInternAtom(dpy, "INCR", False);
    if (type == incr)
    {
        *error = 1;
        printf("Data too large and INCR mechanism not implemented\n");
    }
//    printf("type: %s\n", XGetAtomName(dpy,type));
    /* Read the data in one go. */
    XGetWindowProperty(dpy, w, p, 0, size, False, AnyPropertyType,
                       &da, &di, &dul, &dul, &prop_ret);

    char* result = NULL;
    if(prop_ret) {
        result = strdup((char*) prop_ret);
        *error = 0;
        text_size = size;
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

void
show_targets(Display *dpy, Window w, Atom p)
{
    Atom type, *targets;
    int di;
    unsigned long i, nitems, dul;
    unsigned char *prop_ret = NULL;
    char *an = NULL;

    /* Read the first 1024 atoms from this list of atoms. We don't
     * expect the selection owner to be able to convert to more than
     * 1024 different targets. :-) */
    XGetWindowProperty(dpy, w, p, 0, 1024 * sizeof (Atom), False, XA_ATOM,
                       &type, &di, &nitems, &dul, &prop_ret);

    printf("Targets:\n");
    targets = (Atom *)prop_ret;
    for (i = 0; i < nitems; i++)
    {
        an = XGetAtomName(dpy, targets[i]);
        printf("    '%s'\n", an);
//        if(an && strcmp(an,"UTF8_STRING") == 0){
//            XConvertSelection(dpy,XInternAtom(dpy,"CLIPBOARD",False),targets[i],p,w,CurrentTime);
//            accept = 1;
//            XFree(an);
//            break;
//        }
        if (an)
            XFree(an);
    }
    XFree(prop_ret);

    XDeleteProperty(dpy, w, p);
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

void put_to_clipboard(const char* text){
    Atom clip = XInternAtom(display, "CLIPBOARD", False);
    free(current_text);
    current_text = strdup(text);
    XSetSelectionOwner (display, clip, win, 0);
    if (XGetSelectionOwner (display, clip) != win){
        printf("error\n");
    }
}



void clipboard_notify_loop(void(*callback)(char* text)){
    Atom target_property = XInternAtom(display, "BUFFER", False);
    Atom sel = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
    Atom image = XInternAtom(display, "image/png", False);
    Atom clip = XInternAtom(display, "CLIPBOARD", False);
    Atom targets_atom = XInternAtom(display, "TARGETS", 0);
    Atom text_atom = XInternAtom(display, "TEXT", 0);
    XEvent event;

//    XFixesSelectSelectionInput(display, win, clip, XFixesSetSelectionOwnerNotifyMask);

//    char* last_text = get_from_clipboard();
//    Atom last_owner = XGetSelectionOwner(display,clip);
    int error = 0;
    int lost = 0;
    XSelectInput(display,win,ExposureMask);
    XSetSelectionOwner(display,clip,win,CurrentTime);
//    XConvertSelection(display, clip, targets_atom, target_property, win, CurrentTime);
    while (1){

        XNextEvent(display,&event);
        printf("event.type: %d\n",event.type);
        if(event.type == SelectionClear){
            printf("lost\n");
            lost = 1;
            XConvertSelection(display, clip, utf8, target_property, win, CurrentTime);
        }

        if(event.type == SelectionNotify && lost){
//            free(current_text);
            current_text = show_utf8_prop(display,win,target_property,&error);
//            printf("text: %s\n",current_text ? current_text : "(null)");
            if(error){
                printf("send new req\n");
                XConvertSelection(display, clip, image, target_property, win, CurrentTime);
                continue;
            }
//            if(error == 4){
//                continue;
//            }else{
//                printf("text: %s\n",current_text ? current_text : "(null)");
//            }
            printf("text: %s\n",current_text ? current_text : "(null)");
            lost = 0;
//            show_targets(display,win,target_property);
            XSetSelectionOwner(display,clip,win,CurrentTime);
        }

//        if(event.type == SelectionNotify && accept){
//            accept = 0;
//            current_text = show_utf8_prop(display,win,target_property,&error);
//            printf("text: %s\n",current_text ? current_text : "(null)");
//
//        }

//        printf("%d\n", event.type);
//        if(event.type == 87){
//            XSelectionEvent e = event.xselection;
//            Atom owner = XGetSelectionOwner(display,clip);
//
//            XConvertSelection(display, sel, utf8, target_property, win, CurrentTime);
//            if (e.property == None)
//            {
//                free(last_text);
//                last_text = NULL;
//                printf("Conversion could not be performed.\n");
//            }
//            else
//            {
//                error = 0;
//                char* text = show_utf8_prop(display, win, target_property,&error);
//                printf("%s\n", text ? text : "(null)");
//                if(error == 0 && text){
//                    if((last_text == NULL || (strcmp(text,last_text) != 0) && owner != win)){
//                        free(last_text);
//                        last_text = strdup(text);
//                        callback(text);
//                    }else{
//                        free(text);
//                    }
//                }
//            }
//        }

        if(event.type == SelectionRequest){
            if (event.xselectionrequest.selection != sel) continue;
            XSelectionRequestEvent * xsr = &event.xselectionrequest;
            XSelectionEvent ev = {0};
            int R = 0;
            ev.type = SelectionNotify, ev.display = xsr->display, ev.requestor = xsr->requestor,
            ev.selection = xsr->selection, ev.time = xsr->time, ev.target = xsr->target, ev.property = xsr->property;
            printf("target: %s\n",XGetAtomName(display,ev.target));
            if (ev.target == targets_atom) R = XChangeProperty (ev.display, ev.requestor, ev.property, XA_ATOM, 32,
                                                                PropModeReplace, (unsigned char*)&utf8, 1);
            else if (ev.target == XA_STRING || ev.target == text_atom)
                R = XChangeProperty(ev.display, ev.requestor, ev.property, XA_STRING, 8, PropModeReplace,
                                    reinterpret_cast<const unsigned char*>(current_text), strlen(current_text));
            else if (ev.target == utf8)
                R = XChangeProperty(ev.display, ev.requestor, ev.property, utf8, 8, PropModeReplace,
                                    reinterpret_cast<const unsigned char*>(current_text), strlen(current_text));
            else if (ev.target == image)
                R = XChangeProperty(ev.display,ev.requestor,ev.property,image,8,PropModeReplace,
                        reinterpret_cast<const unsigned char*>(current_text),text_size);
            else ev.property = None;
            if ((R & 2) == 0) XSendEvent (display, ev.requestor, 0, 0, (XEvent *)&ev);
        }

        if(event.type == Expose){
            break;
        }
    }

//    free(last_text);
}

void stop_loop(){
    XEvent event;
    event.type = Expose;

    XSendEvent(display,win,False,ExposureMask,&event);
    XFlush(display);
}