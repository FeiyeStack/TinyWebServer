#pragma once

#include <cassert>
#include "log.h"

//断言，当它为false执行
#define WebSrvAssert(x)\
    if(!(x)){\
        SRV_LOG_DEBUG(SRV_LOGGER_ROOT())<<"Assertion: " #x " "\
        ;\
        assert(x);\
    }

//断言，当它为false执行
#define WebSrvAssert2(x,msg)\
    if(!(x)){\
        SRV_LOG_DEBUG(SRV_LOGGER_ROOT())<<"Assertion: " #x " "\
        << msg;\
        assert(x);\
    }
