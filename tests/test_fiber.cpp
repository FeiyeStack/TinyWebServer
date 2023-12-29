#include "TinyWebServer/fiber.h"
#include "TinyWebServer/log.h"
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");
void runFiber(){
    SRV_LOG_DEBUG(g_logger)<<__func__<<" begin";
    WebSrv::Fiber::getThis()->back();
    SRV_LOG_DEBUG(g_logger)<<__func__;
    WebSrv::Fiber::getThis()->back();
    SRV_LOG_DEBUG(g_logger)<<__func__<<" end";
}

void testFiber(){
    SRV_LOG_DEBUG(g_logger)<<__func__<<" begin";
    {
        WebSrv::Fiber::getThis();
        SRV_LOG_DEBUG(g_logger)<<"main begin";
        WebSrv::Fiber::ptr fiber(new WebSrv::Fiber(runFiber));
        fiber->call();
        SRV_LOG_DEBUG(g_logger)<<"main resume";
        fiber->call();
        SRV_LOG_DEBUG(g_logger)<<"main end";
    }
    SRV_LOG_DEBUG(g_logger)<<__func__<<" end";
}

int main(){
    std::vector<std::thread> threads;
    for(int i=0;i<3;++i){
        threads.emplace_back(std::thread(&testFiber));
    }
    for(auto& i:threads){
        i.join();
    }
    return 0;
}
