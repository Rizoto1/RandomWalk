#ifndef SERVER_THREADS_H
#define SERVER_THREADS_H

void* server_recv_thread(void* arg);
void* server_send_thread(void* arg);
void* simulation_thread(void* arg);

#endif
