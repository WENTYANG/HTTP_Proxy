#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>

#define PORT "12345"

typedef unsigned long ul;

using namespace std;

#include "helperFunction.h"
#include "parse.h"
#include "cache.h"
#include "proxy.h"

extern Cache & cache;

vector<pthread_t> pthreads;
vector<threadPara_t> threadParas;

static void create_daemon() {
    pid_t pid;

    // create child process
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    // dissociate from controlling tty(use setsid())
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    // point stdin/stdout/stderr at /dev/null(use dup2())
    int fd = open( "/dev/null", O_WRONLY);
    if(fd < 0 || dup2(fd, 0) < 0 || dup2(fd, 1) < 0 || dup2(fd, 2) < 0) {
        exit(EXIT_FAILURE);
    }

    chdir("/");

    umask(0);

    // use fork() again to make the process not a session leader
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);
}

int main() {
    create_daemon();

    int request_id = 0;
    int socket_fd;
    try {
        socket_fd = init_server(PORT);
    } catch (exception& e) {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }

    string ipFrom;

    while (true) {
        int client_connection_fd;
        try {
            client_connection_fd = accept_server(socket_fd, &ipFrom);
        } catch (exception& e) {
            cerr << e.what() << endl;
            continue;
        }
        pthread_t thread;
        threadPara_t paras;
        paras.socket_fd = client_connection_fd;
        paras.request_id = request_id;
        paras.ip_from = ipFrom;
        ++request_id;

        pthreads.push_back(thread);
        threadParas.push_back(paras);
        pthread_create(&pthreads.back(), NULL, proxyMain, &threadParas.back());
    }

    return EXIT_SUCCESS;
}