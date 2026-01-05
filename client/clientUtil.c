#include "clientUtil.h"
#include "ipc/ipcUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

#define WIDTH 42

void print_message(const char *s, _Bool error, _Bool userInput)
{
  int col = 0;
  printf("\n");
  if (error) {
    printf(
      "            ---------------------------------\n"
      "                         Error: \n"
    );
  }

  while (*s) {
    if (*s == '\n') {
      putchar('\n');
      col = 0;
      s++;
      continue;
    }

    if(col == 0) {
      printf("       ");
    }

    //count word without shift
    const char *word = s;
    int len = 0;

    while(word[len] && !isspace(word[len]) && word[len] != '\n')
      len++;

    //if word too long split sentence
    if (col + len > WIDTH) {
      putchar('\n');
      col = 0;
      continue;
    }

    //print word
    fwrite(word, 1, len, stdout);
    col += len;
    s += len;

    //space
    while (*s && isspace(*s) && *s != '\n') {
      putchar(*s++);
      col++;
    }
  }
  if (!userInput) {
    printf(
      "\n"
      "            ---------------------------------\n"
      "\n"
    );
  }
}

void print_article(_Bool wLine) {
  clear_screen();
  printf(
    "   ██████╗  █████╗ ███╗   ██╗██████╗  ██████╗ ███╗   ███╗\n"
    "   ██╔══██╗██╔══██╗████╗  ██║██╔══██╗██╔═══██╗████╗ ████║\n"
    "   ██████╔╝███████║██╔██╗ ██║██║  ██║██║   ██║██╔████╔██║\n"
    "   ██╔══██╗██╔══██║██║╚██╗██║██║  ██║██║   ██║██║╚██╔╝██║\n"
    "   ██║  ██║██║  ██║██║ ╚████║██████╔╝╚██████╔╝██║ ╚═╝ ██║\n"
    "   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝     ╚═╝\n"
    "\n"
    "           ██╗    ██╗ █████╗ ██╗     ██╗  ██╗\n"
    "           ██║    ██║██╔══██╗██║     ██║ ██╔╝\n"
    "           ██║ █╗ ██║███████║██║     █████╔╝ \n"
    "           ██║███╗██║██╔══██║██║     ██╔═██╗ \n"
    "           ╚███╔███╔╝██║  ██║███████╗██║  ██╗\n"
    "            ╚══╝╚══╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝\n"
    "\n"
  );
  if (wLine) {
    printf(
    "            ---------------------------------\n"
    );
  }
}

void print_mm(void) {
  printf(
    "\n"
    "                    [ 1 ]  NEW GAME\n"
    "                    [ 2 ]  CONNECT\n"
    "                    [ 3 ]  LOAD\n"
    "                    [ 0 ]  EXIT\n"
    "\n"
    "            ---------------------------------\n"
    "\n"
    "                    Choose: "
  );
}

int read_input(
    input_type_t type,
    void *out,
    size_t outSize //for strings
) {
  char buf[128];
  while (1) {
    if (!fgets(buf, sizeof(buf), stdin))
      return -1;

    buf[strcspn(buf, "\n")] = '\0';

    if (buf[0] == '\0')
      continue;

    break;
  }

  if (strcmp(buf, "q") == 0)
    return 1;

  char *end;

  switch (type) {
    case INPUT_INT: {
      long v = strtol(buf, &end, 10);
      if (*end != '\0')
        return -1;
      *(int *)out = (int)v;
      break;
    }

    case INPUT_DOUBLE: {
      double v = strtod(buf, &end);
      if (*end != '\0')
        return -1;
      *(double *)out = v;
      break;
    }

    case INPUT_STRING: {
      strncpy((char *)out, buf, outSize - 1);
      ((char *)out)[outSize - 1] = '\0';
      break;
    }
  }

  return 0;
}
