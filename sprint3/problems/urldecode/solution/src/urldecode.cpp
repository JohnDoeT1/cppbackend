#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <cctype>

std::string UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%') {
            // Проверяем, что есть минимум 2 символа после %
            if (i + 2 >= str.size()) {
                throw std::invalid_argument("Incomplete percent-encoding sequence");
            }
            
            // Проверяем, что оба символа - шестнадцатеричные цифры
            char hex1 = str[i + 1];
            char hex2 = str[i + 2];
            
            if (!std::isxdigit(static_cast<unsigned char>(hex1)) || 
                !std::isxdigit(static_cast<unsigned char>(hex2))) {
                throw std::invalid_argument("Invalid percent-encoding sequence");
            }
            
            // Конвертируем hex в число
            unsigned int value;
            auto [ptr, ec] = std::from_chars(str.data() + i + 1, str.data() + i + 3, value, 16);
            
            if (ec != std::errc() || ptr != str.data() + i + 3) {
                throw std::invalid_argument("Invalid percent-encoding sequence");
            }
            
            result += static_cast<char>(value);
            i += 2; // Пропускаем два hex-символа
        } else {
            result += str[i];
        }
    }
    
    return result;
}
