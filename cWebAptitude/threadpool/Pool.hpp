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
<<<<<<< HEAD
    int nThreads;
=======
    size_t nThreads;
>>>>>>> b6b7b9d8b6002d737fe630d0fc2e28adae0a78b3
    Task* mainTask;
    void waitForThreadsToFinish();
    int valid = 1;

public:
    cpu_set_t cpuset;
    static int ID;
<<<<<<< HEAD
    Pool(Task* mainTask, int nThreads) : mainTask(mainTask), nThreads(nThreads + MIN_THREADS){
=======
    Pool(Task* mainTask, size_t nThreads) : nThreads(nThreads + MIN_THREADS), mainTask(mainTask) {
>>>>>>> b6b7b9d8b6002d737fe630d0fc2e28adae0a78b3
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
<<<<<<< HEAD
        int* nThreads;
=======
        size_t* nThreads;
>>>>>>> b6b7b9d8b6002d737fe630d0fc2e28adae0a78b3
        int* valid;
        ThreadQueue* myTasks;

    public:
        std::promise<int> promise;
        std::thread* worker;
        std::future<int> finish;

        void run();
        void pushTask(Task* task);
        int empty();
<<<<<<< HEAD
        CoreThread(std::vector<CoreThread*>* cThreads, int* nThreads, int* valid) : threadID(ID++), myTasks(new ThreadQueue()), cThreads(cThreads), nThreads(nThreads), valid(valid){}
=======
        CoreThread(std::vector<CoreThread*>* cThreads, size_t* nThreads, int* valid) : threadID(ID++), cThreads(cThreads), nThreads(nThreads), valid(valid), myTasks(new ThreadQueue()){}
>>>>>>> b6b7b9d8b6002d737fe630d0fc2e28adae0a78b3
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
