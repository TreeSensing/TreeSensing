#include "util/redismodule.h"
#include "util/version.h"
///*
//#include "cm/rm_cm.h"
//#include "cs/rm_cs.h"
//*/
///*
#include "basic_sketch/basic_sketch.h"
#include "basic_cm/basic_cm.h"
//*/
//#include "test/rm_test.h"
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
    {
        if (RedisModule_Init(ctx, "RedisSketches", SKETCHES_MODULE_VERSION, REDISMODULE_APIVER_1) !=
            REDISMODULE_OK)
        {
            return REDISMODULE_ERR;
        }
        ///*
        //CMModule_onLoad(ctx, argv, argc);


        //*/

        Basic_Sketch_Module_onLoad<basic_cm>(ctx, argv, argc);

        // TestModule_onLoad(ctx, argv, argc);

        return REDISMODULE_OK;
    }

#ifdef __cplusplus
}
#endif