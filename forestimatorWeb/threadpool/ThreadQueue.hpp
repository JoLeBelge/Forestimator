#ifndef TP__THREAD_QUEUE__TP
#define TP__THREAD_QUEUE__TP

#include <mutex>
#include <iostream>

#include "Task.hpp"

struct ThreadQueue
{
private:
    struct QueueElement
    {
    private:
        QueueElement *next;
        QueueElement *previous;
        Task *task;

    public:
        QueueElement(Task *task) : task(task) {}
        void setNext(QueueElement *next);
        void setPrevious(QueueElement *previous);
        QueueElement *getNext();
        QueueElement *getPrevious();
        Task *getTask();
    };
    QueueElement *head = nullptr;
    QueueElement *tail = nullptr;
    int nTasks = 0;
    std::mutex manipulateQueueMutex;

public:
    void pushTask(Task *task);
    Task *popTaskHead();
    Task *popStealTaskTail();
    int size();
};
#endif