//
// Created by Marco on 28/07/18.
//

#ifndef CODE_PTHREADPOOL_H
#define CODE_PTHREADPOOL_H

#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <thread>

/**
 * Pool of PThread workers
 */
class PThreadPool {
private:
    /**
     * PThread worker, execute one function at a time with the managed pthread
     */
    class WorkerPThread{
    private:
        /**
         * The function to be executed
         */
        void (*func)(void *);

        /**
         * Args to call the function
         */
        void* funcArgs;

        /**
         * Called after the execution of the task
         */
        void (*callback)(void *);

        /**
         * Args for the execution of the callback
         */
        void* callbackArgs;

        /**
         * Semaphore that notify when a new function to be executed is ready
         */
        sem_t* newFunctionSemaphore;

        /**
         * Pool owner of the worker
         */
        PThreadPool* ownerPool;

        /**
         * PThread variable
         */
        pthread_t workerPthread;

        /**
         * Loop of the worker
         * @return nullptr
         */
        static void* pthreadWorkerLoop(void* args);

    public:
        WorkerPThread(PThreadPool *ownerPool);

        virtual ~WorkerPThread();

        inline void executeFunction(void (*func)(void*),void* args) {
            executeFunction(func, args, nullptr, nullptr);
        }

        inline void executeFunction(void (*func)(void*),void* args,void (*callback)(void*),void* callbackArgs){
            this->func = func;
            this->funcArgs = args;
            this->callback = callback;
            this->callbackArgs = callbackArgs;

            sem_post(newFunctionSemaphore);
        }
    };

    /**
     * Number of worker threads available
     */
    unsigned int numWorkerThreads;

    /**
     * Queue of workers ready for a new function to be executed
     */
    std::queue<WorkerPThread*> readyWorkers;

    /**
     * Array of created workers
     */
    WorkerPThread** workers;

    /**
     * Mutex to synchronize the access to the queue
     */
    pthread_mutex_t queueMutex;

    /**
     * Semaphore that manage the execution of new functions through the executeFunction method
     */
    sem_t* poolSemaphore;

    /**
     * Synchronized access to the queue
     */
    inline WorkerPThread* popReadyQueue(){
        pthread_mutex_lock(&queueMutex);

        WorkerPThread* worker = readyWorkers.front();
        readyWorkers.pop();

        pthread_mutex_unlock(&queueMutex);

        return worker;
    }

    inline void pushReadyQueue(WorkerPThread* worker){
        pthread_mutex_lock(&queueMutex);

        readyWorkers.push(worker);

        pthread_mutex_unlock(&queueMutex);
    }

public:

    PThreadPool();
    PThreadPool(unsigned int numWorkerThreads);

    virtual ~PThreadPool();

    /**
     * Execute the new function passed as parameter
     * If there are no foree workers the call block
     * the current thread until a worker is freed
     * @param func The new function to be executed
     */
    inline void executeFunction(void (*func)(void*), void* args){
        executeFunction(func, args, nullptr, nullptr);
    }

    inline void executeFunction(void (*func)(void*), void* args, void (*callback)(void*), void* callbackArgs) {
        sem_wait(poolSemaphore);

        WorkerPThread* worker = popReadyQueue();

        worker->executeFunction(func, args, callback, callbackArgs);
    }

    inline unsigned int getNumWorkerThreads() {
        return numWorkerThreads;
    }
};


#endif //CODE_PTHREADPOOL_H
