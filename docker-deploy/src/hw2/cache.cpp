#include "parse.h"
#include "helperFunction.h"
#include "cache.h"

#include <iostream>
#include <cstring>

#define CACHE_SIZE 2048


using namespace std;

Cache::Cache() : size(CACHE_SIZE) {}

HttpResponse Cache::get_response(HttpRequest& req) {
    return cache_map[req.make_key()];
}

void Cache::handle_cache(int id, HttpRequest& req, HttpResponse& resp) {
    if(resp.status_code != "200") {
        logging_handle_cache(id, false, "status is not 200", -1);
        return;
    }
    if(resp.header_has("Cache-Control") && resp.header["cache-control"].find("no-store") != string::npos) {
        logging_handle_cache(id, false, "Cache control is no-store", -1);
        return;
    }
    else if(resp.header_has("Cache-Control") && resp.header["cache-control"].find("private") != string::npos) {
        logging_handle_cache(id, false, "Cache control is private", -1);
        return;
    }
    add_to_cache(req, resp);
    is_valid(id, req, true);
}

void Cache::add_to_cache(HttpRequest& req, HttpResponse resp) { // TODO: add ref to resp?
    string key = req.make_key();
    if(cache_map.count(key)) {
        cache_map[key] = resp;
        return;
    }
    if(cache_queue.size() == size) {   // pop first element in the queue
        auto x = cache_queue.front();
        cache_map.erase(cache_map.find(x));
        cache_queue.pop();
    }

    cache_queue.push(key);
    cache_map[key] = resp;
}

bool Cache::need_revalid(int id, HttpResponse& resp, bool handle=false) {
    if((resp.header_has("Cache-Control") && resp.header["cache-control"].find("no-cache") != string::npos) ||
       (resp.header_has("Pragma") && resp.header["pragma"].find("no-cache") != string::npos)) {
        if(handle) logging_handle_cache(id, true, "", -1);
        else logging_cache(id, true, false, -1);
        return true;
    }
    return false;
}

bool Cache::is_incache(int id, HttpRequest& req) {
    string key = req.make_key();

    if(cache_map.count(key) == 0) {
        logging_cache(id, false, false, -1);
        return false;
    }
    return true;
}

bool Cache::is_valid(int id, HttpRequest& req, bool handle=false) {
    string key = req.make_key();
    HttpResponse resp = cache_map[key];
    
    if(need_revalid(id, resp, handle)) return false;

    ul cur_age = 0, max_age = 0;
    time_t expire_time = 0, date_time = 0, last_modify_time = 0;
    if(resp.header_has("Age")) cur_age = stoul(resp.header["age"]);
    if(resp.header_has("Cache-Control")) {
        string cache_control = resp.header["cache-control"];
        max_age = get_max_age(cache_control);
        expire_time = resp.resp_time + max_age - cur_age;
    }
    else if(resp.header_has("Expires")) {
        expire_time = parse_time(resp.header["expires"]);
    }
    else if(resp.header_has("Last-Modified") && resp.header_has("Date")) {
        date_time = parse_time(resp.header["date"]);
        last_modify_time = parse_time(resp.header["last-modified"]);
        expire_time = resp.resp_time + (date_time - last_modify_time) / 10 - cur_age;
    }
    if(!handle && expire_time != 0 && difftime(time(NULL), expire_time) > 0) {
        logging_cache(id, true, false, expire_time);
        return false;
    }

    if(handle && expire_time != 0) {
        logging_handle_cache(id, true, "", expire_time);
    }

    if(!handle) logging_cache(id, true, true, -1);
    return true;
}

ul Cache::get_max_age(string& s) {
    size_t idx = 0;
    ul num = 0;
    if(s.find("s-maxage=") != string::npos) {
        idx = s.find("s-maxage=") + 9;
    }
    else if(s.find("max-age=") != string::npos) {
        idx = s.find("max-age=") + 8;
    }
    for(size_t i = idx; i < s.length(); ++i) {
        if(!isdigit(s[i])) break;
        num = num * 10 + (s[i] - '0');
    }
    return num;
}

time_t Cache::parse_time(string& s) {
    if(s.find("\r\n") != string::npos) {
        s = s.substr(0, s.find("\r\n"));
    }
    tm t;
    strptime(s.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &t);
    return mktime(&t);
}

void Cache::print_cache() {
    cout << "----------- cache -----------" << endl;
    if(cache_queue.size() == 0) {
        cout << "cache empty!" << endl;
    }
    else {
        cout << "cache size = " << cache_queue.size() << endl;
        for(auto& p: cache_map) {
            cout << "key = \"" << p.first << "\" | val = \"" << p.second.firstline << "...\"" << endl;
        }
    }
    cout << "-----------------------------" << endl;
}
