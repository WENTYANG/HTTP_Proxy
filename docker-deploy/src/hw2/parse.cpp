#include "parse.h"

#include <iostream>

using namespace std;


HttpRequest::HttpRequest() {}

void HttpRequest::parse_request(vector<char>& raw_data, int size) {
    string data(raw_data.data(), size);

    // Separate into header and body
    size_t header_size = data.find("\r\n\r\n") + 4;
    head = data.substr(0, header_size-2);

    size_t newline = data.find('\r');
    firstline = data.substr(0, newline);

    // Parse first line: method url version
    size_t space = firstline.find(' ');
    method = firstline.substr(0, space);
    size_t space_sec = firstline.find(' ', space + 1);
    url = firstline.substr(space + 1, space_sec - space - 1);
    version = firstline.substr(space_sec + 1, firstline.size() - space_sec - 1);

    if (method == "POST") {
        body = data.substr(header_size);
    }

    // Parse key and value
    while (newline < header_size - 4) {
        size_t newline_sec = data.find('\r', newline + 1);

        string line = data.substr(newline + 2, newline_sec - newline - 2);

        size_t colon = line.find(':');

        string key = line.substr(0, colon);
        for(char& c: key) c = tolower(c);

        if (key == "host") {
            size_t colon_sec = line.find(':', colon + 1);
            string hostname;
            if (colon_sec < line.size()) {
                hostname = line.substr(colon + 2, colon_sec - colon - 2);
                string port = line.substr(colon_sec + 1, line.size() - colon_sec - 1);
                header["port"] = port;

            } else {
                hostname = line.substr(colon + 2, line.size() - colon - 2);
            }
            header[key] = hostname;
        } else {
            string value = line.substr(
                colon + 2,
                line.size() - colon - 2);  // skip whitespce after colon
            header[key] = value;
        }
        newline = newline_sec;
    }

    if ((method == "POST" || method == "GET") && header_has("port") == 0) {
        header["port"] = "80";
    } else if (method == "CONNECT" && header_has("port") == 0) {
        header["port"] = "443";
    }
}

string HttpRequest::make_key() {
    if (url.find("http://") != string::npos)
        return url;
    else if (url.find(header["host"]) != string::npos)
        return "http://" + url;
    return "http://" + header["host"] + url;
}

bool HttpRequest::header_has(string str) {
    for(char& c: str) c = tolower(c);
    return header.count(str);
}

string HttpRequest::get_whole_req() {
    return head + "\r\n" + body;
}


HttpResponse::HttpResponse() {}

HttpResponse::HttpResponse(vector<char>& buf)
    : is_chunked(false),
        content_len(0),
        raw_data(buf),
        resp_time(time(nullptr)) {}

void HttpResponse::parse_response() {
    string data(raw_data.data(), size);

    // Separate into header and body
    size_t header_size = data.find("\r\n\r\n") + 4;
    head = data.substr(0, header_size-2);

    size_t newline = data.find('\r');
    firstline = data.substr(0, newline);

    // Parse first line: method url version
    size_t space = firstline.find(' ');
    version = firstline.substr(0, space);
    size_t space_sec = firstline.find(' ', space + 1);
    status_code = firstline.substr(space + 1, space_sec - space - 1);
    reason_phrase = firstline.substr(space_sec + 1, firstline.size() - space_sec - 1);

    content_body = data.substr(header_size);

    // Parse key and value
    while (newline < header_size - 4) {
        size_t newline_sec = data.find('\r', newline + 1);

        string line = data.substr(newline + 2, newline_sec - newline - 2);

        size_t colon = line.find(':');

        string key = line.substr(0, colon);
        for(char& c: key) c = tolower(c);

        string value = line.substr(
            colon + 2,
            line.size() - colon - 2);  // skip whitespce after colon
        header[key] = value;
        newline = newline_sec;
    }

    // for (auto kv : header) {
    //     cout << "Key:" << kv.first << ", Value:" << kv.second << endl;
    // }

    if (header_has("transfer-encoding") > 0 && header["transfer-encoding"] == "chunked") {
        is_chunked = true;
    } else if (header_has("content-length") > 0){
        content_len = stoul(header["content-length"]);
    }
}

bool HttpResponse::header_has(string str) {
    for(char& c: str) c = tolower(c);
    return header.count(str);
}

string HttpResponse::get_whole_resp() {
    return head + "\r\n" + content_body;
}