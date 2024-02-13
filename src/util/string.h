#pragma once

#include <vector>

namespace strings
{

    typedef bool (*charset_predicate)(char);

    enum class ownership : u8 {
        CREATE_COPY,
        TAKE_OWNERSHIP,
        VIEW_ONLY,
    };

    struct string {
        union {
            char small[24];
            struct lg {
                char* data;
                u32 length;
                u32 size;
                u32 pad0;
                u16 pad1;
                ownership ownership;
                s8 marker;
            } large;
        };
        static_assert(sizeof(large) == sizeof(small));

        string() : string(0U) {}
        string(const char* s);
        string(const char* s, ownership own);
        string(const char* s, u32 len);
        string(const char* s, u32 len, ownership own);
        explicit string(u32 reserved_size);
        // TODO: varargs ctor
        string(const string& o);
        string(string&& o) noexcept;
        string& operator=(const string& o);
        string& operator=(string&& o) noexcept;
        string& operator=(const char* s);
        ~string();

        char operator[](u32 index) const { return charAt(index); }

        char* release();

        bool isSmallString() const;
        void tryShrinkToSmall();
        void ensureOwnership();

        u32 size() const;
        u32 capacity() const;

        bool empty() const { return size() == 0; }
        operator char* () { return mut_str(); }
        operator const char* () const { return c_str(); }

        char* mut_str();
        const char* c_str() const;
        const char* data() const { return c_str(); }

        bool equals(const char* o) const;
        bool equals(const string& o) const;

        friend bool operator==(const string& a, const char* b) {
            return a.equals(b);
        }
        friend bool operator==(const char* b, const string& a) {
            return a.equals(b);
        }
        friend bool operator==(const string& a, const string& b) {
            return a.equals(b);
        }
        friend bool operator!=(const string& a, const char* b) {
            return !a.equals(b);
        }
        friend bool operator!=(const string& a, const string& b) {
            return !a.equals(b);
        }
        friend bool operator<(const string& a, const string& b) {
            for (u32 i = 0; i < a.size(); i++) {
                if (i >= b.size()) {
                    return false;
                }
                char a0 = a.charAt(i);
                char b0 = b.charAt(i);
                if (a0 != b0) {
                    return a0 < b0;
                }
            }
            return a.size() < b.size();
        }
        friend bool operator>(const string& a, const string& b) { return b < a; }
        friend bool operator<=(const string& a, const string& b) { return !(a > b); }
        friend bool operator>=(const string& a, const string& b) { return !(a < b); }

        bool equalsIgnoresCase(const char* o) const;
        bool equalsIgnoresCase(const string& o) const;

        bool startsWith(const char* s) const;
        bool startsWith(const string& o) const;

        bool endsWith(const char* s) const;
        bool endsWith(const string& o) const;

        bool contains(char c) const;
        bool contains(const char* c) const;

        // Returns the index of the character in the string.
        // If the character is not found, returns -1.
        s32 indexOf(char c) const;
        // Returns the index of the character in the string starting from the `from` index.
        // Note that the `from` index is inclusive.
        // If the character is not found, returns -1.
        // If the `from` index is out of bounds, returns -1.
        // If the `from` index is negative, it is taken as an offset from the end of the string.
        // If the `from` index is negative and the string is too short it is clamped to the size of the string.
        s32 indexOf(char c, s32 from) const;
        // Returns the index of the substring in the string.
        // If the substring is not found, returns -1.
        s32 indexOf(const char* s) const;
        // Returns the index of the substring in the string starting from the `from` index.
        // Note that the `from` index is inclusive.
        // If the substring is not found, returns -1.
        // If the `from` index is out of bounds, returns -1.
        // If the `from` index is negative, it is taken as an offset from the end of the string.
        // If the `from` index is negative and the string is too short, returns -1.
        s32 indexOf(const char* s, s32 from) const;
        // Returns the last index of the character in the string.
        // If the character is not found, returns -1.
        s32 lastIndexOf(char c) const;
        // Returns the last index of the character in the string starting from the `from` index.
        // Note that the `from` index is inclusive and the character is searched for prior to this.
        // If the character is not found, returns -1.
        // If the `from` index is out of bounds it is clamped to the size of the string.
        // If the `from` index is negative, it is taken as an offset from the end of the string.
        // If the `from` index is negative and the string is too short, returns -1.
        s32 lastIndexOf(char c, s32 from) const;

        void pop(u32 count);

        void trim();
        void trimLeft();
        void trimRight();

        void trim(char c);
        void trimLeft(char c);
        void trimRight(char c);

        void trim(charset_predicate c);
        void trimLeft(charset_predicate c);
        void trimRight(charset_predicate c);

        void toLower();
        void toUpper();

        void append(const char* s);
        void append(const char* s, u32 len);
        void append(const string& o);
        void append(char c);
        void appendf(const char* fmt, ...);
        void vappend(const char* fmt, va_list args);
        void reserve(u32 extra);

        void insert(u32 index, char c);
        void erase(u32 index);

        void clip(u32 from, u32 to);

        string substring(u32 from) const;
        string substring(u32 from, u32 to) const;

        void replace(char from, char to);
        void replaceAll(const char* from, const char* to);

        void escape();
        void unescape();

        void set(u32 index, char c);
        char charAt(u32 index) const;

        int compareTo(const string& s) const;

        std::vector<string> split(char c) const;
    };

    bool equals(const char* a, const char* b);
    bool startsWith(const char* str, const char* pre);
    bool endsWith(const char* str, const char* suffix);
    bool contains(const char* str, char n);

    char* dup(const char* s);

    bool isNumber(char n);
    bool isFloatChar(char c);
    bool isHexChar(char n);
    bool isLetter(char n);
    bool isWhitespace(char n);
    // Returns true if the string is a valid identifier.
    // The first character must be a letter or an underscore.
    // The rest of the characters must be letters, underscores or numbers.
    // Empty strings are not valid identifiers.
    bool isIdentifier(const char* str);
    // Returns true if the string is a valid integer.
    // The integer may be preceeded by a sign (+/-).
    // Hexadecimal/octal integers are not supported.
    bool isInteger(const char* str);
    bool isFloat(const char* str);

    char getHexChar(u8 v);
    char getEscapeChar(char c);

    char toLower(char c);
    string toLower(const string& s);
    char toUpper(char c);
}

typedef ::strings::string sstring;

sstring prettyTimeDelta(s64 delta_nanos);
