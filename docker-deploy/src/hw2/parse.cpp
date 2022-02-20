#include "parse.h"

#include <iostream>

using namespace std;


HttpRequest::HttpRequest(vector<char>& buf) : raw_data(buf) {}

void HttpRequest::parse_request() {
    string data(raw_data.data(), size);

    // Separate into header and body
    ssize_t header_size = data.find("\r\n\r\n") + 4;
    head = data.substr(0, header_size-2);

    ssize_t newline = data.find('\r');
    firstline = data.substr(0, newline);

    // Parse first line: method url version
    ssize_t space = firstline.find(' ');
    method = firstline.substr(0, space);
    ssize_t space_sec = firstline.find(' ', space + 1);
    url = firstline.substr(space + 1, space_sec - space - 1);
    version = firstline.substr(space_sec + 1, firstline.size() - space_sec - 1);

    if (method == "POST") {
        body = data.substr(header_size);
    }

    // cout << "method: " << method << endl;
    // cout << "url: " << url << endl;
    // cout << "version: " << version << endl;

    // Parse key and value
    while (newline < header_size - 4) {
        ssize_t newline_sec = data.find('\r', newline + 1);
        // cout << "newline: " << newline << endl;
        // cout << "newline_sec: " << newline_sec << endl;

        string line = data.substr(newline + 2, newline_sec - newline - 2);
        // cout << "line:" << line << endl;

        ssize_t colon = line.find(':');

        string key = line.substr(0, colon);

        if (key == "Host") {
            ssize_t colon_sec = line.find(':', colon + 1);
            string hostname;
            if (colon_sec != string::npos) {
                hostname = line.substr(colon + 2, colon_sec - colon - 2);
                string port = line.substr(colon_sec + 1, line.size() - colon_sec - 1);
                header["port"] = port;

            } else {
                hostname = line.substr(colon + 2, line.size() - colon - 1);
            }
            header[key] = hostname;
        } else {
            string value = line.substr(
                colon + 2,
                line.size() - colon - 1);  // skip whitespce after colon
            header[key] = value;
        }
        newline = newline_sec;
    }

    // for (auto kv : header) {
    //     cout << "Key:" << kv.first << ", Value:" << kv.second << ", value
    //     size: " << kv.second.size() << endl;
    // }

    if (method == "POST") {
        header["port"] = "80";
    } else if (method == "GET") {
        header["port"] = "80";
    } else if (method == "CONNECT" && header.find("port") == header.end()) {
        header["port"] = "443";
    }
}

string HttpRequest::make_key() {
    if (url.find("http://") != string::npos)
        return url;
    else if (url.find(header["Hostname"]) != string::npos)
        return "http://" + url;
    return "http://" + header["Hostname"] + url;
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
    ssize_t header_size = data.find("\r\n\r\n") + 4;
    head = data.substr(0, header_size-2);

    ssize_t newline = data.find('\r');
    string firstline = data.substr(0, newline);

    // Parse first line: method url version
    ssize_t space = firstline.find(' ');
    version = firstline.substr(0, space);
    ssize_t space_sec = firstline.find(' ', space + 1);
    status_code = firstline.substr(space + 1, space_sec - space - 1);
    reason_phrase =
        firstline.substr(space_sec + 1, firstline.size() - space_sec - 1);

    content_body = data.substr(header_size);

    // cout << "version: " << version << endl;
    // cout << "status_code: " << status_code << endl;
    // cout << "reason_phrase: " << reason_phrase << endl;

    // Parse key and value
    while (newline < header_size - 4) {
        ssize_t newline_sec = data.find('\r', newline + 1);

        string line = data.substr(newline + 2, newline_sec - newline - 2);

        ssize_t colon = line.find(':');

        string key = line.substr(0, colon);

        string value = line.substr(
            colon + 2,
            line.size() - colon - 1);  // skip whitespce after colon
        header[key] = value;
        newline = newline_sec;
    }

    // for (auto kv : header) {
    //     cout << "Key:" << kv.first << ", Value:" << kv.second << endl;
    // }

    if (header.count("Transfer-Encoding") > 0 &&
        header["Transfer-Encoding"] == "chunked") {
        is_chunked = true;
    } else if (header.count("Content-Length") > 0){
        content_len = stoul(header["Content-Length"]);
    }
}

string HttpResponse::get_whole_resp() {
    return head + "\r\n" + content_body;
}