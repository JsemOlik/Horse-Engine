#pragma once

#include "HorseEngine/Core.h"
#include <functional>
#include <future>

namespace Horse {

using JobFunction = std::function<void()>;

class JobSystem {
public:
    static void Initialize(u32 numThreads = 0); // 0 = auto-detect
    static void Shutdown();
    
    static void Execute(JobFunction&& job);
    
    template<typename Func>
    static auto ExecuteAsync(Func&& func) -> std::future<decltype(func())> {
        using ReturnType = decltype(func());
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<Func>(func));
        auto future = task->get_future();
        
        Execute([task]() { (*task)(); });
        
        return future;
    }
    
    static void WaitAll();
    static u32 GetThreadCount();
    
private:
    static class JobSystemImpl* s_Impl;
};

} // namespace Horse
