#include <unordered_map>

class HttpRequest
{
public:
    // First line: method url version
    string method;
    string url;
    string version;
    // Header
    unordered_map<string, string> header;
    string body;
};

void parse_request(vector<char> &raw_data, HttpRequest *request)
{
    int i = 0;
    int separate = 0;
    while (raw_data[i] != '\n')
    { // Parse first line
        if (raw_data[i] == ' ')
        {
            string s(raw_data.begin() + separate, raw_data.begin() + i);
            // cout<<"Now at raw_data "<<i<<". The string is:"<<s<<endl;
            if (request->method == "")
            {
                request->method = s;
            }
            else if (request->url == "")
            {
                request->url = s;
            }
            separate = i + 1;
        }
        i++;
    }
    // Now raw_data[i]=='\n'
    request->version = string(raw_data.begin() + separate, raw_data.begin() + i);

    // Parse header
    while (i++ < raw_data.size() && raw_data[i] != '\r')
    { // Header ends with CR
        int key_start = i;
        while (raw_data[i] != ':')
        {
            i++;
        }
        string key = string(raw_data.begin() + key_start, raw_data.begin() + i);

        if (request->method == "CONNECT" && key == "Host")
        {
            int start = (++i) + 1; // skip the whitespace
            while (raw_data[i] != ':')
            {
                i++;
            }
            string hostname = string(raw_data.begin() + start, raw_data.begin() + i);
            request->header[key] = hostname;
            // cout << "Key is " << key << ", value is " << hostname << endl;

            start = ++i;
            while (raw_data[i] != '\n')
            {
                i++;
            }
            string port = string(raw_data.begin() + start, raw_data.begin() + i - 1);
            request->header["port"] = port;
            // cout << "Key is "
            //      << "port"
            //      << ", value is " << port << endl;
        }
        else
        {
            int value_start = i + 2; // skip the whitespace
            while (raw_data[i] != '\n')
            {
                i++;
            }
            string value = string(raw_data.begin() + value_start, raw_data.begin() + i);
            request->header[key] = value;
            // cout << "Key is " << key << ", value is " << value << endl;
        }
    }

    // Parse body
    if (request->method == "POST")
    {
        string body(raw_data.begin() + i + 1, raw_data.end());
        request->body = body;
    }
}