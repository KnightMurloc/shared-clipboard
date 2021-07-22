//
// Created by Victor on 15.07.2021.
//

#ifndef S_CLIPBOARD_SERVER_H
#define S_CLIPBOARD_SERVER_H

#include <string>

bool init_server(std::string port, void (*_callback)(std::wstring*));
void sendData_server(std::wstring* msg);
bool server_is_connected();
void disconnect_server();
#endif //S_CLIPBOARD_SERVER_H
