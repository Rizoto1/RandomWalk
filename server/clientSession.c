#include "clientSession.h"
#include "ipcContext.h"
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

_Bool client_session_init(client_session_t* this, int id, ipc_context_t* ctx)
{
  this->id = id;
  this->connected = 1;
  memcpy(&this->ctx, ctx, sizeof(ipc_context_t));
  return 1;
}

int client_session_send(client_session_t* this, const void* data, size_t size)
{
  return send(this->ctx.socket_fd, data, size, 0);
}

int client_session_receive(client_session_t* this, void* buffer, size_t size)
{
  return recv(this->ctx.socket_fd, buffer, size, MSG_DONTWAIT);
}

void client_session_close(client_session_t* this)
{
  ipc_close(&this->ctx);
  this->connected = 0;
}

int client_session_fd(client_session_t* this)
{
  return this->ctx.socket_fd;
}

