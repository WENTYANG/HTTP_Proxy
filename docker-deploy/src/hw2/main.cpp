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

#define PORT "12345"

typedef unsigned long ul;

using namespace std;

#include "helperFunction.h"
#include "parse.h"
#include "cache.h"
#include "proxy.h"

extern Cache & cache;

static void create_daemon() {
    pid_t pid;

    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    chdir("/");

    int x;
    for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
        close(x);
}

int main() {
    // create_daemon();

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
        // cout << "create thread with socket_fd=" << socket_fd << "with
        // request_id=" << request_id << endl;
        ++request_id;
        pthread_create(&thread, NULL, proxyMain, &paras);
    }

    return EXIT_SUCCESS;
}