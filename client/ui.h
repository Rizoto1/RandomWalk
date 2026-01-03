#ifndef UI_H
#define UI_H

#include <game/walker.h>
#include <ipc/ipcSocket.h>

void draw_interactive_map(const char* buffer, position_t* posBuf, packet_header_t* hdr);
void draw_summary_map(char* buffer, position_t* posBuf, packet_header_t* hdr);
void newGame(void);
void connectToGame(void);
void continueInGame(void);
void mainMenu(void);

#endif

