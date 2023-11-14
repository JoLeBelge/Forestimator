#ifndef TP__ABSTRACT_TASK__TP
#define TP__ABSTRACT_TASK__TP
#include <mutex>

class Task{
protected:
    std::mutex taskMutexStats;
    static long ID;
    static long nTasksOpen;
    static long nTasksFinished;
private:
    long TaskID;

    //int finished = 0;
public:
    Task(){
        taskMutexStats.lock();
        TaskID = ID++;
        ++nTasksOpen;
        taskMutexStats.unlock();
    }
    virtual ~Task(){
        taskMutexStats.lock();
        --nTasksOpen;
        ++nTasksFinished;
        taskMutexStats.unlock();
    }
    long getID() {return TaskID;}
    virtual void run() {}

    static long getNTasksOpen(){return nTasksOpen;}
    static long getNTasksFinished(){return nTasksFinished;}
};
#endif
