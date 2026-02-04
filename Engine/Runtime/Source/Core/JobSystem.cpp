#include "HorseEngine/Core/JobSystem.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace Horse {

class JobSystemImpl {
public:
    explicit JobSystemImpl(u32 numThreads) {
        m_Threads.reserve(numThreads);
        for (u32 i = 0; i < numThreads; ++i) {
            m_Threads.emplace_back(&JobSystemImpl::WorkerThread, this);
        }
    }
    
    ~JobSystemImpl() {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Shutdown = true;
        }
        m_Condition.notify_all();
        
        for (auto& thread : m_Threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    
    void Execute(JobFunction&& job) {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_JobQueue.push(std::move(job));
            m_ActiveJobs++;
        }
        m_Condition.notify_one();
    }
    
    void WaitAll() {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_WaitCondition.wait(lock, [this] { return m_ActiveJobs == 0 && m_JobQueue.empty(); });
    }
    
    u32 GetThreadCount() const { return static_cast<u32>(m_Threads.size()); }
    
private:
    void WorkerThread() {
        while (true) {
            JobFunction job;
            
            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_Condition.wait(lock, [this] { return m_Shutdown || !m_JobQueue.empty(); });
                
                if (m_Shutdown && m_JobQueue.empty()) {
                    return;
                }
                
                if (!m_JobQueue.empty()) {
                    job = std::move(m_JobQueue.front());
                    m_JobQueue.pop();
                }
            }
            
            if (job) {
                job();
                
                {
                    std::unique_lock<std::mutex> lock(m_QueueMutex);
                    m_ActiveJobs--;
                    if (m_ActiveJobs == 0 && m_JobQueue.empty()) {
                        m_WaitCondition.notify_all();
                    }
                }
            }
        }
    }
    
    std::vector<std::thread> m_Threads;
    std::queue<JobFunction> m_JobQueue;
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    std::condition_variable m_WaitCondition;
    std::atomic<u32> m_ActiveJobs{0};
    bool m_Shutdown = false;
};

JobSystemImpl* JobSystem::s_Impl = nullptr;

void JobSystem::Initialize(u32 numThreads) {
    if (numThreads == 0) {
        numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
    }
    s_Impl = new JobSystemImpl(numThreads);
}

void JobSystem::Shutdown() {
    delete s_Impl;
    s_Impl = nullptr;
}

void JobSystem::Execute(JobFunction&& job) {
    if (s_Impl) {
        s_Impl->Execute(std::move(job));
    }
}

void JobSystem::WaitAll() {
    if (s_Impl) {
        s_Impl->WaitAll();
    }
}

u32 JobSystem::GetThreadCount() {
    return s_Impl ? s_Impl->GetThreadCount() : 0;
}

} // namespace Horse
