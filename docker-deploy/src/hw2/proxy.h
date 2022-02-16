class Proxy {
public:
    int client_fd;
    int id;
    int server_fd;
    string client_ip;
    const char * hostname;
    const char * port;

    Proxy(int socket_id, int id, string& ip) : client_fd(socket_id), id(id), server_fd(0), client_ip(ip) {}

    string get_time() {
        time_t rawtime;
        struct tm* ptm;
        time(&rawtime);
        ptm = gmtime(&rawtime);
        return asctime(ptm);
    }

    void recv_all() {

    }

    int proxy_CONNECT() {
        // send OK to client
        ssize_t send_size = send(client_fd, OK, sizeof(OK), 0);
        if(send_size <= 0) return -1;
        string resp = OK;
        resp = resp.substr(0, resp.length()-4);
        logging_send_response(id, resp);

        fd_set rset;
        vector<int> fd_list = {server_fd, client_fd};
        int fdmax = max(server_fd, client_fd);
        struct timeval timeout;
        timeout.tv_sec = 36000;
        while(true) {
            FD_ZERO(&rset);
            vector<char> buf(RECV_BUF_LEN);

            for(int& fd: fd_list) {
                FD_SET(fd, &rset);
            }
            
            select(fdmax+1, &rset, NULL, NULL, &timeout);
            
            for(int& fd: fd_list) {
                if(FD_ISSET(fd, &rset)) {
                    ssize_t recv_size = recv(fd, &buf.data()[0], RECV_BUF_LEN, 0);
                    if(recv_size < 0) return -1;
                    else if(recv_size == 0) return 0;
                    string msg = buf.data();
                    if(fd == client_fd) {
                        send_size = send(server_fd, &buf.data()[0], recv_size, 0);
                        if(send_size <= 0) return -1;

                        logging_send_request(id, msg, hostname);
                    }
                    else {
                        send_size = send(client_fd, &buf.data()[0], recv_size, 0);
                        if(send_size <= 0) return -1;

                        logging_send_response(id, msg);
                    }
                }
            }
            buf.clear();
        }
        return 0;
    }

    int proxy_GET(HttpRequest& http_req) {

        // send request to server
        ssize_t send_size = send(server_fd, &http_req.raw_data.data()[0], http_req.size, 0);
        if(send_size <= 0) return -1;

        logging_send_request(id, http_req.firstline, hostname);

        // receive response from server
        vector<char> server_buf(RECV_BUF_LEN);
        ssize_t recv_size = recv(server_fd, &server_buf.data()[0], RECV_BUF_LEN, 0);
        if(recv_size <= 0) return -1;
        // TODO: HttpResponse http_resp(server_buf);

        string resp = server_buf.data();
        logging_receive_response(id, resp, hostname);

        recv_all();


        // send response to client
        send_size = send(client_fd, &server_buf.data()[0], BUF_LEN, 0);
        if(send_size <= 0) return -1;

        logging_send_response(id, resp);
        return 0;
    }

    int proxy_POST(HttpRequest& http_req) {
        
    }

    int proxy_OTHER() {
        ssize_t send_size = send(client_fd, BADREQUEST, sizeof(BADREQUEST), 0);
        if(send_size <= 0) return -1;
        string resp = BADREQUEST;
        resp = resp.substr(0, resp.length()-4);
        logging_send_response(id, resp);
        return -1;
    }

    ~Proxy() {
        close(server_fd);
        close(client_fd);
    }

};

void* proxyMain(void* paras) {
    threadPara_t* args = (threadPara_t*)paras;

    Proxy proxy(args->socket_fd, args->request_id, args->ip_from);


    // receive request from client
    vector<char> client_buf(BUF_LEN);
    ssize_t recv_size = recv(proxy.client_fd, &client_buf.data()[0], BUF_LEN, 0);
    if(recv_size <= 0) return NULL;

    string ip = proxy.client_ip, time = proxy.get_time();

    HttpRequest http_request(client_buf);
    http_request.size = recv_size;
    http_request.parse_request();

    logging_receive_request(proxy.id, http_request.firstline, ip, time);

    // connect to server
    proxy.hostname = http_request.header["Host"].c_str();
    proxy.port = http_request.header["port"].c_str();

    try {
        proxy.server_fd = init_client(proxy.hostname, proxy.port);
        cout << "hostname: " << proxy.hostname << " server_fd " << proxy.server_fd << endl;
    }
    catch(exception& e) {
        cerr << e.what() << endl;
        return NULL;
    }
    

    if(http_request.method == "CONNECT") { // CONNECT
        return NULL;
        if(proxy.proxy_CONNECT() == -1)
            return NULL;
    }
    else if(http_request.method == "GET") { // GET
        if(proxy.proxy_GET(http_request) == -1)
            return NULL;
    }
    else if(http_request.method == "POST"){ // POST
        return NULL;
        if(proxy.proxy_POST(http_request) == -1)
            return NULL;
    }
    else {
        if(proxy.proxy_OTHER() == -1)
            return NULL;
    }

    logging_close_tunnel(proxy.id);
    
    return NULL;
}