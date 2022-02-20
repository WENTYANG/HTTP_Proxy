#ifndef _HELPERFUNCTION_H_
#define _HELPERFUNCTION_H_

#include <exception>
#include <string>
#include <cstring>


/* ------------------------ exception helper ------------------------ */
class MyException : public std::exception {
   private:
    std::string msg;

   public:
    MyException(std::string msg);

    const char* what() const throw();
};

/* ------------------------ logger helper ------------------------ */

std::string get_time(bool now, time_t rawtime);

void logging_receive_request(int id, std::string& req, std::string& ip);

void logging_send_request(int id, std::string& req, const char* hostname);

void logging_receive_response(int id, std::string& resp, const char* hostname);

void logging_send_response(int id, std::string& resp);

void logging_msg(int id, std::string& cate, std::string& msg);

void logging_close_tunnel(int id);

void logging_cache(int id, bool is_incache, bool is_valid, time_t expired_time);

void logging_handle_cache(int id, bool is_cacheable, std::string reason, time_t expires_time);

/* ------------------------ socket helper ------------------------ */
int init_server(const char* port);

int accept_server(int socket_fd, std::string* ipFrom);

int init_client(const char* hostname, const char* port);

/* ------------------------ multi-thread helper ------------------------ */
typedef struct ThreadPara {
    int socket_fd;
    int request_id;
    std::string ip_from;
} threadPara_t;

#endif