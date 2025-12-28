#include "ipcContext.h"
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

/*
 * Initializes socket IPC.
 *
 * Created with help from AI.
 */
_Bool ipc_init_socket(ipc_context_t* this, int port) {
  memset(this, 0, sizeof(*this));
  this->type = IPC_SOCKET;

  int s = socket(AF_INET, SOCK_STREAM, 0); //Address family - IPv4, type of socket(means TCP), let OS choose deafutl protocol
  if (s < 0) return 0;

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port); //host to network short, sets port on which server listens
  addr.sin_addr.s_addr = INADDR_ANY; //sets all IPs
  if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {  //bind assigns socket to a port and IP
    perror("bind failed");
    close(s);
    return 0;
  }

  if (listen(s, 10) < 0) {  //switches socket to different state (listening to clients)
    perror("listen failed");
    close(s);
    return 0;
  }

  this->listen_fd = s;
  return 1;
}

_Bool ipc_init_from_fd(ipc_context_t* this, int fd) {
  memset(this, 0, sizeof(*this));
  this->type = IPC_SOCKET;
  this->socket_fd = fd;
  return 1;
}

/*
 * Initializes pipe IPC.
 *
 * Created with help from AI.
 */
_Bool ipc_init_pipe(ipc_context_t* this) {
  memset(this, 0, sizeof(*this));
  this->type = IPC_PIPE;

  if (pipe(this->pipefd) < 0)  //system call pipe() creates two ends of pipe
    return 0;
  return 1;
}

/*
 * Creates and initializes shared memory and POSIX semaphore.
 *
 * Created with help from AI.
 */
_Bool ipc_init_semaphore(ipc_context_t* this, const char* name, size_t size) {
  memset(this, 0, sizeof(*this));
  this->type = IPC_SEMAPHORE;
  this->shm_size = size;

  this->shm_id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666); //creates shared segment; name of semaphore, open or create semaphore, UNIX access priviligaes, start value of semaphore
  if (this->shm_id < 0) return 0;

  this->shm_ptr = shmat(this->shm_id, NULL, 0); //connects shared segment with memory of process 
  if (this->shm_ptr == (void*)-1) return 0;

  this->sem = sem_open(name, O_CREAT, 0666, 1); //creates POSIX semaphore
  if (this->sem == SEM_FAILED) return 0;

  return 1;
}

/*
 * Sends data based on IPC type.
 *
 * Created with help from AI.
 */
int  ipc_send(ipc_context_t* this, const void* data, size_t size) {
  switch (this->type)
  {
    case IPC_SOCKET:
      return send(this->socket_fd, data, size, 0);

    case IPC_PIPE:
      return write(this->pipefd[1], data, size);

    case IPC_SEMAPHORE:
      sem_wait(this->sem); //waits till counter is >0, if it is counter--
      memcpy(this->shm_ptr, data, size); //copy files
      sem_post(this->sem); //conter++ and unblocks waiting processes
      return (int)size;
  }
  return -1;
}

/*
 * Receives data based on IPC type.
 *
 * Created with help from AI.
 */
int  ipc_receive(ipc_context_t* this, void* buffer, size_t size) {
  switch (this->type)
  {
    case IPC_SOCKET:
      return recv(this->socket_fd, buffer, size, 0);

    case IPC_PIPE:
      return read(this->pipefd[0], buffer, size);

    case IPC_SEMAPHORE:
      sem_wait(this->sem);
      memcpy(buffer, this->shm_ptr, size);
      sem_post(this->sem);
      return (int)size;
  }
  return -1;
}

/*
 * Accepts new clients connection.
 *
 * Created with help from AI.
 */
int  ipc_accept_socket(ipc_context_t* this) {
  if (this->type != IPC_SOCKET) return -1;
  return accept(this->listen_fd, NULL, NULL);
}

/*
 * Returns deafult file descriptor (defines OS on which open object to apply action).
 *
 * Created with help from AI.
 */
int  ipc_fd(ipc_context_t* this) {
  if (this->type == IPC_SOCKET)
    return this->listen_fd;
  if (this->type == IPC_PIPE)
    return this->pipefd[0];
  return -1;
}

/*
 * Releases all sources.
 *
 * Created with help from AI.
 */
void ipc_close(ipc_context_t* this) {
  if (this->type == IPC_SOCKET)
    close(this->listen_fd);

  if (this->type == IPC_PIPE) {
    close(this->pipefd[0]);
    close(this->pipefd[1]);
  }
  if (this->type == IPC_SEMAPHORE) {
    shmdt(this->shm_ptr);
    shmctl(this->shm_id, IPC_RMID, NULL);
    sem_close(this->sem);
  }
}

