#ifndef SERVER_THREADS_H
#define SERVER_THREADS_H

#define SLEEP_SUMMARY 200000
#define SLEEP_INTERACTIVE 300000

void* server_recv_thread(void* arg);
void* server_accept_thread(void* arg);
void* server_send_thread(void* arg);
void* simulation_thread(void* arg);
int server_init(char** argv);

#endif
