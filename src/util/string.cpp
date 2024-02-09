#include "util/string.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "util/scalar_math.h"

namespace strings
{

    string::string(const char* s)
        : string(s, UINT32_MAX, ownership::CREATE_COPY)
    {
    }

    string::string(const char* s, ownership o)
        : string(s, UINT32_MAX, o)
    {
    }

    string::string(const char* s, u32 len)
        : string(s, len, ownership::CREATE_COPY)
    {
    }

    string::string(const char* s, u32 len, ownership own)
    {
        if (s == nullptr)
        {
            small[0] = '\0';
            small[23] = 23;
            return;
        }
        if (len == UINT32_MAX)
        {
            len = (u32)strlen(s);
        }
        if (len < 24)
        {
            // small string
            u8 stored_size = (u8)(23 - len);
            memcpy(small, s, len);
            small[len] = '\0';
            small[23] = stored_size;
            if (own == ownership::TAKE_OWNERSHIP)
            {
                delete[] s;
            }
        }
        else
        {
            char* cpy;
            if (own == ownership::TAKE_OWNERSHIP || own == ownership::VIEW_ONLY)
            {
                cpy = (char*)s;
            }
            else
            {
                cpy = new char[len + 1];
                memcpy(cpy, s, len);
                cpy[len] = '\0';
            }
            large.data = cpy;
            large.length = len;
            large.size = len + 1;
            large.marker = -128;
            large.ownership = own;
        }
    }

    string::string(u32 reserved_size)
    {
        if (reserved_size < 24)
        {
            small[0] = '\0';
            small[23] = 23;
        }
        else
        {
            char* cpy = new char[reserved_size + 1];
            cpy[0] = '\0';
            large.data = cpy;
            large.length = 0;
            large.size = reserved_size + 1;
            large.marker = -128;
            large.ownership = ownership::CREATE_COPY;
        }
    }

    string::string(const string& o)
    {
        if (o.isSmallString())
        {
            memcpy(small, o.small, 24);
        }
        else
        {
            if (o.large.ownership == ownership::VIEW_ONLY)
            {
                large.data = o.large.data;
            }
            else
            {
                char* cpy = new char[o.large.size];
                memcpy(cpy, o.large.data, o.large.size);
                large.data = cpy;
            }
            large.length = o.large.length;
            large.size = o.large.size;
            large.marker = o.large.marker;
            large.ownership = o.large.ownership;
        }
    }

    string::string(string&& o) noexcept
    {
        memcpy(small, o.small, 24);
        if (!o.isSmallString())
        {
            o.large.data = nullptr;
        }
    }

    string& string::operator=(const string& o)
    {
        if (&o != this)
        {
            if (!isSmallString() && large.data != nullptr && large.ownership != ownership::VIEW_ONLY)
            {
                delete[] large.data;
            }
            if (o.isSmallString())
            {
                memcpy(small, o.small, 24);
            }
            else
            {
                if (o.large.ownership == ownership::VIEW_ONLY)
                {
                    large.data = o.large.data;
                }
                else
                {
                    char* cpy = new char[o.large.size];
                    memcpy(cpy, o.large.data, o.large.size);
                    large.data = cpy;
                }
                large.length = o.large.length;
                large.size = o.large.size;
                large.marker = o.large.marker;
                large.ownership = o.large.ownership;
            }
        }
        return *this;
    }

    string& string::operator=(string&& o) noexcept
    {
        if (&o != this)
        {
            if (!isSmallString() && large.data != nullptr && large.ownership != ownership::VIEW_ONLY)
            {
                delete[] large.data;
            }
            memcpy(small, o.small, 24);
            if (!o.isSmallString())
            {
                o.large.data = nullptr;
            }
        }
        return *this;
    }

    string& string::operator=(const char* s)
    {
        u32 slen = (u32)strlen(s);
        if (slen < 24)
        {
            if (!isSmallString() && large.data != nullptr && large.ownership != ownership::VIEW_ONLY)
            {
                delete[] large.data;
            }
            u8 stored_size = (u8)(23 - slen);
            memcpy(small, s, slen);
            small[slen] = '\0';
            small[23] = stored_size;
        }
        else
        {
            if (!isSmallString())
            {
                if (large.size < slen + 1 || large.ownership == ownership::VIEW_ONLY)
                {
                    char* cpy = new char[slen + 1];
                    memcpy(cpy, s, (size_t)slen + 1);
                    large.data = cpy;
                }
                else
                {
                    memcpy(large.data, s, (size_t)slen + 1);
                }
                large.length = slen;
                large.size = slen + 1;
                large.marker = -128;
                large.ownership = ownership::CREATE_COPY;
            }
            else
            {
                char* cpy = new char[slen + 1];
                memcpy(cpy, s, (size_t)slen + 1);
                large.data = cpy;
                large.length = slen;
                large.size = slen + 1;
                large.marker = -128;
                large.ownership = ownership::CREATE_COPY;
            }
        }
        return *this;
    }

    string::~string()
    {
        if (!isSmallString())
        {
            if (large.data != nullptr && large.ownership != ownership::VIEW_ONLY)
            {
                delete[] large.data;
                large.data = nullptr;
                large.length = 0;
                large.size = 0;
            }
        }
    }

    char* string::release()
    {
        if (isSmallString())
        {
            return dup(small);
        }
        if (large.ownership == ownership::VIEW_ONLY)
        {
            return dup(large.data);
        }
        char* data = large.data;
        large.data = nullptr;
        large.length = 0;
        large.size = 0;
        large.ownership = ownership::VIEW_ONLY;
        return data;
    }

    bool string::isSmallString() const
    {
        return large.marker >= 0;
    }

    void string::tryShrinkToSmall()
    {
        if (!isSmallString() && large.length < 24)
        {
            char* data = large.data;
            u32 len = large.length;
            u8 stored_size = (u8)(23 - len);
            memcpy(small, data, len);
            small[len] = '\0';
            small[23] = stored_size;
        }
    }

    void string::ensureOwnership()
    {
        if (!isSmallString() && large.ownership == ownership::VIEW_ONLY)
        {
            char* cpy = new char[large.size];
            memcpy(cpy, large.data, large.length);
            large.data = cpy;
            large.data[large.length] = '\0';
            large.ownership = ownership::CREATE_COPY;
        }
    }

    u32 string::size() const
    {
        if (isSmallString())
            return 23 - small[23];
        else
            return large.length;
    }

    u32 string::capacity() const {
        if (isSmallString())
            return 23;
        else
            return large.size - 1;
    }

    char* string::mut_str() {
        if (isSmallString())
            return (char*)small;
        else {
            ensureOwnership();
            return large.data;
        }
    }

    const char* string::c_str() const {
        if (isSmallString())
            return (const char*)small;
        else
            return large.data;
    }

    bool string::equals(const char* o) const
    {
        size_t len = strlen(o);
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            if (len != sz)
                return false;
            for (u32 i = 0; i < sz; i++)
            {
                if (small[i] != o[i])
                    return false;
            }
        }
        else {
            u32 sz = large.length;
            if (len != sz)
                return false;
            for (u32 i = 0; i < sz; i++)
            {
                if (large.data[i] != o[i])
                    return false;
            }
        }
        return true;
    }

    bool string::equals(const string& o) const
    {
        if (isSmallString())
        {
            if (!o.isSmallString())
                return false;
            u32 sz = 23 - small[23];
            u32 len = 23 - o.small[23];
            if (len != sz)
                return false;
            for (u32 i = 0; i < sz; i++)
            {
                if (small[i] != o.small[i])
                    return false;
            }
        }
        else
        {
            if (o.isSmallString())
                return false;
            u32 sz = large.length;
            u32 len = o.large.length;
            if (len != sz)
                return false;
            for (u32 i = 0; i < sz; i++)
            {
                if (large.data[i] != o.large.data[i])
                    return false;
            }
        }
        return true;
    }

    bool string::equalsIgnoresCase(const char* o) const
    {
        size_t len = strlen(o);
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            if (len != sz)
                return false;
            for (u32 i = 0; i < sz; i++)
            {
                char a0 = ::strings::toLower(small[i]);
                char b0 = ::strings::toLower(o[i]);
                if (a0 != b0)
                    return false;
            }
        }
        else
        {
            u32 sz = large.length;
            if (len != sz)
                return false;
            for (u32 i = 0; i < sz; i++)
            {
                char a0 = ::strings::toLower(large.data[i]);
                char b0 = ::strings::toLower(o[i]);
                if (a0 != b0)
                    return false;
            }
        }
        return true;
    }

    bool string::equalsIgnoresCase(const sstring& o) const
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            if (o.size() != sz)
                return false;
            for (u32 i = 0; i < sz; i++)
            {
                char a0 = ::strings::toLower(small[i]);
                char b0 = ::strings::toLower(o[i]);
                if (a0 != b0)
                    return false;
            }
        }
        else
        {
            u32 sz = large.length;
            if (o.size() != sz)
                return false;
            for (u32 i = 0; i < sz; i++)
            {
                char a0 = ::strings::toLower(large.data[i]);
                char b0 = ::strings::toLower(o[i]);
                if (a0 != b0)
                    return false;
            }
        }
        return true;
    }

    bool string::startsWith(const char* s) const
    {
        size_t len = strlen(s);
        if (len > size()) return false;
        return strncmp(c_str(), s, len) == 0;
    }

    bool string::startsWith(const string& o) const
    {
        size_t len = o.size();
        if (len > size()) return false;
        return strncmp(c_str(), o.c_str(), len) == 0;
    }

    bool string::endsWith(const char* s) const
    {
        size_t len = strlen(s);
        if (len > size()) return false;
        return strncmp(c_str() + (size() - len), s, len) == 0;
    }

    bool string::endsWith(const string& o) const
    {
        size_t len = o.size();
        if (len > size()) return false;
        return strncmp(c_str() + (size() - len), o.c_str(), len) == 0;
    }

    bool string::contains(char c) const
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            for (u32 i = 0; i < sz; i++)
            {
                if (small[i] == c)
                    return true;
            }
        }
        else
        {
            u32 sz = large.length;
            for (u32 i = 0; i < sz; i++)
            {
                if (large.data[i] == c)
                    return true;
            }
        }
        return false;
    }

    bool string::contains(const char* c) const
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            u32 offset = 0;
            for (u32 i = 0; i < sz; i++)
            {
                if (small[i] != c[offset])
                    offset = 0;
                else
                {
                    offset++;
                    if (c[offset] == '\0')
                        return true;
                }
            }
            if (c[offset] == '\0')
                return true;
        }
        else
        {
            u32 sz = large.length;
            u32 offset = 0;
            for (u32 i = 0; i < sz; i++)
            {
                if (large.data[i] != c[offset])
                    offset = 0;
                else
                {
                    offset++;
                    if (c[offset] == '\0')
                        return true;
                }
            }
            if (c[offset] == '\0')
                return true;
        }
        return false;
    }


    s32 string::indexOf(char c) const
    {
        return indexOf(c, 0);
    }

    s32 string::indexOf(char c, s32 from) const
    {
        if (isSmallString()) {
            u32 sz = 23 - small[23];
            if (from < 0) {
                from = sz + from;
                if (from < 0)
                    from = 0;
            }
            if (from > (s32) sz)
                return -1;
            for (u32 i = from; i < sz; i++)
            {
                if (small[i] == c)
                    return i;
            }
        }
        else
        {
            u32 sz = large.length;
            if (from < 0)
                from = sz + from;
            if (from < 0)
                return -1;
            if (from > (s32) sz)
                return -1;
            for (u32 i = from; i < sz; i++)
            {
                if (large.data[i] == c)
                    return i;
            }
        }
        return -1;
    }

    s32 string::indexOf(const char* s) const
    {
        return indexOf(s, 0);
    }

    s32 string::indexOf(const char* s, s32 from) const
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            if (from < 0)
                from = sz + from;
            if (from < 0)
                return -1;
            if (from > (s32) sz)
                return -1;
            s32 start = -1;
            s32 o = 0;
            for (u32 i = from; i < sz; i++)
            {
                if (small[i] == s[o])
                {
                    if (start == -1)
                        start = i;
                    o++;
                    if (s[o] == '\0')
                        return start;
                }
                else
                {
                    o = 0;
                    start = -1;
                }
            }
        }
        else
        {
            u32 sz = large.length;
            if (from < 0)
                from = sz + from;
            if (from < 0)
                return -1;
            if (from > (s32) sz)
                return -1;
            s32 start = -1;
            s32 o = 0;
            for (u32 i = from; i < sz; i++)
            {
                if (large.data[i] == s[o])
                {
                    if (start == -1)
                        start = i;
                    o++;
                    if (s[o] == '\0')
                        return start;
                }
                else
                {
                    o = 0;
                    start = -1;
                }
            }
        }
        return -1;
    }

    s32 string::lastIndexOf(char c) const
    {
        return lastIndexOf(c, -1);
    }

    s32 string::lastIndexOf(char c, s32 from) const
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            if (from < 0)
                from = (s32)sz + from;
            if (from < 0)
                return -1;
            if (from > (s32) sz - 1)
                from = (s32)sz - 1;
            for (s32 i = from; i >= 0; i--)
            {
                if (small[i] == c)
                    return i;
            }
        }
        else
        {
            u32 sz = large.length;
            if (from < 0) {
                from = (s32)sz + from;
            }
            if (from < 0)
                return -1;
            if (from > (s32) sz - 1)
                from = (s32)sz - 1;
            for (s32 i = from; i >= 0; i--)
            {
                if (large.data[i] == c)
                    return i;
            }
        }
        return -1;
    }

    void string::trim()
    {
        trim(isWhitespace);
    }

    void string::trimLeft()
    {
        trimLeft(isWhitespace);
    }

    void string::trimRight()
    {
        trimRight(isWhitespace);
    }

    void string::trim(char p)
    {
        trimRight(p);
        trimLeft(p);
    }

    void string::trimLeft(char p)
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            u32 i = 0;
            for (; i < sz; i++)
            {
                char n = small[i];
                if (p != n)
                    break;
            }
            if (i > 0)
            {
                memmove(small, small + i, (size_t)sz - i + 1);
                small[23] = (char)(23 - (sz - i));
            }
        }
        else
        {
            ensureOwnership();
            u32 sz = large.length;
            u32 i = 0;
            for (; i < sz; i++)
            {
                char n = large.data[i];
                if (p != n)
                    break;
            }
            if (i > 0)
            {
                u32 new_size = large.length - i;
                if (new_size < 24)
                {
                    memcpy(small, large.data + i, (size_t)new_size + 1);
                    small[23] = (char)(23 - new_size);
                }
                else
                {
                    memmove(large.data, large.data + i, (size_t)sz - i + 1);
                    large.length -= i;
                }
            }
        }
    }

    void string::trimRight(char p)
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            s32 i = (s32)sz - 1;
            for (; i >= 0; i--)
            {
                char n = small[i];
                if (p != n)
                    break;
            }
            s32 highest_whitespace = i + 1;
            if (highest_whitespace < (s32)sz)
            {
                small[highest_whitespace] = '\0';
                small[23] = (char)(23 - highest_whitespace);
            }
        }
        else
        {
            ensureOwnership();
            u32 sz = large.length;
            s32 i = (s32)sz - 1;
            for (; i >= 0; i--)
            {
                char n = large.data[i];
                if (p != n)
                    break;
            }
            s32 highest_whitespace = i + 1;
            if (highest_whitespace < (s32)sz)
            {
                large.data[highest_whitespace] = '\0';
                large.length = highest_whitespace;
                if (large.length < 24)
                    tryShrinkToSmall();
            }
        }
    }

    void string::trim(charset_predicate p)
    {
        trimRight(p);
        trimLeft(p);
    }

    void string::trimLeft(charset_predicate p)
    {
        if (isSmallString()) {
            u32 sz = 23 - small[23];
            u32 i = 0;
            for (; i < sz; i++)
            {
                char n = small[i];
                if (!p(n))
                    break;
            }
            if (i > 0)
            {
                memmove(small, small + i, (size_t)sz - i + 1);
                small[23] = (char)(23 - (sz - i));
            }
        }
        else
        {
            ensureOwnership();
            u32 sz = large.length;
            u32 i = 0;
            for (; i < sz; i++)
            {
                char n = large.data[i];
                if (!p(n))
                    break;
            }
            if (i > 0)
            {
                u32 new_size = large.length - i;
                if (new_size < 24)
                {
                    memcpy(small, large.data + i, (size_t)new_size + 1);
                    small[23] = (char)(23 - new_size);
                }
                else
                {
                    memmove(large.data, large.data + i, (size_t)sz - i + 1);
                    large.length -= i;
                }
            }
        }
    }

    void string::trimRight(charset_predicate p)
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            s32 i = (s32)sz - 1;
            for (; i >= 0; i--)
            {
                char n = small[i];
                if (!p(n))
                    break;
            }
            s32 highest_whitespace = i + 1;
            if (highest_whitespace < (s32)sz) {
                small[highest_whitespace] = '\0';
                small[23] = (char)(23 - highest_whitespace);
            }
        }
        else
        {
            ensureOwnership();
            u32 sz = large.length;
            s32 i = (s32)sz - 1;
            for (; i >= 0; i--)
            {
                char n = large.data[i];
                if (!p(n))
                    break;
            }
            s32 highest_whitespace = i + 1;
            if (highest_whitespace < (s32)sz)
            {
                large.data[highest_whitespace] = '\0';
                large.length = highest_whitespace;
                if (large.length < 24)
                    tryShrinkToSmall();
            }
        }
    }

    void string::toLower()
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            for (int i = sz - 1; i >= 0; i--)
            {
                if (small[i] >= 'A' && small[i] <= 'Z')
                    small[i] = (small[i] - 'A') + 'a';
            }
        }
        else
        {
            ensureOwnership();
            u32 sz = large.length;
            for (int i = sz - 1; i >= 0; i--)
            {
                if (large.data[i] >= 'A' && large.data[i] <= 'Z')
                    large.data[i] = (large.data[i] - 'A') + 'a';
            }
        }
    }

    void string::toUpper()
    {
        if (isSmallString())
        {
            u32 sz = 23 - small[23];
            for (int i = sz - 1; i >= 0; i--)
            {
                if (small[i] >= 'a' && small[i] <= 'z')
                    small[i] = (small[i] - 'a') + 'A';
            }
        }
        else
        {
            ensureOwnership();
            u32 sz = large.length;
            for (int i = sz - 1; i >= 0; i--)
            {
                if (large.data[i] >= 'a' && large.data[i] <= 'z')
                    large.data[i] = (large.data[i] - 'a') + 'A';
            }
        }
    }

    void string::append(const char* s)
    {
        u32 len = (u32)strlen(s);
        append(s, len);
    }

    void string::append(const char* s, u32 len)
    {
        u32 sz = size();
        if (isSmallString())
        {
            if (sz + len < 24)
            {
                // Result is still a small string
                memcpy(small + sz, s, len);
                small[sz + len] = '\0';
                small[23] = (char)(23 - (sz + len));
            }
            else
            {
                // Result will be a large string
                // Note: Ensure the size is at least 64 to avoid immediately reallocating
                // if we're doing a one-by-one append.
                u32 new_len = scalar::max(sz + len + 1, 64U);
                char* cpy = new char[new_len];
                memcpy(cpy, small, sz);
                memcpy(cpy + sz, s, len);
                cpy[sz + len] = '\0';
                large.data = cpy;
                large.length = sz + len;
                large.size = new_len;
                large.marker = -128;
                large.ownership = ownership::CREATE_COPY;
            }
        }
        else
        {
            if (large.length + len >= large.size || large.ownership == ownership::VIEW_ONLY)
            {
                // Result will be larger than our current buffer
                u32 new_len = scalar::max(large.length + len + 1, large.size + 256);
                char* cpy =  new char[new_len];
                memcpy(cpy, large.data, large.length);
                large.data = cpy;
                large.data[large.length] = '\0';
                large.size = new_len;
                large.ownership = ownership::CREATE_COPY;
            }
            memcpy(large.data + large.length, s, len);
            large.length += len;
            large.data[large.length] = '\0';
        }
    }

    void string::append(const string& o)
    {
        append(o.c_str());
    }

    void string::append(char c)
    {
        u32 sz = size();
        if (isSmallString())
        {
            if (sz + 1 < 24)
            {
                // Result is still a small string
                small[sz] = c;
                small[sz + 1] = '\0';
                small[23] = (char)(23 - (sz + 1));
            }
            else
            {
                // Result will be a large string
                // Note: Ensure the size is at least 64 to avoid immediately reallocating
                // if we're doing a one-by-one append.
                u32 new_len = 64U;
                char* cpy = new char[new_len];
                memcpy(cpy, small, sz);
                cpy[sz] = c;
                cpy[sz + 1] = '\0';
                large.data = cpy;
                large.length = sz + 1;
                large.size = new_len;
                large.marker = -128;
                large.ownership = ownership::CREATE_COPY;
            }
        }
        else
        {
            if (large.length + 1 >= large.size || large.ownership == ownership::VIEW_ONLY)
            {
                // Result will be larger than our current buffer
                u32 new_len = large.size + 256;
                char* cpy = new char[new_len];
                memcpy(cpy, large.data, large.length);
                large.data = cpy;
                large.data[large.length] = '\0';
                large.size = new_len;
                large.ownership = ownership::CREATE_COPY;
            }
            large.data[large.length] = c;
            large.length++;
            large.data[large.length] = '\0';
        }
    }

    void string::appendf(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        vappend(fmt, args);
        va_end(args);
    }

    void string::vappend(const char* fmt, va_list args)
    {
        va_list args2;
        va_copy(args2, args);
        int required_space = vsnprintf(nullptr, 0, fmt, args2);
        va_end(args2);
        char* tmp = new char[required_space + 1];
        vsnprintf(tmp, (size_t)required_space + 1, fmt, args);
        append(tmp);
        delete[] tmp;
    }

    void string::reserve(u32 extra)
    {
        u32 sz = size();
        if (isSmallString())
        {
            if (sz + extra < 24)
            {
                // Result is still a small string
            }
            else
            {
                // Result will be a large string
                char* cpy = new char[sz + extra + 1];
                memcpy(cpy, small, sz);
                cpy[sz] = '\0';
                large.data = cpy;
                large.length = sz;
                large.size = sz + extra + 1;
                large.marker = -128;
                large.ownership = ownership::CREATE_COPY;
            }
        }
        else
        {
            if (large.length + extra >= large.size)
            {
                // Result will be larger than our current buffer
                u32 new_len = scalar::max(large.length + extra + 1, large.size + 256);
                char* cpy = new char[new_len];
                memcpy(cpy, large.data, large.length);
                large.data = cpy;
                large.data[large.length] = '\0';
                large.size = new_len;
                large.ownership = ownership::CREATE_COPY;
            }
        }
    }

    void string::insert(u32 index, char c)
    {
        u32 sz = size();
        debug_assert(index >= 0); // @Todo: Negagive indices for end-relative insertions
        debug_assert(sz >= index);
        reserve(sz + 1);
        if (isSmallString())
        {
            memmove(small + index + 1, small + index, (size_t)sz - index);
            small[index] = c;
            small[sz + 1] = '\0';
            small[23] = (char)(23 - (sz + 1));
        }
        else
        {
            memmove(large.data + index + 1, large.data + index, (size_t)large.length - index);
            large.data[index] = c;
            large.length++;
            large.data[large.length] = '\0';
        }
    }

    void string::erase(u32 index)
    {
        u32 sz = size();
        debug_assert(index >= 0); // @Todo: Negagive indices for end-relative insertions
        debug_assert(sz > index);
        if (isSmallString())
        {
            if (index != sz - 1)
                memmove(small + index, small + index + 1, (size_t)sz - index - 1);
            small[sz - 1] = '\0';
            small[23] = (char)(23 - (sz - 1));
        }
        else
        {
            if (index != sz - 1)
                memmove(large.data + index, large.data + index + 1, (size_t)large.length - index);
            else
                large.data[index] = '\0';
            large.length--;
        }
    }

    void string::clip(u32 from, u32 to)
    {
        from = scalar::min(scalar::min(from, to), size());
        to = scalar::min(scalar::max(from, to), size());
        u32 new_len = to - from;
        if (isSmallString())
        {
            if (from > 0)
                memmove(small, small + from, (size_t)new_len + 1);
            small[new_len] = '\0';
            small[23] = (char)(23 - new_len);
        }
        else
        {
            if (new_len < 24)
            {
                // Check if the clipped size is a small string so we can avoid
                // a potential copy if this string is a read-only view.
                char* data = large.data;
                debug_assert(data != nullptr);
                bool was_owned = large.ownership != ownership::VIEW_ONLY;
                memcpy(small, data + from, new_len);
                small[new_len] = '\0';
                small[23] = (char)(23 - new_len);
                if (was_owned)
                    delete[] data;
            }
            else
            {
                ensureOwnership();
                if (from > 0)
                    memmove(large.data, large.data + from, (size_t)new_len + 1);
                large.data[new_len] = '\0';
                large.length = new_len;
            }
        }
    }

    string string::substring(u32 from) const
    {
        return substring(from, size());
    }

    string string::substring(u32 from, u32 to) const
    {
        from = scalar::min(scalar::min(from, to), size());
        to = scalar::min(scalar::max(from, to), size());
        debug_assert(from <= to);
        u32 result_len = to - from;
        if (isSmallString())
            return string(small + from, result_len);
        else
            return string(large.data + from, result_len);
    }

    void string::replace(char from, char to) {
        if (isSmallString()) {
            u32 sz = 23 - small[23];
            for (int i = sz - 1; i >= 0; i--) {
                if (small[i] == from) {
                    small[i] = to;
                }
            }
        }
        else {
            ensureOwnership();
            u32 sz = large.length;
            for (int i = sz - 1; i >= 0; i--) {
                if (large.data[i] == from) {
                    large.data[i] = to;
                }
            }
        }
    }

    void string::escape() {
        u32 required_extra = 0;
        if (isSmallString()) {
            u32 sz = 23 - small[23];
            for (int i = sz - 1; i >= 0; i--) {
                char n = small[i];
                switch (n) {
                case '\n':
                case '\r':
                case '\t':
                case '\\':
                case '\'':
                case '"':
                    required_extra++;
                    break;
                default: break;
                }
            }
            reserve(required_extra);
        }
        else {
            ensureOwnership();
            u32 sz = large.length;
            for (int i = sz - 1; i >= 0; i--) {
                char n = large.data[i];
                switch (n) {
                case '\n':
                case '\r':
                case '\t':
                case '\\':
                case '\'':
                case '"':
                    required_extra++;
                    break;
                default: break;
                }
            }
            reserve(required_extra);
        }
        if (isSmallString()) {
            s32 sz = 23 - small[23];
            s32 remaining = (s32)required_extra;
            small[sz + remaining] = '\0';
            for (s32 i = (s32)sz - 1; i >= 0; i--) {
                char n = small[i];
                switch (n) {
                case '\n':
                case '\r':
                case '\t':
                case '\\':
                case '\'':
                case '"': {
                    small[i + remaining] = getEscapeChar(n);
                    remaining--;
                    small[i + remaining] = '\\';
                } break;
                default:
                    small[i + remaining] = n;
                    break;
                }
            }
            small[23] = (char)(23 - (sz + required_extra));
        }
        else {
            ensureOwnership();
            s32 sz = (s32)large.length;
            s32 remaining = (s32)required_extra;
            large.data[sz + remaining] = '\0';
            for (s32 i = (s32)sz - 1; i >= 0; i--) {
                char n = large.data[i];
                switch (n) {
                case '\n':
                case '\r':
                case '\t':
                case '\\':
                case '\'':
                case '"': {
                    large.data[i + remaining] = getEscapeChar(n);
                    remaining--;
                    large.data[i + remaining] = '\\';
                } break;
                default:
                    large.data[i + remaining] = n;
                    break;
                }
            }
            large.length += required_extra;
        }
    }

    void string::unescape() {
        if (isSmallString()) {
            u32 sz = 23 - small[23];
            for (u32 i = 0; i < sz; i++) {
                char n = small[i];
                if (n == '\\') {
                    memmove(small + i, small + i + 1, (size_t)sz - i);
                    sz--;
                    switch (small[i]) {
                    case 'n': {
                        small[i] = '\n';
                    } break;
                    case 'r': {
                        small[i] = '\r';
                    } break;
                    case 't': {
                        small[i] = '\t';
                    } break;
                    default: break;
                    }
                }
            }
            small[23] = (char)(23 - sz);
        }
        else {
            ensureOwnership();
            u32 sz = large.length;
            for (u32 i = 0; i < sz; i++) {
                char n = large.data[i];
                if (n == '\\') {
                    memmove(large.data + i, large.data + i + 1, (size_t)sz - i);
                    sz--;
                    switch (large.data[i]) {
                    case 'n': {
                        large.data[i] = '\n';
                    } break;
                    case 'r': {
                        large.data[i] = '\r';
                    } break;
                    case 't': {
                        large.data[i] = '\t';
                    } break;
                    default: break;
                    }
                }
            }
            large.length = sz;
        }
    }

    void string::set(u32 index, char c) {
        if (isSmallString()) {
            small[index] = c;
        }
        else {
            large.data[index] = c;
        }
    }

    char string::charAt(u32 index) const {
        if (isSmallString()) {
            return small[index];
        }
        else {
            return large.data[index];
        }
    }

    int string::compareTo(const string& s) const {
        u32 sz = size();
        u32 ssz = s.size();
        if (sz != ssz) {
            return sz < ssz ? 1 : -1;
        }
        if (isSmallString()) {
            for (u32 i = 0; i < sz; i++) {
                if (small[i] != s.small[i]) {
                    return small[i] < s.small[i] ? 1 : -1;
                }
            }
            return 0;
        }
        else {
            for (u32 i = 0; i < sz; i++) {
                if (large.data[i] != s.large.data[i]) {
                    return large.data[i] < s.large.data[i] ? 1 : -1;
                }
            }
            return 0;
        }
    }

    bool equals(const char* a, const char* b)
    {
        return strcmp(a, b) == 0;
    }

    bool startsWith(const char* str, const char* pre)
    {
        size_t slen = strlen(str);
        size_t len = strlen(pre);
        if (len > slen) return false;
        return strncmp(str, pre, len) == 0;
    }

    bool endsWith(const char* str, const char* suffix)
    {
        size_t str_len = strlen(str);
        size_t suffix_len = strlen(suffix);
        if (suffix_len > str_len)
            return false;
        s32 j = (s32)str_len - 1;
        for (s32 i = (s32)suffix_len - 1; i >= 0; i--)
        {
            if (str[j] != suffix[i])
                return false;
            j--;
        }
        return true;
    }

    bool contains(const char* str, char n)
    {
        size_t len = strlen(str);
        for (size_t i = 0; i < len; i++)
        {
            if (str[i] == n)
                return true;
        }
        return false;
    }

    char* dup(const char* s)
    {
        size_t len = strlen(s);
        char* n = new char[len + 1];
        memcpy(n, s, len);
        n[len] = '\0';
        return n;
    }

    bool isNumber(char c)
    {
        return c >= '0' && c <= '9';
    }

    bool isFloatChar(char c)
    {
        return (c >= '0' && c <= '9') || c == 'e' || c == 'E' || c == '-' || c == '+' || c == '.';
    }

    bool isHexChar(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    bool isLetter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    bool isIdentifier(const char* str)
    {
        size_t i = 0;
        char n = str[0];
        while (n != '\0' && (isLetter(n) || n == '_' || (isNumber(n) && i > 0)))
            n = str[++i];
        return n == '\0' && i > 0;
    }

    bool isInteger(const char* str)
    {
        size_t i = 0;
        char n = str[0];
        if (n == '-' || n == '+') n = *(++str);
        while (n != '\0')
        {
            if (n < '0' || n > '9')
                return false;
            n = str[++i];
        }
        return i > 0;
    }

    bool isFloat(const char* str)
    {
        size_t i = 0;
        char n = str[0];
        if (n == '-' || n == '+') n = *(++str);
        while (n != '\0' && n != '.' && n != 'e' && n != 'E')
        {
            if (n < '0' || n > '9')
                return false;
            n = str[++i];
        }
        if (n == '\0')
            return i > 0;
        if ((n == '.' || n == 'e' || n == 'E') && i == 0)
            return false;
        bool exponent = (n == 'e' || n == 'E');
        n = str[++i];
        if (exponent && (n == '-' || n == '+'))
            n = str[++i];
        size_t j = i;
        while (n != '\0' && n != 'e' && n != 'E')
        {
            if (n < '0' || n > '9')
                return false;
            n = str[++i];
        }
        if (n == '\0')
            return i > j;
        if (exponent)
            return false;
        n = str[++i];
        if (n == '-' || n == '+')
            n = str[++i];
        j = i;
        while (n != '\0')
        {
            if (n < '0' || n > '9')
                return false;
            n = str[++i];
        }
        return i > j;
    }

    char getHexChar(u8 v)
    {
        if (v < 10)
            return '0' + v;
        else if (v < 16)
            return 'A' + (v - 10);
        return '?';
    }

    char getEscapeChar(char c)
    {
        switch (c)
        {
        case '\n': return 'n';
        case '\r': return 'r';
        case '\t': return 't';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"': return '"';
        }
        return '?';
    }

    char toLower(char c)
    {
        if (c >= 'A' && c <= 'Z')
            return (c - 'A') + 'a';
        return c;
    }

    string toLower(const string& s)
    {
        string ret(s.size());
        for (u32 i = 0; i < s.size(); i++)
            ret.append(toLower(s.charAt(i)));
        return ret;
    }

    char toUpper(char c)
    {
        if (c >= 'a' && c <= 'z')
            return (c - 'a') + 'A';
        return c;
    }

}

sstring prettyTimeDelta(s64 delta_nanos)
{
    sstring result;
    if (delta_nanos < 0)
    {
        result.append("-");
        delta_nanos = -delta_nanos;
    }
    else
        result.append("+");
    static const char* suffixes[]{ "ns", "us", "ms" };
    // Sub-second times
    u64 acc = delta_nanos;
    for (int i = 0; i < 3; i++)
    {
        if (acc < 10 && i > 0)
        {
            u64 l = i == 1 ? delta_nanos : delta_nanos / 1000;
            double l0 = l / 1000.0;
            result.appendf("%.02f%s", l0, suffixes[i]);
            return result;
        }
        else if (acc < 100 && i > 0)
        {
            u64 l = i == 1 ? delta_nanos : delta_nanos / 1000;
            double l0 = l / 1000.0;
            result.appendf("%.01f%s", l0, suffixes[i]);
            return result;
        }
        else if (acc < 1000)
        {
            result.appendf("%d%s", acc, suffixes[i]);
            return result;
        }
        else
        {
            acc /= 1000;
        }
    }
    // seconds
    if (delta_nanos < 60 * 1000000000llu)
    {
        double secs = delta_nanos / 1e9;
        result.appendf("%.2fs", secs);
        return result;
    }
    u64 seconds = delta_nanos / 1000000000llu;
    double subseconds = (delta_nanos - seconds * 1e9) / 1e9;
    // minutes to hours
    if (seconds < 3600)
    {
        u64 m = seconds / 60;
        u64 s = seconds % 60;
        s32 ms = scalar::floori(subseconds * 1000 + 0.5); // + 0.5 to round
        result.appendf("%d:%02d.%03d", m, s, ms);
        return result;
    }
    else if (seconds < 3600llu * 24)
    {
        u64 h = seconds / 3600;
        seconds %= 3600;
        u64 m = seconds / 60;
        u64 s = seconds % 60;
        s32 ms = scalar::floori(subseconds * 1000 + 0.5); // + 0.5 to round
        result.appendf("%d:%02d:%02d.%03d", h, m, s, ms);
        return result;
    }
    // days
    u64 days = seconds / (3600llu * 24);
    result.appendf("%d days", days);
    return result;
}