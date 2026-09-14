#include "htmldecode.h"

#include <array>
#include <cstddef>
#include <string_view>

namespace {

// Описание одной HTML-мнемоники.
// name — имя без ведущего '&' и без завершающей ';' (в нижнем регистре).
// decoded — символ, в который мнемоника декодируется.
struct Entity {
    std::string_view name;
    char decoded;
};

constexpr std::array<Entity, 5> ENTITIES = {{
    {"lt",   '<'},
    {"gt",   '>'},
    {"amp",  '&'},
    {"apos", '\''},
    {"quot", '"'},
}};

// Проверяет, что фрагмент str[pos..pos+pattern.size()) совпадает
// с образцом без учёта регистра, НО все буквы фрагмента должны быть
// записаны в одном регистре — либо целиком строчными, либо целиком
// заглавными. Смешанный регистр (например, "Lt") не допускается.
bool MatchesSameCase(std::string_view str, std::size_t pos,
                     std::string_view pattern) {
    bool all_lower = true;
    bool all_upper = true;

    for (std::size_t i = 0; i < pattern.size(); ++i) {
        const char c = str[pos + i];
        if (c >= 'a' && c <= 'z') {
            all_upper = false;
        } else if (c >= 'A' && c <= 'Z') {
            all_lower = false;
        } else {
            // Не буква — не может быть частью имени мнемоники
            return false;
        }

        char lower = c;
        if (lower >= 'A' && lower <= 'Z') {
            lower = static_cast<char>(lower - 'A' + 'a');
        }
        if (lower != pattern[i]) {
            return false;
        }
    }
    return all_lower || all_upper;
}

// Пытается распознать мнемонику в позиции pos (str[pos] == '&').
// Возвращает {декодированный символ, длина съеденной последовательности}.
// Если мнемоника не распознана — {0, 0}.
std::pair<char, std::size_t> TryDecodeEntity(std::string_view str,
                                             std::size_t pos) {
    for (const auto& entity : ENTITIES) {
        const std::size_t name_len = entity.name.size();
        const std::size_t after_name = pos + 1 + name_len;  // пропускаем '&'

        // Не хватает символов даже на '&' + name
        if (after_name > str.size()) {
            continue;
        }
        if (!MatchesSameCase(str, pos + 1, entity.name)) {
            continue;
        }

        // Мнемоника распознана. Проверяем опциональную ';'
        if (after_name < str.size() && str[after_name] == ';') {
            return {entity.decoded, name_len + 2};  // '&' + name + ';'
        }
        return {entity.decoded, name_len + 1};      // '&' + name
    }
    return {0, 0};
}

}  // namespace

std::string HtmlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    std::size_t i = 0;
    while (i < str.size()) {
        if (str[i] == '&') {
            auto [decoded, consumed] = TryDecodeEntity(str, i);
            if (consumed != 0) {
                result.push_back(decoded);
                i += consumed;
                continue;
            }
        }
        // Не мнемоника — копируем символ как есть.
        // Декодированные ранее символы повторно не анализируются,
        // так как мы двигаем i вперёд.
        result.push_back(str[i]);
        ++i;
    }
    return result;
}
