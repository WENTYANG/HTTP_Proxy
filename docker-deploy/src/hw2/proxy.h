#ifndef _PROXY_H_
#define _PROXY_H_

#include "parse.h"
#include <string>

typedef unsigned long ul;

class Proxy {
   public:
    int client_fd;
    int id;
    int server_fd;
    std::string client_ip;
    const char* hostname;
    const char* port;

    Proxy(int socket_fd, int id, std::string& ip);

    int connect_server(HttpRequest& http_request);

    void recv_and_send(HttpResponse& http_resp);

    int proxy_CONNECT();

    int proxy_GET(HttpRequest& http_req, bool revalidate);

    int proxy_POST(HttpRequest& http_req);

    int proxy_OTHER();

    int send_log_badrequest();

    int send_log_badgateway();

    ~Proxy();
};

void* proxyMain(void* paras);

#endif