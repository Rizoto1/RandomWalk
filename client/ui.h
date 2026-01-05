#ifndef UI_H
#define UI_H

#include <game/walker.h>
#include <ipc/ipcSocket.h>

void draw_interactive_map(const char* buffer, position_t* posBuf, packet_header_t* hdr);
void draw_summary_map(double* buf, packet_header_t* hdr);
void new_game(void);
void connect_to_game(void);
void load_game(void);
void main_menu(void);

#endif

