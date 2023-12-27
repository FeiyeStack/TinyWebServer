#include "TinyWebServer/scheduler.h"
#include <thread>
#include <iostream>
#include "log.h"
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");

void testFiber() {
    static int s_count = 5;
    SRV_LOG_DEBUG(g_logger) << "test in fiber s_count=" << s_count;

    std::this_thread::sleep_for(std::chrono::seconds(1));
    if(--s_count >= 0) {
        WebSrv::Scheduler::getThis()->schedule(&testFiber, std::this_thread::get_id());
    }
}

int main(){
    SRV_LOG_DEBUG(g_logger)<<__func__;
    WebSrv::Scheduler sc(3,false);
    sc.start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    SRV_LOG_DEBUG(g_logger)<<"scheduler";
    sc.schedule(&testFiber);
    sc.stop();
    SRV_LOG_DEBUG(g_logger)<<"end";

}