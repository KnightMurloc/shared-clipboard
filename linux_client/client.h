//
// Created by Victor on 16.07.2021.
//

#ifndef S_CLIPBOARD_CLIENT_H
#define S_CLIPBOARD_CLIENT_H

#include <string>

bool init_client(std::string ip, std::string port, void (*_callback)(std::string));
bool client_is_connected();
void sendData_client(std::string msg);
void disconnect_client();

#endif //S_CLIPBOARD_CLIENT_H
