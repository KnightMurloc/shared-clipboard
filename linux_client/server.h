//
// Created by Victor on 15.07.2021.
//

#ifndef CLIPBOAD_TEST_SERVER_H
#define CLIPBOAD_TEST_SERVER_H

#include <string>
#include <map>
#include <vector>

bool init_server(std::string port, void (*_callback)(const std::map<std::string, std::vector<char>>& data));
void sendData_server(const std::map<std::string, std::vector<char>>& data);
bool server_is_connected();
void disconnect_server();

#endif //CLIPBOAD_TEST_SERVER_H
