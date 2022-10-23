//
// Created by Victor on 15.07.2021.
//

#ifndef CLIPBOAD_TEST_SERVER_H
#define CLIPBOAD_TEST_SERVER_H

#include <string>

bool init_server(std::string port, void (*_callback)(std::string));
void sendData_server(std::string msg);
bool server_is_connected();
void disconnect_server();
#endif //CLIPBOAD_TEST_SERVER_H
