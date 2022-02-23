#ifndef _CACHE_H_
#define _CACHE_H_

#include <queue>
#include <unordered_map>
#include <string>
#include <cstring>

typedef unsigned long ul;

// Using FIFO policy
class Cache {
   private:
    std::unordered_map<std::string, HttpResponse> cache_map;
    std::queue<std::string> cache_queue;
    size_t size;

   public:
    Cache();

    static Cache& getInstance() {
        static Cache instance;
        return instance;
    }

    HttpResponse get_response(HttpRequest& req);

    void handle_cache(int id, HttpRequest& req, HttpResponse& resp);

    void add_to_cache(HttpRequest& req, HttpResponse resp);  // add ref to resp?

    bool need_revalid(int id, HttpResponse& resp, bool handle);

    bool is_incache(int id, HttpRequest& req);

    bool is_valid(int id, HttpRequest& req, bool handle);

    ul get_max_age(std::string& s);

    time_t parse_time(std::string& s);

    void print_cache();
};

#endif