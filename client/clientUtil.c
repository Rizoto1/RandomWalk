#include "clientUtil.h"

void ctx_destroy(client_context_t* ctx) {
  if (!ctx) return;
    ctx->pipe = NULL;
    ctx->shm = NULL;
    ctx->socket = NULL;
  
}

void ctx_init(client_context_t* ctx) {
  if (!ctx) return;
  ctx->running = 1;
  ctx->socket = NULL;
  ctx->type = 2;
  ctx->shm = NULL;
  ctx->pipe = NULL;
}
