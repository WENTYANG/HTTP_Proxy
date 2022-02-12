#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <pthread.h>
#include <exception>
#include <cstring>
#include <vector>
#include <fstream>

#define PORT "12345"
#define LOG_PATH "proxy.log"
#define BUF_LEN 4096

using namespace std;

#include "helperFunction.h"
#include "proxy.h"


static void create_daemon() {
    pid_t pid;

    pid = fork();

    if(pid < 0) exit(EXIT_FAILURE);
    if(pid > 0) exit(EXIT_SUCCESS);
    if(setsid() < 0) exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    pid = fork();
    
    if(pid < 0) exit(EXIT_FAILURE);
    if(pid > 0) exit(EXIT_SUCCESS);
    
    umask(0);
    
    chdir("/");
    
    int x;
    for(x = sysconf(_SC_OPEN_MAX); x>=0; x--) close(x);
}

int main() {
    // create_daemon();

    int request_id = 0;

    int socket_fd = init_server(PORT);

    while(true) {
        int client_connection_fd = accept_server(socket_fd);

        pthread_t thread;
        pthread_mutex_lock(&lock);
        threadPara_t paras;
        paras.socket_fd = client_connection_fd;
        paras.request_id = request_id;
        // cout << "create thread with socket_fd=" << socket_fd << "with request_id=" << request_id << endl;
        cout << thread << ' ' << request_id << endl;
        ++request_id;
        pthread_mutex_unlock(&lock);
        pthread_create(&thread, NULL, proxyMain, &paras);
    }

    return EXIT_SUCCESS;
}