#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

#include "util/my_string.h"

#define REDIS_MODULE_TARGET
#ifdef REDIS_MODULE_TARGET
#include "util/redismodule.h"
#define CALLOC(count, size) RedisModule_Calloc(count, size)
#define FREE(ptr) RedisModule_Free(ptr)
#else
#define CALLOC(count, size) calloc(count, size)
#define FREE(ptr) free(ptr)
#endif

#define HASH(key, keylen, i) MurmurHash2(key, keylen, i)

class hash_table_node
{
public:
    my_string key;
    int value;
    hash_table_node *next;

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
};

class hash_table
{
public:
    int mod;
    hash_table_node **h;
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
    hash_table(int x)
    {
        mod = x;
        h = (hash_table_node **)CALLOC(x, sizeof(hash_table_node *));
    }
    ~hash_table()
    {
        for (int i = 0; i < mod; ++i)
            for (hash_table_node *it = h[i]; it;)
            {
                hash_table_node *tmp = it->next;
                delete it;
                it = tmp;
            }
        FREE(h);
    }
    void insert(const my_string &key, int value)
    {
        int pos = HASH(key.c_str(), key.len(), 1) % mod;
        for (hash_table_node *it = h[pos]; it; it = it->next)
        {
            if (it->key == key)
            {
                it->value = value;
                return;
            }
        }
        hash_table_node *tmp = (hash_table_node *)CALLOC(1, sizeof(hash_table_node));
        tmp->key = key;
        tmp->value = value;
        tmp->next = h[pos];
        h[pos] = tmp;
    }
    void update(const my_string &key, int value)
    {
        int pos = HASH(key.c_str(), key.len(), 1) % mod;
        for (hash_table_node *it = h[pos]; it; it = it->next)
        {
            if (it->key == key)
            {
                it->value += value;
                return;
            }
        }
        hash_table_node *tmp = (hash_table_node *)CALLOC(1, sizeof(hash_table_node));
        tmp->key = key;
        tmp->value = value;
        tmp->next = h[pos];
        h[pos] = tmp;
    }
    void reply(RedisModuleCtx *ctx)
    {
        long result_num = 0;
        for (int i = 0; i < mod; ++i)
            for (hash_table_node *it = h[i]; it; it = it->next)
                ++result_num;
        RedisModule_ReplyWithArray(ctx, result_num * 2);
        for (int i = 0; i < mod; ++i)
            for (hash_table_node *it = h[i]; it; it = it->next)
            {
                RedisModule_ReplyWithStringBuffer(ctx, it->key.c_str(), it->key.len());
                RedisModule_ReplyWithLongLong(ctx, it->value);
            }
    }
    void save(RedisModuleIO *io)
    {
        long result_num = 0;
        for (int i = 0; i < mod; ++i)
            for (hash_table_node *it = h[i]; it; it = it->next)
                ++result_num;
        RedisModule_SaveSigned(io, result_num);
        for (int i = 0; i < mod; ++i)
            for (hash_table_node *it = h[i]; it; it = it->next)
            {
                RedisModule_SaveUnsigned(io, it->key.len());
                RedisModule_SaveStringBuffer(io, it->key.c_str(), it->key.len());
                RedisModule_SaveSigned(io, it->value);
            }
    }
    void load(RedisModuleIO *io)
    {
        int result_num = RedisModule_LoadSigned(io);
        for (int i = 0; i < result_num; ++i)
        {
            my_string s;
            s.s = RedisModule_LoadStringBuffer(io, &s.length);
            int value = RedisModule_LoadSigned(io);
            insert(s, value);
        }
    }
};

#endif