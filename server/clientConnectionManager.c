#include "clientConnectionManager.h"
#include "ipcContext.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

_Bool conn_manager_init(client_connection_manager_t* this, int port)
{
  this->count = 0;

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    return 0;
  }

  int opt = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(s);
    return 0;
  }
  if (listen(s, 10) < 0) {
    perror("listen");
    close(s);
    return 0;
  }

  this->listen_fd = s;
  printf("[CONN-MGR] Listening on port %d\n", port);
  return 1;
}

client_session_t* conn_manager_accept(client_connection_manager_t* this)
{
  int fd = accept(this->listen_fd, NULL, NULL);
  if (fd < 0) return NULL;

  if (this->count >= MAX_CLIENTS) {
    close(fd);
    return NULL;
  }

  ipc_context_t ctx;
  ipc_init_from_fd(&ctx, fd);

  client_session_t* s = &this->sessions[this->count];
  client_session_init(s, this->count, &ctx);

  printf("[CCM] Accepted session id=%d fd=%d\n", s->id, client_session_fd(s));

  this->count++;
  return s;
}

void conn_manager_poll(client_connection_manager_t* this)
{
  char buf[256];

  for (int i = 0; i < this->count; ++i) {
    client_session_t* s = &this->sessions[i];
    if (!s->connected) continue;

    int bytes = client_session_receive(s, buf, sizeof(buf) - 1);
    if (bytes > 0) {
      buf[bytes] = '\0';
      printf("[CCM] msg from session %d: %s\n", s->id, buf);
      client_session_send(s, buf, bytes);
    } else if (bytes == 0) {
      printf("[CCM] session %d disconnected\n", s->id);
      client_session_close(s);
    }
  }
}

int conn_manager_get_count(client_connection_manager_t* this)
{
  return this->count;
}

client_session_t* conn_manager_get_session(client_connection_manager_t* this, int index)
{
  if (index < 0 || index >= this->count) return NULL;
  return &this->sessions[index];
}

void conn_manager_broadcast(client_connection_manager_t* this, const void* data, size_t len)
{
  for (int i = 0; i < this->count; ++i) {
    client_session_t* s = &this->sessions[i];
    if (s->connected)
      client_session_send(s, data, len);
  }
}

void conn_manager_close(client_connection_manager_t* this)
{
  for (int i = 0; i < this->count; ++i) {
    client_session_close(&this->sessions[i]);
  }
  close(this->listen_fd);
}

