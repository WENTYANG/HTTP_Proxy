

void* proxyMain(void* paras) {
    threadPara_t* args = (threadPara_t*)paras;
    int socket_fd = args->socket_fd;
    int request_id = args->request_id;
    cout << socket_fd << ' ' << request_id << endl;

    vector<char> buf(BUF_LEN);
    ssize_t recv_size = recv(socket_fd, &buf.data()[0], BUF_LEN, 0);
    if(recv_size <= 0) {
        return NULL;
    }

    string req = buf.data(), ip = "ip", time = "time";
    logging_receive_request(request_id, req, ip, time);
    return NULL;
}