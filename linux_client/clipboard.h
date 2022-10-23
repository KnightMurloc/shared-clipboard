//
// Created by victor on 23.10.2022.
//

#ifndef S_CLIPBOARD_CLIPBOARD_H
#define S_CLIPBOARD_CLIPBOARD_H
int clip_board_init();

char* get_from_clipboard();
void put_to_clipboard(const char* text);

void clipboard_notify_loop(void(*callback)(char* text));

void stop_loop();
#endif //S_CLIPBOARD_CLIPBOARD_H
