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
        while (
            raw_data[i] !=
            '\r') {  // Parse first line: method url version separated by whitespace
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
        while (i++ < raw_data.size() &&
            raw_data[i] != '\r') {  // Header ends with CR
            int key_start = i;
            while (raw_data[i] != ':') {
                i++;
            }
            string key = string(raw_data.begin() + key_start, raw_data.begin() + i);

            if (method == "CONNECT" && key == "Host") {
                int start = (++i) + 1;  // skip the whitespace
                while (raw_data[i] != ':') {
                    i++;
                }
                string hostname =
                    string(raw_data.begin() + start, raw_data.begin() + i);
                header[key] = hostname;
                // cout << "Key is " << key << ", value is " << hostname << endl;

                start = ++i;
                while (raw_data[i] != '\n') {
                    i++;
                }
                string port =
                    string(raw_data.begin() + start, raw_data.begin() + i - 1);
                header["port"] = port;
                // cout << "Key is "
                //      << "port"
                //      << ", value is " << port << endl;
            } else {
                int value_start = i + 2;  // skip the whitespace
                while (raw_data[i] != '\n') {
                    i++;
                }
                string value =
                    string(raw_data.begin() + value_start, raw_data.begin() + i);
                header[key] = value;
                // cout << "Key is " << key << ", value is " << value << endl;
            }
        }

        // Parse body
        if (method == "POST") {
            string req_body(raw_data.begin() + i + 1, raw_data.end());
            body = req_body;
        }
    }
};


class HttpResponse {
public:
    // First line

    // Header
    unordered_map<string, string> header;
    string body;
    string firstline;
    // Raw data and size
    vector<char> raw_data;
    ssize_t size;

    

    void parse_response() {

    }

};