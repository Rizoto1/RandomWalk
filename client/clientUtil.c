#include "clientUtil.h"
#include "ipc/ipcUtil.h"
#include <stdio.h>

void ctx_destroy(client_context_t* ctx) {
  if (!ctx) return;
  ipc_destroy(ctx->ipc);
}

int ctx_init(client_context_t* ctx, ipc_ctx_t* ipc) {
  if (!ctx) return 1;
  ctx->running = 1;
  ctx->ipc = ipc;
  return 0;
}

void clear_screen(void) {
  printf("\033[2J\033[1;1H");
}
