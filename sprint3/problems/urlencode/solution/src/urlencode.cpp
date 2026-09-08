#include "urlencode.h"

#include <iomanip>
#include <sstream>

std::string UrlEncode(std::string_view str) {
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase;
    
    for (unsigned char c : str) {
        // Пробел кодируется как +
        if (c == ' ') {
            encoded << '+';
        }
        // Незарезервированные символы: буквы, цифры, -._~
        else if ((c >= 'A' && c <= 'Z') || 
                 (c >= 'a' && c <= 'z') || 
                 (c >= '0' && c <= '9') ||
                 c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        }
        // Все остальные символы (включая зарезервированные, 
        // символы с кодом < 32 и >= 128) кодируются как %XX
        else {
            encoded << '%' << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(c);
        }
    }
    
    return encoded.str();
}
