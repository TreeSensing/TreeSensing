// the header file that defines the basic sketch structure and operations
#ifndef _BASIC_SKETCH_H_
#define _BASIC_SKETCH_H_

#include <assert.h>
#include <string.h>
#include <type_traits>

#ifdef __cplusplus
extern "C" {
#endif
#include "util.h"
#ifdef __cplusplus
};
#endif

#include "util/MurmurHash2.h"

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

//basic class for the sketch_string: replace the string class in std
class basic_sketch_string {
protected:
  size_t length;
  const char *s;
  bool flag;

public:
  void *operator new(size_t size) { return CALLOC(1, size); }
  void *operator new[](size_t size) { return CALLOC(1, size); }
  void operator delete(void *ptr) { FREE(ptr); }
  void operator delete[](void *ptr) { FREE(ptr); }
  basic_sketch_string() {
    length = 0;
    s = NULL;
    flag = true;
  }
  basic_sketch_string(const basic_sketch_string &t) {
    flag = true;
    length = t.length;
    char *tmp = (char *)CALLOC(length + 1, sizeof(char));
    memcpy(tmp, t.s, length);
    s = tmp;
  }
  basic_sketch_string(const char *_s, size_t len = 0) {
    flag = true;
    if (len == 0)
      length = strlen(_s);
    else
      length = len;
    char *tmp = (char *)CALLOC(length + 1, sizeof(char));
    memcpy(tmp, _s, length);
    s = tmp;
  }
  ~basic_sketch_string() {
    if (s && flag)
      FREE(const_cast<char *>(s));
  }

  //compare two string: overload the '==' 
  bool operator==(const basic_sketch_string &t) const {
    if (length != t.length)
      return false;
    for (size_t i = 0; i < length; ++i)
      if (s[i] != t.s[i])
        return false;
    return true;
  }

  //combine two string: overload the "+"
  basic_sketch_string operator+(const basic_sketch_string &t) const {
    basic_sketch_string result;
    result.flag = true;
    result.length = length + t.length;
    char *tmp = (char *)CALLOC(result.length + 1, sizeof(char));
    memcpy(tmp, s, length);
    memcpy(tmp + length, t.s, t.length);
    result.s = tmp;
    return result;
  }

  //assignment one string to the other: overload the '='
  void operator=(const basic_sketch_string &t) {
    if (s && flag)
      FREE(const_cast<char *>(s));
    flag = true;
    length = t.length;
    char *tmp = (char *)CALLOC(length + 1, sizeof(char));
    memcpy(tmp, t.s, length);
    s = tmp;
  }
  void change(const char *_s, size_t len = 0) {
    if (s && flag)
      FREE(const_cast<char *>(s));
    flag = false;
    if (len == 0)
      length = strlen(_s);
    else
      length = len;
    s = _s;
  }
  size_t len() const { return length; }
  const char *c_str() const { return s; }
  const int to_int() { return atoi(s); }
  long long to_long_long() { return atoll(s); }
  double to_double() { return atof(s); }
};

//basic entity class for the reply of a sketch
class basic_sketch_reply {
protected:
  size_t cnt;
  size_t limit;
  enum basic_sketch_reply_type { type_long_long, type_double, type_string };
  basic_sketch_reply_type *types;
  union reply_union {
    long long l;
    double d;
    basic_sketch_string *s;
  } * replys;

public:
  void *operator new(size_t size) { return CALLOC(1, size); }
  void *operator new[](size_t size) { return CALLOC(1, size); }
  void operator delete(void *ptr) { FREE(ptr); }
  void operator delete[](void *ptr) { FREE(ptr); }
  basic_sketch_reply() {
    cnt = 0;
    limit = 5;
    types =
        (basic_sketch_reply_type *)CALLOC(5, sizeof(basic_sketch_reply_type));
    replys = (reply_union *)CALLOC(5, sizeof(reply_union));
  }
  ~basic_sketch_reply() {
    for (size_t i = 0; i < cnt; ++i)
      if (types[i] == type_string)
        delete replys[i].s;
    FREE(types);
    FREE(replys);
  }
  //adjust the size of reply structure
  void adjust() {
    if (cnt + 1 < limit)
      return;

    limit *= 2;
    basic_sketch_reply_type *tmp_types = (basic_sketch_reply_type *)CALLOC(
        limit, sizeof(basic_sketch_reply_type));
    for (size_t i = 0; i < cnt; ++i)
      tmp_types[i] = types[i];
    FREE(types);
    types = tmp_types;
    reply_union *tmp_replys = (reply_union *)CALLOC(limit, sizeof(reply_union));
    for (size_t i = 0; i < cnt; ++i)
      tmp_replys[i] = replys[i];
    FREE(replys);
    replys = tmp_replys;
  }

  //push the result into the reply array
  void push_back(const long long &l) {
    adjust();
    types[cnt] = type_long_long;
    replys[cnt].l = l;
    ++cnt;
  }
  void push_back(const double &d) {
    adjust();
    types[cnt] = type_double;
    replys[cnt].d = d;
    ++cnt;
  }
  void push_back(const basic_sketch_string &s) {
    adjust();
    types[cnt] = type_string;
    replys[cnt].s = new basic_sketch_string(s);
    ++cnt;
  }

  //reply the result to Redis cli with redis module
  void reply(RedisModuleCtx *ctx) {
    RedisModule_ReplyWithArray(ctx, (long)cnt);
    for (size_t i = 0; i < cnt; ++i) {
      switch (types[i]) {
      case type_long_long:
        RedisModule_ReplyWithLongLong(ctx, replys[i].l);
        break;
      case type_double:
        RedisModule_ReplyWithDouble(ctx, replys[i].d);
        break;
      case type_string:
        RedisModule_ReplyWithCString(ctx, replys[i].s->c_str());
        break;
      default:
        RedisModule_ReplyWithError(ctx, "wrong type reply");
        break;
      }
    }
    for (size_t i = 0; i < cnt; ++i) {
      if (types[i] == type_string) {
        delete replys[i].s;
      }
      memset(&replys[i], 0, sizeof(reply_union));
    }
    cnt = 0;
  }
};

typedef basic_sketch_reply *(*basic_sketch_func)(
    void *o, const int &argc, const basic_sketch_string *argv);

//basic class for the sketch
//overload the new and delete 
class basic_sketch {
public:
  void *operator new(size_t size) { return CALLOC(1, size); }
  void *operator new[](size_t size) { return CALLOC(1, size); }
  void operator delete(void *ptr) { FREE(ptr); }
  void operator delete[](void *ptr) { FREE(ptr); }

  basic_sketch() {}
  basic_sketch(int argc, basic_sketch_string *argv) {}
  basic_sketch(const basic_sketch_string &s) {}

  basic_sketch_string *to_string() { return new basic_sketch_string; }
  ~basic_sketch() {}

  static int command_num() { return 0; }
  static basic_sketch_string command_name(int index) {
    return basic_sketch_string();
  }
  static basic_sketch_func command(int index) { return NULL; }
  static basic_sketch_string class_name() { return "basic_sketch"; }
  static int command_type(int index) { return 0; }
  static char *type_name() { return "SKETCHTYP"; }
};

#define ERROR(x)                                                               \
  RedisModule_ReplyWithError(ctx, x);                                          \
  return REDISMODULE_ERR;

static RedisModuleType *SKETCHType1;

template <class T>
static int GetKey(RedisModuleCtx *ctx, RedisModuleString *keyName, T **sketch,
                  int mode) {
  static_assert(std::is_base_of<basic_sketch, T>::value, "type error!");

  RedisModuleKey *key =
      (RedisModuleKey *)RedisModule_OpenKey(ctx, keyName, mode);
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
    RedisModule_CloseKey(key);
    ERROR("key does not exist");
  } else if (RedisModule_ModuleTypeGetType(key) != SKETCHType1) {
    RedisModule_CloseKey(key);
    ERROR(REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  *sketch = (T *)RedisModule_ModuleTypeGetValue(key);
  RedisModule_CloseKey(key);
  return REDISMODULE_OK;
}

static basic_sketch_string *convert_argv(RedisModuleString **argv, int argc) {
  basic_sketch_string *result = new basic_sketch_string[argc];
  for (int i = 0; i < argc; ++i) {
    size_t len;
    const char *s = RedisModule_StringPtrLen(argv[i], &len);
    result[i].change(s, len);
  }
  return result;
}

//Create commands with Redis module
template <class T>
static int Create_Cmd(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  static_assert(std::is_base_of<basic_sketch, T>::value, "type error!");

  if (argc < 2) {
    return RedisModule_WrongArity(ctx);
  }

  RedisModuleKey *key = (RedisModuleKey *)RedisModule_OpenKey(
      ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  basic_sketch_string *argv_new = convert_argv(argv + 2, argc - 2);
  T *sketch = new T(argc - 2, argv_new);
  delete[] argv_new;

  if (RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_EMPTY) {
    RedisModule_ReplyWithError(ctx, "key already exists");
    goto final;
  }

  if (RedisModule_ModuleTypeSetValue(key, SKETCHType1, sketch) ==
      REDISMODULE_ERR) {
    goto final;
  }

  RedisModule_ReplicateVerbatim(ctx);
  RedisModule_ReplyWithSimpleString(ctx, "OK");
final:
  RedisModule_CloseKey(key);
  return REDISMODULE_OK;
}

template <class T, int func_index>
static int Command_Cmd(RedisModuleCtx *ctx, RedisModuleString **argv,
                       int argc) {
  static_assert(std::is_base_of<basic_sketch, T>::value, "type error!");

  if (argc < 2)
    return RedisModule_WrongArity(ctx);

  T *sketch;
  if (GetKey(ctx, argv[1], &sketch, REDISMODULE_READ | REDISMODULE_WRITE) !=
      REDISMODULE_OK) {
    return REDISMODULE_OK;
  }

  basic_sketch_string *argv_new = convert_argv(argv + 2, argc - 2);
  basic_sketch_reply *result =
      T::command(func_index)(sketch, argc - 2, argv_new);
  result->reply(ctx);
  delete result;
  delete[] argv_new;

  if (T::command_type(func_index) == 0)
    RedisModule_ReplicateVerbatim(ctx);
  return REDISMODULE_OK;
}


//persistent with Rdb: save the basic_string returned by the to_string in basic_cm to disk 
template <class T> static void RdbSave(RedisModuleIO *io, void *obj) {
  static_assert(std::is_base_of<basic_sketch, T>::value, "type error!");

  T *sketch = (T *)obj;
  basic_sketch_string *s = sketch->to_string();
  RedisModule_SaveStringBuffer(io, s->c_str(), s->len() * sizeof(char));
  delete s;
}

//read the persistent data in disk and construct template class T
template <class T> static void *RdbLoad(RedisModuleIO *io, int encver) {
  static_assert(std::is_base_of<basic_sketch, T>::value, "type error!");

  if (encver > 0) {
    return NULL;
  }

  size_t len;
  char *s = RedisModule_LoadStringBuffer(io, &len);
  T *sketch = new T(basic_sketch_string(s, len));

  return sketch;
}

template <class T> static void Free(void *value) {
  static_assert(std::is_base_of<basic_sketch, T>::value, "type error!");

  T *sketch = (T *)value;
  delete sketch;
}

template <class T, int x> int add_command1(RedisModuleCtx *ctx) {
  static_assert(std::is_base_of<basic_sketch, T>::value, "type error!");

  if (x <= T::command_num()) {
    // fprintf(stderr, "%s\n", (T::class_name() + basic_sketch_string(".") +
    // T::command_name(x)).c_str());
    switch (T::command_type(x)) {
    case 0:
      RMUtil_RegisterWriteCmd(
          ctx,
          (T::class_name() + basic_sketch_string(".") + T::command_name(x))
              .c_str(),
          (Command_Cmd<T, x>));
      break;
    case 1:
      RMUtil_RegisterReadCmd(
          ctx,
          (T::class_name() + basic_sketch_string(".") + T::command_name(x))
              .c_str(),
          (Command_Cmd<T, x>));
      break;
    default:
      break;
    }
    return REDISMODULE_OK;
  } else {
    return REDISMODULE_OK;
  }
}

//load module with Redis module
template <class T>
int Basic_Sketch_Module_onLoad(RedisModuleCtx *ctx, RedisModuleString **argv,
                               int argc) {
  static_assert(std::is_base_of<basic_sketch, T>::value, "type error!");

  RedisModuleTypeMethods tm = {.version = REDISMODULE_TYPE_METHOD_VERSION,
                               .rdb_load = RdbLoad<T>,
                               .rdb_save = RdbSave<T>,
                               .aof_rewrite = RMUtil_DefaultAofRewrite,
                               .free = Free<T>};

  SKETCHType1 = RedisModule_CreateDataType(ctx, T::type_name(), 0, &tm);
  if (SKETCHType1 == NULL)
    return REDISMODULE_ERR;

  RMUtil_RegisterWriteCmd(
      ctx, (T::class_name() + basic_sketch_string(".create")).c_str(),
      (Create_Cmd<T>));

  add_command1<T, 0>(ctx);
  add_command1<T, 1>(ctx);
  add_command1<T, 2>(ctx);
  add_command1<T, 3>(ctx);
  add_command1<T, 4>(ctx);
  add_command1<T, 5>(ctx);
  add_command1<T, 6>(ctx);
  add_command1<T, 7>(ctx);
  add_command1<T, 8>(ctx);
  add_command1<T, 9>(ctx);

  return REDISMODULE_OK;
}

#endif