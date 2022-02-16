#include <unordered_map>

class HttpRequest {
   public:
    // First line: method url version
    string method;
    string url;
    string version;
    // Header
    unordered_map<string, string> header;
    string body;
    string firstline;
    // Raw data and size
    vector<char> raw_data;
    ssize_t size;

    HttpRequest(vector<char>& buf) : raw_data(buf) {}

    void parse_request() {
        int i = 0;
        int separate = 0;
        while (raw_data[i] != '\r') {  // Parse first line: method url version
            // separated by whitespace
            if (raw_data[i] == ' ') {
                string s(raw_data.begin() + separate, raw_data.begin() + i);
                // cout<<"Now at raw_data "<<i<<". The string is:"<<s<<endl;
                if (method == "") {
                    method = s;
                } else if (url == "") {
                    url = s;
                }
                separate = i + 1;
            }
            i++;
        }
        // Now raw_data[i]=='\r'
        version = string(raw_data.begin() + separate, raw_data.begin() + i);

        // Extract first line
        firstline = string(raw_data.begin(), raw_data.begin() + i);

        // Parse header
        while (size_t(i + 3) < raw_data.size() &&
               string(raw_data.begin() + i, raw_data.begin() + i + 4) !=
                   "\r\n\r\n") {
            // Header ends with \r\n\r\n
            i = i + 2;
            int key_start = i;
            while (raw_data[i] != ':') {
                i++;
            }
            string key =
                string(raw_data.begin() + key_start, raw_data.begin() + i);

            if (method == "CONNECT" && key == "Host") {
                int start = (++i) + 1;
                // skip the colon and whitespace
                while (raw_data[i] != ':') {
                    i++;
                }
                string hostname =
                    string(raw_data.begin() + start, raw_data.begin() + i);
                header[key] = hostname;
                // cout << "Key is " << key << ", value is " << hostname
                //      << " len: " << hostname.size() << endl;

                start = ++i;
                while (raw_data[i] != '\r') {
                    i++;
                }
                string port =
                    string(raw_data.begin() + start, raw_data.begin() + i);
                header["port"] = port;
                // cout << "Key is "
                //      << "port"
                //      << ", value is " << port << endl;
            } else {
                int value_start = i + 2;  // skip the whitespace
                while (raw_data[i] != '\r') {
                    i++;
                }
                string value = string(raw_data.begin() + value_start,
                                      raw_data.begin() + i);
                header[key] = value;
                // cout << "Key is " << key << ", value is " << value << endl;
            }
        }

        // Parse body
        if (method == "POST") {
            string req_body(raw_data.begin() + i + 2, raw_data.end());
            body = req_body;
            header["port"] = "80";
        } else if (method == "GET") {
            header["port"] = "80";
        }
    }
};

class HttpResponse {
   public:
    // First line: version status_code reason_phrase
    string version;
    string status_code;
    string reason_phrase;
    // Header
    bool is_chunked;
    ull content_len;
    unordered_map<string, string> header;
    string body;
    string firstline;
    // Raw data and size
    vector<char> raw_data;
    ssize_t size;
    HttpResponse(vector<char>& buf) : is_chunked(false), content_len(0), raw_data(buf)   {}

    void parse_response() {
        int i = 0;
        int separate = 0;
        while (raw_data[i] != '\r') {  //\r\n
            if (raw_data[i] == ' ') {
                string s(raw_data.begin() + separate, raw_data.begin() + i);
                // cout<<"Now at raw_data "<<i<<". The string is:"<<s<<endl;
                if (version == "") {
                    version = s;
                } else if (status_code == "") {
                    status_code = s;
                }
                separate = i + 1;
            }
            i++;
        }
        reason_phrase =
            string(raw_data.begin() + separate, raw_data.begin() + i);
        // Extract first line
        firstline = string(raw_data.begin(), raw_data.begin() + i);

        // Parse header
        while (size_t(i + 3) < raw_data.size() &&
               string(raw_data.begin() + i, raw_data.begin() + i + 4) !=
                   "\r\n\r\n") {
            // Header ends with \r\n\r\n
            i = i + 2;
            int key_start = i;
            while (raw_data[i] != ':') {
                i++;
            }
            string key =
                string(raw_data.begin() + key_start, raw_data.begin() + i);

            int value_start = i + 2;  // skip the whitespace
            while (raw_data[i] != '\r') {
                i++;
            }
            string value =
                string(raw_data.begin() + value_start, raw_data.begin() + i);
            header[key] = value;
            // cout << "Key is " << key << ", value is " << value << endl;
        }

        if (header.find("Transfer-Encoding") != header.end() &&
            header["Transfer-Encoding"] == "chunked") {
            is_chunked = true;
        } else {
            is_chunked = false;
            content_len = stoull(header["Content-Length"]);
        }
        // Parse content
        string res_body(raw_data.begin() + i + 2, raw_data.end());
        body = res_body;
    }
};