#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

class StaticContentHandler {
private:
    fs::path root_path_;

    static std::string url_decode(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%' && i + 2 < str.size()) {
                int value = 0;
                std::istringstream is(str.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }

    static std::string get_mime_type(const std::string& path) {
        fs::path file_path(path);
        std::string ext = file_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), 
                       [](unsigned char c) { return std::tolower(c); });
        
        if (ext.empty()) return "application/octet-stream";
        if (ext == ".htm" || ext == ".html") return "text/html";
        if (ext == ".css") return "text/css";
        if (ext == ".txt") return "text/plain";
        if (ext == ".js") return "text/javascript";
        if (ext == ".json") return "application/json";
        if (ext == ".xml") return "application/xml";
        if (ext == ".png") return "image/png";
        if (ext == ".jpg" || ext == ".jpe" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".gif") return "image/gif";
        if (ext == ".bmp") return "image/bmp";
        if (ext == ".ico") return "image/vnd.microsoft.icon";
        if (ext == ".tiff" || ext == ".tif") return "image/tiff";
        if (ext == ".svg" || ext == ".svgz") return "image/svg+xml";
        if (ext == ".mp3") return "audio/mpeg";
        if (ext == ".fbx") return "application/octet-stream";
        
        return "application/octet-stream";
    }

    static bool is_path_within_root(const fs::path& root, const fs::path& path) {
        std::error_code ec;
        auto root_canonical = fs::weakly_canonical(root, ec);
        if (ec) return false;
        auto path_canonical = fs::weakly_canonical(path, ec);
        if (ec) return false;
        
        auto it_root = root_canonical.begin();
        auto it_path = path_canonical.begin();
        
        for (; it_root != root_canonical.end(); ++it_root, ++it_path) {
            if (it_path == path_canonical.end()) return false;
            if (*it_root != *it_path) return false;
        }
        return true;
    }

    static std::string read_file(const fs::path& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) return "";
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

public:
    StaticContentHandler() = default;
    StaticContentHandler(const std::string& root_path) : root_path_(root_path) {}
    
    void set_root_path(const std::string& root_path) {
        root_path_ = fs::path(root_path);
    }
    
    struct Response {
        int status_code = 200;
        std::string status_message = "OK";
        std::string content_type = "text/plain";
        std::string body;
        size_t content_length = 0;
    };
    
    Response handle_request(const std::string& uri) {
        Response response;
        std::string decoded_path = url_decode(uri);
        
        size_t query_pos = decoded_path.find('?');
        if (query_pos != std::string::npos) {
            decoded_path = decoded_path.substr(0, query_pos);
        }
        
        if (!decoded_path.empty() && decoded_path[0] == '/') {
            decoded_path = decoded_path.substr(1);
        }
        
        fs::path full_path = root_path_ / decoded_path;
        
        if (!is_path_within_root(root_path_, full_path)) {
            response.status_code = 400;
            response.status_message = "Bad Request";
            response.body = "Bad Request";
            response.content_length = response.body.size();
            return response;
        }
        
        std::error_code ec;
        if (fs::is_directory(full_path, ec)) {
            full_path /= "index.html";
        }
        
        if (ec || !fs::exists(full_path, ec) || !fs::is_regular_file(full_path, ec)) {
            response.status_code = 404;
            response.status_message = "Not Found";
            response.body = "File not found";
            response.content_length = response.body.size();
            return response;
        }
        
        response.body = read_file(full_path);
        response.content_type = get_mime_type(full_path.string());
        response.content_length = response.body.size();
        return response;
    }
};
