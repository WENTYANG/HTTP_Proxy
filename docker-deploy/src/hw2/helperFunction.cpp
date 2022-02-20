#include <fstream>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <time.h>

#include "helperFunction.h"

#define LOG_PATH "proxy.log"

using namespace std;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static ofstream log_ofs(LOG_PATH);

/* ------------------------ exception helper ------------------------ */
MyException::MyException(string msg) : msg(msg) {}

const char* MyException::what() const throw() { return msg.c_str(); }

/* ------------------------ logger helper ------------------------ */
string get_time(bool now, time_t rawtime=0) {
    struct tm* ptm;
    if(now) time(&rawtime); // current time
    ptm = gmtime(&rawtime);
    return asctime(ptm);
}

void logging_receive_request(int id, string& req, string& ip) {
    pthread_mutex_lock(&lock);
    log_ofs << id << ": \"" << req << "\" from " << ip << " @ " << get_time(true);
    pthread_mutex_unlock(&lock);
}

void logging_send_request(int id, string& req, const char* hostname) {
    pthread_mutex_lock(&lock);
    log_ofs << id << ": Requesting \"" << req << "\" from " << hostname << endl;
    pthread_mutex_unlock(&lock);
}

void logging_receive_response(int id, string& resp, const char* hostname) {
    pthread_mutex_lock(&lock);
    log_ofs << id << ": Received \"" << resp << "\" from " << hostname << endl;
    pthread_mutex_unlock(&lock);
}

void logging_send_response(int id, string& resp) {
    pthread_mutex_lock(&lock);
    log_ofs << id << ": Responding \"" << resp << "\"" << endl;
    pthread_mutex_unlock(&lock);
}

void logging_msg(int id, string& cate, string& msg) {
    pthread_mutex_lock(&lock);
    if (id < 0)
        log_ofs << "(no-id): ";
    else
        log_ofs << id << ": ";
    log_ofs << cate << " " << msg << endl;
    pthread_mutex_unlock(&lock);
}

void logging_close_tunnel(int id) {
    pthread_mutex_lock(&lock);
    log_ofs << id << ": Tunnel closed" << endl;
    pthread_mutex_unlock(&lock);
}

void logging_cache(int id, bool is_incache, bool is_valid, time_t expired_time=-1) {
    pthread_mutex_lock(&lock);
    if(!is_incache) {
        log_ofs << id << ": not in cache" << endl;
    }
    else if(is_valid) {
        log_ofs << id << ": in cache, valid" << endl;
    }
    else if(expired_time != -1) {
        log_ofs << id << ": in cache, but expired at " << get_time(false,expired_time) << endl;
    }
    else {
        log_ofs << id << ": in cache, requires validation" << endl;
    }
    pthread_mutex_unlock(&lock);
}

void logging_handle_cache(int id, bool is_cacheable, string reason="", time_t expires_time=-1) {
    pthread_mutex_lock(&lock);
    if(!is_cacheable) {
        log_ofs << id << ": not cacheable bacause " << reason << endl;
    }
    else if(expires_time != -1) {
        log_ofs << id << ": cached, expires at " << get_time(false, expires_time) << endl;
    }
    else {
        log_ofs << id << ": cached, but requires re-validation" << endl;
    }
    pthread_mutex_unlock(&lock);
}

/* ------------------------ socket helper ------------------------ */
int init_server(const char* port) {
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo* host_info_list;
    const char* hostname = NULL;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        string msg = "cannot get address info for host";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        throw MyException("Socket expection");
    }

    if (strlen(port) == 0) {
        struct sockaddr_in* addr_in =
            (struct sockaddr_in*)(host_info_list->ai_addr);
        addr_in->sin_port = 0;
    }

    socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (socket_fd == -1) {
        string msg = "cannot create socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        throw MyException("Socket expection");
    }

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status =
        bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        string msg = "cannot bind socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        throw MyException("Socket expection");
    }

    status = listen(socket_fd, 100);
    if (status == -1) {
        string msg = "cannot listen on socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        throw MyException("Socket expection");
    }

    freeaddrinfo(host_info_list);

    return socket_fd;
}

int accept_server(int socket_fd, string* ipFrom) {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd =
        accept(socket_fd, (struct sockaddr*)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
        string msg = "cannot accept connection on socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        throw MyException("Socket expection");
    }
    struct sockaddr_in* addr_in = (struct sockaddr_in*)&socket_addr;
    *ipFrom = inet_ntoa(addr_in->sin_addr);
    return client_connection_fd;
}

int init_client(const char* hostname, const char* port) {
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo* host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        string msg = "cannot get address info for host";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        throw MyException("Socket expection");
    }

    socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (socket_fd == -1) {
        string msg = "cannot create socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        throw MyException("Socket expection");
    }

    status =
        connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        string msg = "cannot connect to socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        throw MyException("Socket expection");
    }

    freeaddrinfo(host_info_list);
    // cout << "Build connection with " << hostname << ':' << port << "
    // socket_fd=" << socket_fd << endl;
    return socket_fd;
}