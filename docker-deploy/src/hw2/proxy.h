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
        timeout.tv_sec = 5;
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
                    // cout << "recv size: " << recv_size << " msg len: " << msg.length() << " fd " << fd << endl;
                    // cout << "msg: " << msg << endl;
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

    int proxy_GET(vector<char>& client_buf) {
        string req = client_buf.data();

        // send request to server
        ssize_t send_size = send(server_fd, &client_buf.data()[0], BUF_LEN, 0);
        if(send_size <= 0) return -1;

        logging_send_request(id, req, hostname);


        // receive response from server
        vector<char> server_buf(BUF_LEN);
        ssize_t recv_size = recv(server_fd, &server_buf.data()[0], BUF_LEN, 0);
        if(recv_size <= 0) return -1;
        // TODO: HttpResponse http_resp(resp);

        string resp = server_buf.data();
        logging_receive_response(id, resp, hostname);


        // send response to client
        send_size = send(client_fd, &server_buf.data()[0], BUF_LEN, 0);
        if(send_size <= 0) return -1;

        logging_send_response(id, resp);
        return 0;
    }

    int proxy_POST(vector<char>& client_buf) {
        string req = client_buf.data();

        // send request to server
        ssize_t send_size = send(server_fd, &client_buf.data()[0], BUF_LEN, 0);
        if(send_size <= 0) return -1;

        logging_send_request(id, req, hostname);


        // receive response from server
        vector<char> server_buf(BUF_LEN);
        ssize_t recv_size = recv(server_fd, &server_buf.data()[0], BUF_LEN, 0);
        if(recv_size <= 0) return -1;
        // TODO: HttpResponse http_resp(resp);

        string resp = server_buf.data();
        logging_receive_response(id, resp, hostname);


        // send response to client
        send_size = send(client_fd, &server_buf.data()[0], BUF_LEN, 0);
        if(send_size <= 0) return -1;

        logging_send_response(id, resp);
        return 0;
    }

    int proxy_OTHER() {
        ssize_t send_size = send(client_fd, BADREQUEST, sizeof(BADREQUEST), 0);
        if(send_size <= 0) return -1;
        string resp = BADREQUEST;
        resp = resp.substr(0, resp.length()-4);
        logging_send_response(id, resp);
        return 0;
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

    string req = client_buf.data(), ip = proxy.client_ip, time = proxy.get_time();
    logging_receive_request(proxy.id, req, ip, time);

    HttpRequest* request = new HttpRequest();
    parse_request(client_buf, request);

    // connect to server
    proxy.hostname = request->header["Host"].c_str();
    proxy.port = request->header["port"].c_str();

    try {
        proxy.server_fd = init_client(proxy.hostname, proxy.port);
        cout << "hostname: " << proxy.hostname << " server_fd " << proxy.server_fd << endl;
    }
    catch(exception& e) {
        cerr << e.what() << endl;
        return NULL;
    }
    

    if(request->method == "CONNECT") { // CONNECT
        if(proxy.proxy_CONNECT() == -1)
            return NULL;
    }
    else if(request->method == "GET") { // GET
        return NULL;
        if(proxy.proxy_GET(client_buf) == -1)
            return NULL;
    }
    else if(request->method == "POST"){ // POST
        return NULL;
        if(proxy.proxy_POST(client_buf) == -1)
            return NULL;
    }
    else {
        if(proxy.proxy_OTHER() == -1)
            return NULL;
    }
    
    return NULL;
}