#pragma once

#include <cerrno>
#include <charconv>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace ConfigJson {

enum Type { Boolean, Number, String, Array, Other };
enum FieldType { BoolField, IntField, FloatField, ColorField, StringField };

struct Value {
    Type type = Other;
    bool boolean = false;
    double number = 0.0;
    std::string text;
    std::vector<double> array;
};

struct Field {
    const char* name;
    FieldType type;
    void* target;
    size_t capacity = 0;
};

class Document {
    const std::string* source = nullptr;
    size_t cursor = 0;
    std::map<std::string, Value> values;

    void Space() {
        while (cursor < source->size() && ((*source)[cursor] == ' ' || (*source)[cursor] == '\n' || (*source)[cursor] == '\r' || (*source)[cursor] == '\t')) ++cursor;
    }

    bool Consume(char ch) {
        Space();
        if (cursor >= source->size() || (*source)[cursor] != ch) return false;
        ++cursor;
        return true;
    }

    bool Hex(unsigned& value) {
        value = 0;
        for (int i = 0; i < 4; ++i) {
            if (cursor >= source->size()) return false;
            char ch = (*source)[cursor++];
            int digit = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
            if (digit < 0) return false;
            value = value * 16 + static_cast<unsigned>(digit);
        }
        return true;
    }

    static void Utf8(std::string& out, unsigned value) {
        if (value < 0x80) out += static_cast<char>(value);
        else if (value < 0x800) { out += static_cast<char>(0xc0 | (value >> 6)); out += static_cast<char>(0x80 | (value & 63)); }
        else if (value < 0x10000) { out += static_cast<char>(0xe0 | (value >> 12)); out += static_cast<char>(0x80 | ((value >> 6) & 63)); out += static_cast<char>(0x80 | (value & 63)); }
        else { out += static_cast<char>(0xf0 | (value >> 18)); out += static_cast<char>(0x80 | ((value >> 12) & 63)); out += static_cast<char>(0x80 | ((value >> 6) & 63)); out += static_cast<char>(0x80 | (value & 63)); }
    }

    bool Text(std::string& out) {
        if (!Consume('"')) return false;
        out.clear();
        while (cursor < source->size()) {
            unsigned char ch = static_cast<unsigned char>((*source)[cursor++]);
            if (ch == '"') return true;
            if (ch < 0x20) return false;
            if (ch != '\\') { out += static_cast<char>(ch); continue; }
            if (cursor >= source->size()) return false;
            char escape = (*source)[cursor++];
            if (escape == '"' || escape == '\\' || escape == '/') out += escape;
            else if (escape == 'b') out += '\b';
            else if (escape == 'f') out += '\f';
            else if (escape == 'n') out += '\n';
            else if (escape == 'r') out += '\r';
            else if (escape == 't') out += '\t';
            else if (escape == 'u') {
                unsigned code = 0;
                if (!Hex(code)) return false;
                if (code >= 0xd800 && code <= 0xdbff) {
                    if (cursor + 2 > source->size() || source->substr(cursor, 2) != "\\u") return false;
                    cursor += 2;
                    unsigned low = 0;
                    if (!Hex(low) || low < 0xdc00 || low > 0xdfff) return false;
                    code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                } else if (code >= 0xdc00 && code <= 0xdfff) return false;
                if (code == 0) return false;
                Utf8(out, code);
            } else return false;
        }
        return false;
    }

    bool ParseValue(Value& value, int depth) {
        if (depth > 24) return false;
        Space();
        if (cursor >= source->size()) return false;
        char ch = (*source)[cursor];
        if (ch == '"') { value.type = String; return Text(value.text); }
        if (ch == '{') {
            ++cursor;
            value.type = Other;
            if (Consume('}')) return true;
            do { std::string key; Value child; if (!Text(key) || !Consume(':') || !ParseValue(child, depth + 1)) return false; } while (Consume(','));
            return Consume('}');
        }
        if (ch == '[') {
            ++cursor;
            value.type = Array;
            if (Consume(']')) return true;
            do {
                Value child;
                if (!ParseValue(child, depth + 1)) return false;
                if (child.type == Number) value.array.push_back(child.number);
                else value.type = Other;
            } while (Consume(','));
            return Consume(']');
        }
        for (const char* literal : { "true", "false", "null" }) {
            size_t length = std::strlen(literal);
            if (source->compare(cursor, length, literal) == 0) {
                cursor += length;
                value.type = literal[0] == 'n' ? Other : Boolean;
                value.boolean = literal[0] == 't';
                return true;
            }
        }
        size_t begin = cursor;
        if (ch == '-') ++cursor;
        auto digit = [&]() { return cursor < source->size() && (*source)[cursor] >= '0' && (*source)[cursor] <= '9'; };
        if (!digit()) return false;
        if ((*source)[cursor] == '0') ++cursor;
        else while (digit()) ++cursor;
        if (cursor < source->size() && (*source)[cursor] == '.') { ++cursor; if (!digit()) return false; while (digit()) ++cursor; }
        if (cursor < source->size() && ((*source)[cursor] == 'e' || (*source)[cursor] == 'E')) {
            ++cursor;
            if (cursor < source->size() && ((*source)[cursor] == '+' || (*source)[cursor] == '-')) ++cursor;
            if (!digit()) return false;
            while (digit()) ++cursor;
        }
        auto converted = std::from_chars(source->data() + begin, source->data() + cursor, value.number);
        value.type = Number;
        return converted.ec == std::errc() && converted.ptr == source->data() + cursor && std::isfinite(value.number);
    }

public:
    bool Parse(const std::string& input) {
        source = &input; cursor = 0; values.clear();
        if (input.size() >= 3 && input.compare(0, 3, "\xef\xbb\xbf") == 0) cursor = 3;
        if (!Consume('{')) return false;
        if (!Consume('}')) {
            do {
                std::string key;
                Value value;
                if (!Text(key) || values.count(key) || !Consume(':') || !ParseValue(value, 0)) return false;
                values.emplace(key, value);
            } while (Consume(','));
            if (!Consume('}')) return false;
        }
        Space();
        return cursor == input.size();
    }

    bool Has(const char* name) const { return values.count(name) != 0; }

    int Version() const {
        auto it = values.find("_v");
        if (it == values.end()) return 0;
        const auto& value = it->second;
        if (value.type != Number || value.number < 0 || value.number > INT_MAX || value.number != std::trunc(value.number)) return -1;
        return static_cast<int>(value.number);
    }

    bool Apply(const std::vector<Field>& fields) const {
        for (const auto& field : fields) {
            auto it = values.find(field.name);
            if (it == values.end()) continue;
            const auto& value = it->second;
            if (field.type == BoolField && value.type != Boolean) return false;
            if (field.type == StringField && (value.type != String || field.capacity == 0)) return false;
            if (field.type == IntField && (value.type != Number || value.number < INT_MIN || value.number > INT_MAX || value.number != std::trunc(value.number))) return false;
            if (field.type == FloatField && (value.type != Number || std::fabs(value.number) > std::numeric_limits<float>::max())) return false;
            if (field.type == ColorField) {
                if (value.type != Array || value.array.size() != 4) return false;
                for (double number : value.array) if (number < 0.0 || number > 1.0) return false;
            }
        }
        for (const auto& field : fields) {
            auto it = values.find(field.name);
            if (it == values.end()) continue;
            const auto& value = it->second;
            if (field.type == BoolField) *static_cast<bool*>(field.target) = value.boolean;
            else if (field.type == IntField) *static_cast<int*>(field.target) = static_cast<int>(value.number);
            else if (field.type == FloatField) *static_cast<float*>(field.target) = static_cast<float>(value.number);
            else if (field.type == ColorField) for (int i = 0; i < 4; ++i) static_cast<float*>(field.target)[i] = static_cast<float>(value.array[i]);
            else {
                size_t count = value.text.size() < field.capacity - 1 ? value.text.size() : field.capacity - 1;
                std::memcpy(field.target, value.text.data(), count);
                static_cast<char*>(field.target)[count] = 0;
            }
        }
        return true;
    }
};


inline std::string Escape(const char* text) {
    std::string result;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        if (*p == '"' || *p == '\\') { result += '\\'; result += static_cast<char>(*p); }
        else if (*p < 0x20) { char buffer[7]; std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(*p)); result += buffer; }
        else result += static_cast<char>(*p);
    }
    return result;
}

}
