//
// Created by Victor on 16.07.2021.
//

#ifndef S_CLIPBOARD_CLIENT_H
#define S_CLIPBOARD_CLIENT_H

#include <string>
#include <map>
#include <vector>

bool init_client(std::string ip, std::string port, void (*_callback)(const std::map<std::string, std::vector<char>>& data));
bool client_is_connected();
void sendData_client(const std::map<std::string, std::vector<char>>& data);
void disconnect_client();

#endif //S_CLIPBOARD_CLIENT_H
