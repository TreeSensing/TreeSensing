#ifndef _MY_STRING_H_
#define _MY_STRING_H_

#include <cstring>

#define REDIS_MODULE_TARGET
#ifdef REDIS_MODULE_TARGET
#include "util/redismodule.h"
#define CALLOC(count, size) RedisModule_Calloc(count, size)
#define FREE(ptr) RedisModule_Free(ptr)
#else
#define CALLOC(count, size) calloc(count, size)
#define FREE(ptr) free(ptr)
#endif

class my_string
{
public:
    size_t length;
    char *s;
    void *operator new(size_t size)
    {
        return CALLOC(1, size);
    }
    void *operator new[](size_t size)
    {
        return CALLOC(1, size);
    }
    void operator delete(void *ptr)
    {
        FREE(ptr);
    }
    void operator delete[](void *ptr)
    {
        FREE(ptr);
    }
    my_string()
    {
        length = 0;
        s = NULL;
    }
    my_string(const my_string &t)
    {
        length = t.length;
        s = (char *)CALLOC(length + 1, sizeof(char));
        memcpy(s, t.s, length);
    }
    my_string(const char *_s, size_t len = 0)
    {
        if (len == 0)
            length = strlen(_s);
        else
            length = len;
        s = (char *)CALLOC(length + 1, sizeof(char));
        memcpy(s, _s, length);
    }
    ~my_string() { FREE(s); }
    bool operator==(const my_string &t) const
    {
        if (length != t.length)
            return false;
        for (size_t i = 0; i < length; ++i)
            if (s[i] != t.s[i])
                return false;
        return true;
    }
    my_string operator+(const my_string &t) const
    {
        my_string result;
        result.length = length + t.length;
        result.s = (char *)CALLOC(result.length + 1, sizeof(char));
        memcpy(result.s, s, length);
        memcpy(result.s + length, t.s, t.length);
        return result;
    }
    void operator=(const my_string &t)
    {
        FREE(s);
        length = t.length;
        s = (char *)CALLOC(length + 1, sizeof(char));
        memcpy(s, t.s, length);
    }
    size_t len() const { return length; }
    const char *c_str() const { return s; }
    int to_int() { return atoi(s); }
    long long to_long_long() { return atoll(s); }
    double to_double() { return atof(s); }
};

#endif