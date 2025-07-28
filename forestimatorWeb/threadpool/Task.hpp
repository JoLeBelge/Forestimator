#ifndef TP__ABSTRACT_TASK__TP
#define TP__ABSTRACT_TASK__TP
#include <mutex>
#include <functional>

class Task
{
protected:
    std::mutex taskMutexStats;
    static long ID;
    static long nTasksOpen;
    static long nTasksFinished;

private:
    long TaskID;
    std::function<void(void)> callbackAfter = nullptr;

public:
    Task()
    {
        taskMutexStats.lock();
        TaskID = ID++;
        ++nTasksOpen;
        taskMutexStats.unlock();
    }
    virtual ~Task()
    {
        taskMutexStats.lock();
        --nTasksOpen;
        ++nTasksFinished;
        taskMutexStats.unlock();
    }
    long getID() { return TaskID; }
    virtual void run() {}

    void setCallbackAfter(std::function<void(void)>callback)
    {
        callbackAfter = callback;
        
    }
    
    void whenFinished()
    {
        if (callbackAfter != nullptr)
            callbackAfter();
    }

    static long getNTasksOpen() { return nTasksOpen; }
    static long getNTasksFinished() { return nTasksFinished; }
};
#endif
