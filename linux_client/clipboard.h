//
// Created by victor on 23.10.2022.
//

#ifndef S_CLIPBOARD_CLIPBOARD_H
#define S_CLIPBOARD_CLIPBOARD_H

#include <map>
#include <vector>
#include <X11/Xlib.h>
#include <string>

void put_to_clipboard(const std::map<Atom, std::vector<char>>&);

void clipboard_notify_loop(void(*callback)(const std::map<Atom, std::vector<char>>&));

void stop_loop();

std::string get_atom_name(Atom atom);
Atom get_atom_by_name(std::string_view name);

#endif //S_CLIPBOARD_CLIPBOARD_H
