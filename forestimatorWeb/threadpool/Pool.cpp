#include "Pool.hpp"
int Pool::ID = 0;

static void launchWorker(Pool::CoreThread *tr)
{
    tr->run();
    tr->promise.set_value_at_thread_exit(0);
    return;
}
/*----------------------------------------------------------------*/
/*                   Pool::                                       */
/*----------------------------------------------------------------*/
void Pool::add(Task *task)
{
    cThreads[1 + std::rand() % (nThreads - 1)]->pushTask(task);
}

void Pool::waitForThreadsToFinish()
{
        for (int i = 1; i < nThreads; i++)
    {
        cThreads[i]->finish.wait();
    }
    return;
}

void Pool::waitForMainThreadToFinish()
{
    cThreads[0]->finish.wait();
}

void Pool::start()
{
        for (int i = 0; i < nThreads; i++)
    {
        CoreThread *that = new CoreThread(&cThreads, &nThreads, &valid);
        cThreads.push_back(that);
        that->finish = that->promise.get_future();
        std::thread(launchWorker, that).detach();
    }
        for (int i = 0; i < nThreads; i++)
    {
        cThreads[i]->startWork();
    }
    cThreads[0]->pushTask(mainTask);
    Pool::waitForThreadsToFinish();
    std::cout << "Here";
    Pool::waitForMainThreadToFinish();
    std::cout << "nowhere";
}

void Pool::waitOnWorkerToFinished()
{
    while (!workerFinished())
    {
    }
    return;
}

int Pool::workerFinished()
{
        for (int i = 1; i < nThreads; i++)
    {
        CoreThread *cT = cThreads[i];
        if (!cT->empty())
            return 0;
    }
    return 1;
}

void Pool::end()
{
    for (int i = 0; i < nThreads; i++)
    {
        cThreads[i]->suspendWork();
    }
    valid = 0;
}

void Pool::suspendWork()
{
        for (int i = 1; i < nThreads; i++)
    {
        cThreads[i]->suspendWork();
    }
}

void Pool::continueWork()
{
        for (int i = 1; i < nThreads; i++)
    {
        cThreads[i]->startWork();
    }
}

void Pool::clean()
{
        for (int i = 0; i < nThreads; i++)
    {
        cThreads[i]->clean();
    }
    for (int i = 0; i < nThreads; i++)
    {
        delete (cThreads[i]);
        cThreads[i] = nullptr;
    }
}

void Pool::getUnfinishdTasks(int *ls)
{
        for (int i = 0; i < nThreads; i++)
    {
        ls[i] = cThreads[i]->getUnfinishdTasks();
    }
    return;
}

long Pool::getNunfinishedTasks()
{
    int sum = 0;
    for (auto &tr : cThreads)
    {
        sum += tr->sizeQueue();
    }
    return sum;
}

std::string Pool::getThreadPoolInfo()
{
    std::string pretty = "";
    pretty.append("Number of running threads: ");
    pretty.append(std::to_string(cThreads.size()));
    pretty.append("\n");
    pretty.append("Number of active Tasks: ");
    pretty.append(std::to_string(Task::getNTasksOpen()));
    pretty.append("\n");
    pretty.append("Number of finished Tasks: ");
    pretty.append(std::to_string(Task::getNTasksFinished()));
    pretty.append("\n");
    for (auto &thread : cThreads)
    {
        pretty.append("Thread: ");
        pretty.append(std::to_string(thread->getID()));
        pretty.append(" -> ");
        pretty.append("N° Tasks in queue: ");
        pretty.append(std::to_string(thread->sizeQueue()));
        pretty.append("\n");
    }
    return pretty;
}
/*----------------------------------------------------------------*/
/*                   CoreThread::                                 */
/*----------------------------------------------------------------*/
void Pool::CoreThread::run()
{
    while (*valid)
    {
        while (working)
        {
            Task *task = this->myTasks->popTaskHead();
            if (task != nullptr)
            {
                this->idle = 0;
                task->run();
                task->whenFinished();
                delete (task);
            }
            else
            {
                this->idle = 1;
                CoreThread *thisVictim = (*cThreads)[std::rand() % (*nThreads)];
                int victimsTasksToDo = thisVictim->myTasks->size();
                if (victimsTasksToDo / 2 > 1)
                {
                    while (victimsTasksToDo / 2 < thisVictim->myTasks->size())
                    {
                        Task *stolenTask = thisVictim->myTasks->popStealTaskTail();
                        this->myTasks->pushTask(stolenTask);
                    }
                    std::cout << "I stole tasks" << this->threadID << std::endl;
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        }
    }
}

void Pool::CoreThread::pushTask(Task *task)
{
    this->myTasks->pushTask(task);
    this->idle = 0;
}

int Pool::CoreThread::empty()
{
    if (myTasks->size() == 0 && idle)
        return 1;
    return 0;
}

int Pool::CoreThread::getUnfinishdTasks()
{
    return this->myTasks->size();
}

void Pool::CoreThread::startWork()
{
    this->working = 1;
}

void Pool::CoreThread::suspendWork()
{
    this->working = 0;
}

void Pool::CoreThread::clean()
{
    std::cout << "destroying core in memory\n";
    Task *task = this->myTasks->popTaskHead();
    while (task != nullptr)
    {
        delete (task);
        task = this->myTasks->popTaskHead();
    }
    delete (myTasks);
    myTasks = nullptr;
}

int Pool::CoreThread::getID()
{
    return threadID;
}

int Pool::CoreThread::sizeQueue()
{
    return myTasks->size();
}
