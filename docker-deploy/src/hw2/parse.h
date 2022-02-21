#ifndef _PARSE_H_
#define _PARSE_H_

#include <vector>
#include <cstring>
#include <string>
#include <unordered_map>

typedef unsigned long ul;

class HttpRequest {
   public:
    // First line: method url version
    std::string method;
    std::string url;
    std::string version;
    // Header
    std::unordered_map<std::string, std::string> header;
    std::string head;  // end with \r\n, include firstline
    std::string body;
    std::string firstline;

    HttpRequest();

    void parse_request(std::vector<char>& buf, int size);

    std::string make_key();

    std::string get_whole_req();
};

class HttpResponse {
   public:
    // First line: version status_code reason_phrase
    std::string version;
    std::string status_code;
    std::string reason_phrase;
    // Header
    bool is_chunked;
    ul content_len;
    std::unordered_map<std::string, std::string> header;
    std::string firstline;
    std::string head; // end with \r\n
    // First received raw data and size
    std::vector<char> raw_data;
    ssize_t size;
    // content body
    std::string content_body;
    // response time
    time_t resp_time;

    HttpResponse();
    HttpResponse(std::vector<char>& buf);

    void parse_response();

    std::string get_whole_resp();
};

#endif