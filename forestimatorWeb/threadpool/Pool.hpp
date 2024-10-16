#ifndef TP__POOL__TP
#define TP__POOL__TP
#include <thread>
#include <chrono>
#include <vector>
#include <future>

#include "Task.hpp"
#include "ThreadQueue.hpp"

static const int MIN_THREADS = 1;

struct Pool{
private:
    int nThreads;
    Task* mainTask;
    void waitForThreadsToFinish();
    int valid = 1;

public:
    cpu_set_t cpuset;
    static int ID;
    Pool(Task* mainTask, int nThreads) : mainTask(mainTask), nThreads(nThreads + MIN_THREADS){
        for (size_t j = 0; j < nThreads; j++)
            CPU_SET(j, &cpuset);
    }
    ~Pool() {
        this->clean();
    }
    void add(Task* task);
    void start();
    void waitOnWorkerToFinished();
    void waitForMainThreadToFinish();
    int workerFinished();
    void end();
    void suspendWork();
    void continueWork();
    void clean();
    void getUnfinishdTasks(int* ls);
    long getNunfinishedTasks();
    std::string getThreadPoolInfo();

    struct CoreThread{
    private:
        int threadID;
        int idle = 1;
        int working = 0;
        std::vector<CoreThread*>* cThreads;
        int* nThreads;
        int* valid;
        ThreadQueue* myTasks;

    public:
        std::promise<int> promise;
        std::thread* worker;
        std::future<int> finish;

        void run();
        void pushTask(Task* task);
        int empty();
        CoreThread(std::vector<CoreThread*>* cThreads, int* nThreads, int* valid) : threadID(ID++), myTasks(new ThreadQueue()), cThreads(cThreads), nThreads(nThreads), valid(valid){}
        int getUnfinishdTasks();
        void startWork();
        void suspendWork();
        void clean();
        int getID();
        int sizeQueue();

    };

    std::vector<Pool::CoreThread*> cThreads;
};

#endif
