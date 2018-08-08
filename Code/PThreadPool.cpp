//
// Created by Marco on 28/07/18.
//

#include "PThreadPool.h"

void *PThreadPool::WorkerPThread::pthreadWorkerLoop(void* args) {
    WorkerPThread* worker = (WorkerPThread*) args;

    while(true){
        //Wait for a new function
        sem_wait( worker->newFunctionSemaphore );

        //Execute the new function
        worker->func(worker->funcArgs);

        //Call the callback if specified
        if(worker->callback != nullptr)
            worker->callback(worker->callbackArgs);

        //Set this pthread as ready
        worker->ownerPool->pushReadyQueue(worker);
        sem_post(worker->ownerPool->poolSemaphore);
    }

    return nullptr;
}

PThreadPool::WorkerPThread::WorkerPThread(PThreadPool *ownerPool) : ownerPool(ownerPool) {
    newFunctionSemaphore = sem_open("newFunctionSemaphore", O_CREAT, 0644, 0);
    sem_unlink("newFunctionSemaphore");

    pthread_create(&workerPthread, NULL, pthreadWorkerLoop, this);
}

PThreadPool::WorkerPThread::~WorkerPThread(){
    pthread_cancel(workerPthread);

    sem_close(newFunctionSemaphore);
}


PThreadPool::PThreadPool() : PThreadPool(std::thread::hardware_concurrency()){}

PThreadPool::PThreadPool( unsigned int numWorkerThreads ) : numWorkerThreads(numWorkerThreads) {
    poolSemaphore = sem_open("poolSemaphore", O_CREAT , 0644, numWorkerThreads);
    sem_unlink("poolSemaphore");

    queueMutex = PTHREAD_MUTEX_INITIALIZER;

    workers = new WorkerPThread*[numWorkerThreads];

    for (int i = 0; i < numWorkerThreads; ++i) {
        WorkerPThread* newWorker = new WorkerPThread(this);
        workers[i] = newWorker;

        pushReadyQueue(newWorker);
    }
}


PThreadPool::~PThreadPool() {
    for (int i = 0; i < numWorkerThreads; ++i) {
        delete workers[i];
    }

    delete[] workers;


    sem_close(poolSemaphore);
}


