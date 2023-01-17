//
// Created by victor on 23.10.2022.
//

#include "clipboard.h"
#include <X11/Xlib.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <X11/Xatom.h>
#include <string>
#include <mutex>

static std::map<Atom,std::vector<char>> targets_map;
static std::vector<Atom> keys;
static Window win;
static Display* disp;

static std::mutex mtx;

void get_targets(Display *dpy, Window w, Atom p){
    Atom type, *targets;
    int di;
    unsigned long i, nitems, dul;
    unsigned char *prop_ret = nullptr;
    char *an = nullptr;

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

        if(std::strcmp(an,"TARGETS") != 0){
            targets_map.insert(make_pair(targets[i],std::vector<char>()));
        }
        if (an)
            XFree(an);
    }
    XFree(prop_ret);

    XDeleteProperty(dpy, w, p);
}

std::vector<char> get_prop(Display *dpy, Window w, Atom p){

    std::vector<char> result;
    bool is_incr = false;
    begin:
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
        is_incr = true;
        printf("using incr\n");
    }

    /* Read the data in one go. */
    XGetWindowProperty(dpy, w, p, 0, size, False, AnyPropertyType,
                       &da, &di, &dul, &dul, &prop_ret);

    std::vector<char> part;
    if(prop_ret && type != incr) {

        part.resize(size);
        std::memcpy(part.data(),prop_ret,size);

        result.insert(result.end(),part.begin(),part.end());

        XFree(prop_ret);
    }

    if(size != 0 && is_incr){
        XDeleteProperty(dpy, w, p);
        XEvent event;
        do{
            XNextEvent(disp,&event);

        }while(event.type != PropertyNotify || event.xproperty.atom != p || event.xproperty.state != PropertyNewValue);
        goto begin;
    }

    XDeleteProperty(dpy, w, p);

    return result;
}

void clipboard_notify_loop(void(*callback)(const std::map<Atom, std::vector<char>>&)){
    disp = XOpenDisplay(nullptr);
    Window root = XDefaultRootWindow(disp);
    win = XCreateSimpleWindow(disp, root, -1, -1, 1, 1, 0, 0, 0);
    Atom clip = XInternAtom(disp, "CLIPBOARD", False);
    Atom target_property = XInternAtom(disp, "BUFFER", False);
    Atom targets_atom = XInternAtom(disp, "TARGETS", False);

    XSelectInput(disp,win,PropertyChangeMask | ExposureMask);
    XSetSelectionOwner(disp,clip,win,CurrentTime);

    XEvent event;

    bool target_req = false;

    auto targets = targets_map.end();
    while (true){
        XNextEvent(disp,&event);
        mtx.lock();
        if(event.type == SelectionClear){

            XConvertSelection(disp, clip, targets_atom, target_property, win, CurrentTime);
            target_req = true;
            targets_map.clear();
        }

        if(event.type == SelectionNotify && target_req){
            target_req = false;
            get_targets(disp,win,target_property);
            targets = targets_map.begin();
            if(targets != targets_map.end()){
                XConvertSelection(disp, clip, (*targets).first, target_property, win, CurrentTime);
            }
            mtx.unlock();
            continue;
        }

        if(event.type == SelectionNotify && !targets_map.empty() && targets != targets_map.end()){
            auto& target = *targets;
            target.second = get_prop(disp,win,target_property);
            targets++;
            if(targets != targets_map.end()){
                XConvertSelection(disp, clip, (*targets).first, target_property, win, CurrentTime);
            }else{
                keys.clear();
                for(const auto& t : targets_map){
                    keys.push_back(t.first);
                }
                XSetSelectionOwner(disp,clip,win,CurrentTime);
                callback(targets_map);
            }
        }

        if(event.type == SelectionRequest){
            if (event.xselectionrequest.selection != clip) continue;
            XSelectionRequestEvent * xsr = &event.xselectionrequest;
            XSelectionEvent ev = {0};
            int R = 0;
            ev.type = SelectionNotify, ev.display = xsr->display, ev.requestor = xsr->requestor,
            ev.selection = xsr->selection, ev.time = xsr->time, ev.target = xsr->target, ev.property = xsr->property;
            printf("target: %s\n", XGetAtomName(disp,ev.target));
            if (ev.target == targets_atom) R = XChangeProperty (ev.display, ev.requestor, ev.property, XA_ATOM, 32,
                                                                PropModeReplace, (unsigned char*)keys.data(), keys.size());
            else if(std::find(keys.begin(),keys.end(),ev.target) != keys.end()){
                auto data = targets_map[ev.target];
                R = XChangeProperty(ev.display, ev.requestor, ev.property, ev.target, 8, PropModeReplace,
                                    (unsigned char*) data.data(), data.size());
            }else{
                ev.property = None;
            }

            if ((R & 2) == 0) XSendEvent (disp, ev.requestor, 0, 0, (XEvent *)&ev);
        }
        mtx.unlock();
        if (event.type == Expose){
            break;
        }
    }
}

void stop_loop(){
    XEvent event;
    event.type = Expose;

    XSendEvent(disp,win,False,ExposureMask,&event);
    XFlush(disp);
}

std::string get_atom_name(Atom atom){
    char* name_x = XGetAtomName(disp,atom);
    std::string result(name_x);
    XFree(name_x);
    return result;
}

Atom get_atom_by_name(std::string_view name){
    return XInternAtom(disp,name.data(),False);
}

void put_to_clipboard(const std::map<Atom, std::vector<char>>& data){
    mtx.lock();
    targets_map = data;
    keys.clear();
    for(const auto& t : targets_map){
        printf("atom: %s\n", XGetAtomName(disp,t.first));
        keys.push_back(t.first);
    }
    mtx.unlock();
}