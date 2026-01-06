# RandomWalk - Client/Server simulation

This is a semester assignment for Principles of Operating Systems (POS) in Faculty of Management Science and Informatics of University of Žilina. It's purpose is to implement a client-server application for simulating Random Walk in a 2D space. The application is written in the C programming language and uses multithreading and inter-process communication (IPC).

 ---

## Project Description

The application simulates the movement of walker in a 2D world, based on random steps.  
The server manages the simulation logic and world state, while the client is used to control and observe the simulation.

The project is divided into several logical modules:
- **server** – simulation control, client management, communication handling
- **client** – user interface and communication with the server
- **game** – simulation logic (world, walker, simulation)
- **ipc** – inter-process communication utilities (sockets, ~~pipes~~, ~~shared memory~~)

 ---

## Technologies Used

- Language: **C (C11)**
- Build system: **CMake**
- Multithreading: **POSIX threads (pthread)**
- IPC mechanisms:
  - Sockets
  - ~~Pipes~~
  - ~~Shared memory with semaphores~~ 
- Synchronization:
  - Mutexes
  - Condition variables
  - Atomic variables

 ---

## Build and Run

### Build the Project

From the project root directory:

        mkdir build
        cd build
        cmake ..
        cmake --build . 

To build only a specific target:

        cmake --build . --target server
        cmake --build . --target client

### Run the Application

The server can be started from client. (recommended)  
Or you can start the server from console:  
0. This part is same for both types:  

        ./path/to/file load(0)/create(1) "sock" port ...

1. Create server from scratch. The server needs 15 arguments:  

                                 int  double double   double    double   int   int                                    int          int               
        ./server/server 1 "sock" port probUp probDown probRight probLeft width height woObstacles(0)/withObstacles(1) replications steps saveFileName oneClient(0)/multipleClients(1)

3. Create server by loading it from file. The server needs 7 arguments:  

        ./server/server 0 "sock" port loadFileName replications saveFileName oneClient(0)/multipleClients(1)
  
Run the client:  

        ./client/client

---

## Notes

The application is intended to run on Linux systems.

---

# Author

[Rizoto1](https://github.com/Rizoto1)
