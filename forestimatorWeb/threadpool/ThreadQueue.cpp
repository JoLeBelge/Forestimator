#include "ThreadQueue.hpp"

void ThreadQueue::QueueElement::setNext(QueueElement *next) {
    this->next = next;
}

void ThreadQueue::QueueElement::setPrevious(QueueElement *previous) {
    this->previous = previous;
}

ThreadQueue::QueueElement* ThreadQueue::QueueElement::getNext(){
    return next;
}

ThreadQueue::QueueElement* ThreadQueue::QueueElement::getPrevious(){
    return previous;
}

Task* ThreadQueue::QueueElement::getTask(){
    return task;
}

void ThreadQueue::pushTask(Task* task){
    manipulateQueueMutex.lock();
    ThreadQueue::QueueElement* newE = new ThreadQueue::QueueElement(task);
    newE->setNext(nullptr);
    newE->setPrevious(this->tail);
    if(this->tail != nullptr){
        this->tail->setNext(newE);
    }
    this->tail = newE;
    if(this->head == nullptr){
        this->head = this->tail;
    }
    this->nTasks++;
    manipulateQueueMutex.unlock();
    return;
}

Task* ThreadQueue::popTaskHead(){
    manipulateQueueMutex.lock();
    if (this->head == nullptr){
        manipulateQueueMutex.unlock();
        return nullptr;
    }
    Task* task = this->head->getTask();
    QueueElement* oldE = this->head;
    if (oldE->getNext() != nullptr){
           this->head = oldE->getNext();
        this->head->setPrevious(nullptr);
    }
    else{
        this->head = nullptr;
        this->tail = nullptr;
    }
    this->nTasks--;
    delete(oldE);
    manipulateQueueMutex.unlock();
    return task;
}

Task* ThreadQueue::popStealTaskTail(){
    manipulateQueueMutex.lock();
    if (this->tail == nullptr){
        manipulateQueueMutex.unlock();
        return nullptr;
    }
    Task* task = this->tail->getTask();
    QueueElement* oldE = this->tail;
    if(oldE->getPrevious() != nullptr){
        this->tail = oldE->getPrevious();
        this->tail->setNext(nullptr);
    }
    else{
        this->head = nullptr;
        this->tail = nullptr;
    }
    this->nTasks--;
    delete(oldE);
    manipulateQueueMutex.unlock();
    return task;
}

int ThreadQueue::size(){
    return nTasks;
}