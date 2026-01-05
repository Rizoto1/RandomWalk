#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#include <stdatomic.h>
#include <ipc/ipcUtil.h>

typedef struct {
  ipc_ctx_t* ipc;
  atomic_bool running;
} client_context_t;

typedef enum {
  INPUT_INT,
  INPUT_DOUBLE,
  INPUT_STRING
} input_type_t;

void ctx_destroy(client_context_t* ctx);
int ctx_init(client_context_t* ctx, ipc_ctx_t* ipc);

void clear_screen(void);
void print_message(const char *s, _Bool error, _Bool userInput);
void print_article(_Bool wLine);
void print_mm(void);
int read_input(
    input_type_t type,
    void *out,
    size_t outSize //for strings
);
#endif
