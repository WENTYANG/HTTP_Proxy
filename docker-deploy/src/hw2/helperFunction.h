
/* ------------------------ logger helper ------------------------ */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static ofstream log_ofs(LOG_PATH);

void logging_receive_request(int id, string& req, string& ip, string& time) {
    pthread_mutex_lock(&lock);
    log_ofs << id << ": \"" << req << "\" from " << ip << " @ " << time << endl;
    pthread_mutex_unlock(&lock);
}

void logging_msg(int id, string& cate, string& msg) {
    pthread_mutex_lock(&lock);
    if(id < 0) log_ofs << "(no-id): ";
    else log_ofs << id << ": ";
    log_ofs << cate << " " << msg << endl;
    pthread_mutex_unlock(&lock);
}


/* ------------------------ socket helper ------------------------ */
int init_server(const char *port) {
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        string msg = "cannot get address info for host";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        exit(EXIT_FAILURE);
    }

    if(strlen(port) == 0) {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
        addr_in->sin_port = 0;
    }

    socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (socket_fd == -1) {
        string msg = "cannot create socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        exit(EXIT_FAILURE);
    }

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        string msg = "cannot bind socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        exit(EXIT_FAILURE);
    }

    status = listen(socket_fd, 100);
    if (status == -1) {
        string msg = "cannot listen on socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(host_info_list);

    return socket_fd;
}

int accept_server(int socket_fd) {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
        string msg = "cannot accept connection on socket";
        string cate = "ERROR";
        logging_msg(-1, cate, msg);
        exit(EXIT_FAILURE);
    }
    return client_connection_fd;
}


typedef struct ThreadPara {
    int socket_fd;
    int request_id;
} threadPara_t;

