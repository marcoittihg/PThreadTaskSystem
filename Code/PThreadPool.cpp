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
    static pthread_mutex_t idMutex1 = PTHREAD_MUTEX_INITIALIZER;
    static int idCont1 = 0;

    pthread_mutex_lock(&idMutex1);
    semName = "newFunctionSemaphore" + std::to_string(idCont1++);
    pthread_mutex_unlock(&idMutex1);

    newFunctionSemaphore = sem_open(semName.c_str(), O_CREAT, 0644, 0);
    sem_unlink(semName.c_str());

    pthread_create(&workerPthread, NULL, pthreadWorkerLoop, this);
}

PThreadPool::WorkerPThread::~WorkerPThread(){
    pthread_cancel(workerPthread);

    sem_close(newFunctionSemaphore);
}


PThreadPool::PThreadPool() : PThreadPool(std::thread::hardware_concurrency()){}

PThreadPool::PThreadPool( unsigned int numWorkerThreads ) : numWorkerThreads(numWorkerThreads) {
    static pthread_mutex_t idMutex2 = PTHREAD_MUTEX_INITIALIZER;
    static int idCont2 = 0;

    pthread_mutex_lock(&idMutex2);
    semName = "poolSemaphore" + std::to_string(idCont2++);
    pthread_mutex_unlock(&idMutex2);

    poolSemaphore = sem_open(semName.c_str(), O_CREAT , 0644, numWorkerThreads);
    sem_unlink(semName.c_str());

    queueMutex = PTHREAD_MUTEX_INITIALIZER;

    workers = new WorkerPThread*[numWorkerThreads];

    for (int i = 0; i < numWorkerThreads; ++i) {
        WorkerPThread* newWorker = new WorkerPThread(this);
        workers[i] = newWorker;

        pushReadyQueue(newWorker);
    }
}


PThreadPool::~PThreadPool() {
    if(workers == nullptr) return;

    for (int i = 0; i < numWorkerThreads; ++i) {
        delete workers[i];
        workers[i] = nullptr;
    }

    delete[] workers;
    workers = nullptr;


    sem_close(poolSemaphore);
}


