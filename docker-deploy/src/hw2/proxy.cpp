#include "proxy.h"
#include "cache.h"
#include "helperFunction.h"

#include <iostream>
#include <vector>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define BUF_LEN 4096
#define RECV_BUF_LEN 65536

#define OK "HTTP/1.1 200 OK\r\n\r\n"
#define BADREQUEST "HTTP/1.1 400 Bad Request\r\n\r\n"
#define BADGATEWAY "HTTP/1.1 502 Bad Gateway\r\n\r\n"
#define NOTIMPLEMENTED "HTTP/1.1 501 Not Implemented\r\n\r\n"


using namespace std;

Cache & cache = Cache::getInstance();

Proxy::Proxy(int socket_fd, int id, string& ip)
    : client_fd(socket_fd), id(id), server_fd(-1), client_ip(ip) {}

int Proxy::connect_server(HttpRequest& http_request) {
    hostname = http_request.header["host"].c_str();
    port = http_request.header["port"].c_str();

    // hostname = "vcm-24073.vm.duke.edu";
    // port = "15213";

    try {
        // cout << "***Test: hostname: " << hostname
        //     << " port: " << port;
        server_fd = init_client(hostname, port, id);
        // cout << " server_fd " << server_fd << "***" << endl;
    } catch (exception& e) {
        cerr << "Connect to server error: " << e.what() << endl;
        send_log_badrequest();
        return -1;
    }
    return 0;
}

void Proxy::recv_and_send(HttpResponse& http_resp) {
    vector<char> buf(RECV_BUF_LEN);
    ul recv_size;
    if (!http_resp.is_chunked && http_resp.content_len > 0) {
        ul tot_len = http_resp.content_len,
            cur_len = http_resp.size - http_resp.head.size() - 2;
        // cout << "cur len: " << cur_len << " tot len: " << tot_len << endl;
        while (cur_len < tot_len) {
            recv_size = recv(server_fd, &buf.data()[0], RECV_BUF_LEN, 0);
            if (recv_size == 0)
                break;
            if (send(client_fd, &buf.data()[0], recv_size, 0) == -1) {
                throw MyException("Send content data to client error");
            }
            cur_len += recv_size;
            http_resp.content_body += string(buf.data(), recv_size);
        }
    } else if (http_resp.is_chunked) {
        while (http_resp.content_body.find("0\r\n\r\n") == string::npos) {
            recv_size = recv(server_fd, &buf.data()[0], RECV_BUF_LEN, 0);
            if (send(client_fd, &buf.data()[0], recv_size, 0) == -1) {
                throw MyException("Send chunked data to client error");
            }
            http_resp.content_body += string(buf.data(), recv_size);
        }
    }
}

int Proxy::proxy_CONNECT() {
    // send OK to client
    ssize_t send_size = send(client_fd, OK, sizeof(OK), 0);
    if (send_size <= 0)
        return -1;
    string resp = OK;
    resp = resp.substr(0, resp.length() - 4);
    logging_send_response(id, resp);

    fd_set rset;
    vector<int> fd_list = {server_fd, client_fd};
    int fdmax = max(server_fd, client_fd);
    struct timeval timeout;
    timeout.tv_sec = 36000;
    while (true) {
        FD_ZERO(&rset);
        vector<char> buf(RECV_BUF_LEN);

        for (int& fd : fd_list) {
            FD_SET(fd, &rset);
        }

        select(fdmax + 1, &rset, NULL, NULL, &timeout);

        for (int& fd : fd_list) {
            if (FD_ISSET(fd, &rset)) {
                ssize_t recv_size = recv(fd, &buf.data()[0], RECV_BUF_LEN, 0);
                if (recv_size < 0)
                    return -1;
                else if (recv_size == 0)
                    return 0;
                string msg = buf.data();
                if (fd == client_fd) {
                    send_size = send(server_fd, &buf.data()[0], recv_size, 0);
                    if (send_size <= 0)
                        return -1;
                } else {
                    send_size = send(client_fd, &buf.data()[0], recv_size, 0);
                    if (send_size <= 0)
                        return -1;
                }
            }
        }
    }
    return 0;
}

int Proxy::proxy_GET(HttpRequest& http_req, bool revalidate=false) {
    // send request to server
    string req_str = http_req.get_whole_req();
    ssize_t send_size = send(server_fd, &req_str.c_str()[0], req_str.size(), 0);
    if (send_size <= 0) return -1;

    logging_send_request(id, http_req.firstline, hostname);

    // receive first response (whole header) from server
    vector<char> server_buf(RECV_BUF_LEN), tmp_buf(RECV_BUF_LEN);
    ssize_t tot_recv_size = 0;
    string tmp;
    while (tmp.find("\r\n\r\n") == string::npos) {
        ssize_t recv_size = recv(server_fd, &tmp_buf.data()[0], RECV_BUF_LEN, 0);
        if (recv_size < 0) return -1;
        tmp += string(tmp_buf.data(), recv_size);
        for (int i = 0; i < recv_size; ++i) {
            server_buf[tot_recv_size + i] = tmp_buf[i];
        }
        tot_recv_size += recv_size;
    }

    HttpResponse http_resp(server_buf);
    http_resp.size = tot_recv_size;

    try {
        http_resp.parse_response();
    } catch (exception& e) {
        cerr << "Parse response error: " << e.what() << endl;
        send_log_badgateway();
        return -1;
    }

    logging_receive_response(id, http_resp.firstline, hostname);

    if(revalidate && http_resp.status_code == "304") {
        http_resp = cache.get_response(http_req);
        string resp_str = http_resp.get_whole_resp();
        send_size = send(client_fd, &resp_str.c_str()[0], resp_str.size(), 0);
        if (send_size <= 0) return -1;
        
        logging_send_response(id, http_resp.firstline);
        return 0;
    }

    // send first response to client
    send_size = send(client_fd, &http_resp.raw_data.data()[0], http_resp.size, 0);
    if (send_size <= 0) return -1;

    // recv rest response from server and sent to client
    try {
        recv_and_send(http_resp);
    } catch (exception& e) {
        cerr << "Transfer data error: " << e.what() << endl;
        send_log_badgateway();
        return -1;
    }
    logging_send_response(id, http_resp.firstline);
    
    cache.handle_cache(id, http_req, http_resp);

    return 0;
}

int Proxy::proxy_POST(HttpRequest& http_req) {
    // send request to server
    string req_str = http_req.get_whole_req();
    ssize_t send_size = send(server_fd, &req_str.c_str()[0], req_str.size(), 0);
    if (send_size <= 0) return -1;

    logging_send_request(id, http_req.firstline, hostname);

    // receive first response from server
    vector<char> server_buf(RECV_BUF_LEN);
    ssize_t tot_recv_size = 0;
    vector<char> tmp_buf(RECV_BUF_LEN);
    string tmp;
    while (tmp.find("\r\n\r\n") == string::npos) {
        ssize_t recv_size =
            recv(server_fd, &tmp_buf.data()[0], RECV_BUF_LEN, 0);
        if (recv_size < 0)
            return -1;
        tmp += string(tmp_buf.data(), recv_size);
        for (int i = 0; i < recv_size; ++i) {
            server_buf[tot_recv_size + i] = tmp_buf[i];
        }
        tot_recv_size += recv_size;
    }

    HttpResponse http_resp(server_buf);
    http_resp.size = tot_recv_size;

    try {
        http_resp.parse_response();
    } catch (exception& e) {
        cerr << "Parse response error: " << e.what() << endl;
        send_log_badgateway();
        return -1;
    }

    logging_receive_response(id, http_resp.firstline, hostname);

    // send first response to client
    send_size =
        send(client_fd, &http_resp.raw_data.data()[0], http_resp.size, 0);
    if (send_size <= 0)
        return -1;
    try {
        recv_and_send(http_resp);
    } catch (exception& e) {
        cerr << "Transfer data error: " << e.what() << endl;
        send_log_badgateway();
        return -1;
    }

    logging_send_response(id, http_resp.firstline);
    return 0;
}

int Proxy::proxy_OTHER() {
    return send_log_notimplemented();
}

int Proxy::send_log_badrequest() {
    ssize_t send_size = send(client_fd, BADREQUEST, sizeof(BADREQUEST), 0);
    if (send_size <= 0)
        return -1;
    string resp = BADREQUEST;
    resp = resp.substr(0, resp.length() - 4);
    logging_send_response(id, resp);
    return 0;
}

int Proxy::send_log_badgateway() {
    ssize_t send_size = send(client_fd, BADGATEWAY, sizeof(BADGATEWAY), 0);
    if (send_size <= 0)
        return -1;
    string resp = BADGATEWAY;
    resp = resp.substr(0, resp.length() - 4);
    logging_send_response(id, resp);
    return 0;
}

int Proxy::send_log_notimplemented() {
    ssize_t send_size = send(client_fd, NOTIMPLEMENTED, sizeof(NOTIMPLEMENTED), 0);
    if (send_size <= 0)
        return -1;
    string resp = NOTIMPLEMENTED;
    resp = resp.substr(0, resp.length() - 4);
    logging_send_response(id, resp);
    return 0;
}

Proxy::~Proxy() {
    if(server_fd != -1) close(server_fd);
    close(client_fd);
}

void* proxyMain(void* paras) {
    threadPara_t* args = (threadPara_t*)paras;

    Proxy proxy(args->socket_fd, args->request_id, args->ip_from);

    // receive request from client
    vector<char> client_buf(BUF_LEN);
    ssize_t recv_size =
        recv(proxy.client_fd, &client_buf.data()[0], BUF_LEN, 0);
    if (recv_size <= 0)
        return NULL;

    string ip = proxy.client_ip;

    HttpRequest http_request;
    try {
        http_request.parse_request(client_buf, recv_size);
    } catch (exception& e) {
        cerr << "Parse request error: " << e.what() << endl;
        proxy.send_log_badrequest();
        return NULL;
    }

    logging_receive_request(proxy.id, http_request.firstline, ip);

    // check if GET and in cache
    if(http_request.method == "GET") {
        cache.print_cache();
        if(cache.is_incache(proxy.id, http_request)) {
            HttpResponse cache_resp = cache.get_response(http_request);
            if(!cache.is_valid(proxy.id, http_request, false)) { // need revalidate
                cout << cache_resp.head << endl;
                string cate = "NOTE", msg;
                if(cache_resp.header_has("ETag")) {
                    http_request.header["if-none-match"] = cache_resp.header["etag"];
                    msg = "If-None-Match: " + cache_resp.header["etag"];
                    http_request.head.append(msg + "\r\n");
                    logging_msg(proxy.id, cate, msg);
                }
                if(cache_resp.header_has("Last-Modified")) {
                    http_request.header["if-modified-since"] = cache_resp.header["last-modified"];
                    msg = "If-Modified-Since: " + cache_resp.header["last-modified"];
                    http_request.head.append(msg + "\r\n");
                    logging_msg(proxy.id, cate, msg);
                }
                if(proxy.connect_server(http_request) == -1) return NULL;
                if(proxy.proxy_GET(http_request, true) == -1) return NULL;
                logging_close_tunnel(proxy.id);
            }
            else {
                string resp_str = cache_resp.get_whole_resp();
                ssize_t send_size = send(proxy.client_fd, &resp_str.c_str()[0], resp_str.size(), 0);
                if(send_size <= 0) return NULL;
                logging_send_response(proxy.id, cache_resp.firstline);
            }
            return NULL;
        }
    }

    // connect to server
    if(proxy.connect_server(http_request) == -1) {
        return NULL;
    }

    if (http_request.method == "CONNECT") {  // CONNECT
        // return NULL;
        if (proxy.proxy_CONNECT() == -1)
            return NULL;
    } else if (http_request.method == "GET") {  // GET
        // return NULL;
        if (proxy.proxy_GET(http_request) == -1)
            return NULL;
    } else if (http_request.method == "POST") {  // POST
        // return NULL;
        if (proxy.proxy_POST(http_request) == -1)
            return NULL;
    } else {
        if (proxy.proxy_OTHER() == -1)
            return NULL;
    }

    logging_close_tunnel(proxy.id);
    // cout << "end!!" << endl;
    return NULL;
}