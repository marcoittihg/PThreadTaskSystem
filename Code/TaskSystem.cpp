//
// Created by Marco on 28/07/18.
//

#include <iostream>
#include "TaskSystem.h"


namespace TaskSystem {

    void TaskSystem::TaskElement::setParentGraph(TaskSystem::TaskGraph *taskGraph) {
        parentGraph = taskGraph;
    }

    TaskSystem::TaskGraph* TaskSystem::TaskElement::getParentGraph() {
        return parentGraph;
    }

    TaskSystem::Task::Task() : Task(false) {}

    TaskSystem::Task::Task(bool dummy) : dummy(dummy) {
        parentGraph = nullptr;

        dependencyMutex = PTHREAD_MUTEX_INITIALIZER;

        execute = [](void*){};

        static unsigned int idIncrement = 0;

        taskID = idIncrement++;
    }

    TaskSystem::Task::~Task() {
    }

    bool TaskSystem::Task::isDummy() {
        return dummy;
    }

    TaskSystem::TaskDependency *
    TaskSystem::Task::addDependencyBetween(TaskSystem::Task *taskStart,
                                                       TaskSystem::Task *taskEnd) {

        TaskDependency *newDependency = new TaskDependency(taskStart, taskEnd);

        taskStart->toTask.emplace_back(newDependency);
        taskEnd->fromTask.emplace_back(newDependency);

        return newDependency;
    }

    bool TaskSystem::Task::removeDependencyBetween(TaskSystem::Task *taskStart,
                                                               TaskSystem::Task *taskEnd) {
        bool found = false;

        std::vector<TaskDependency *>::iterator it = taskStart->toTask.begin();
        while (it != taskStart->toTask.end()) {
            if ((*it)->toTask == taskEnd) {
                it = taskStart->toTask.erase(it);
                found = true;
            } else {
                it++;
            }
        }

        it = taskEnd->fromTask.begin();
        while (it != taskEnd->fromTask.end()) {
            if ((*it)->fromTask == taskStart) {
                TaskDependency *dependency = *it;
                it = taskEnd->fromTask.erase(it);

                delete dependency;
            } else {
                it++;
            }
        }

        return found;
    }

    void TaskSystem::Task::addDependencyTo(TaskSystem::Task *task) noexcept(false) {
        //check that the two tasks are under the same TaskGraph
        if (getParentGraph() != task->getParentGraph() || getParentGraph() == nullptr)
            throw TaskElementParentingException();

        Task::removeDependencyBetween(this, task);

        bool foundTe = Task::removeDependencyBetween(this, getParentGraph()->getEnd());
        bool foundSt = Task::removeDependencyBetween(getParentGraph()->getStart(), task);

        TaskDependency *newDependency = Task::addDependencyBetween(this, task);

        if (checkAcyclicDependency(*newDependency, this)) {
            Task::removeDependencyBetween(this, task);

            if (foundTe) Task::addDependencyBetween(this, getParentGraph()->getEnd());
            if (foundSt) Task::addDependencyBetween(getParentGraph()->getStart(), task);

            throw CyclicGraphException();
        }
    }

    void TaskSystem::Task::addDependencyTo(TaskSystem::TaskGraph *taskGraph) noexcept(false) {
        if (taskGraph->getParentGraph() != getParentGraph() || getParentGraph() == nullptr)
            throw TaskElementParentingException();

        TaskGraph *oldParent = taskGraph->getParentGraph();

        bool foundSg, foundTe;

        //Create dependencies
        Task::removeDependencyBetween(this, taskGraph->getStart());

        foundSg = Task::removeDependencyBetween(getParentGraph()->getStart(), taskGraph->getStart());
        foundTe = Task::removeDependencyBetween(this, getParentGraph()->getEnd());

        TaskDependency *newDependency = Task::addDependencyBetween(this, taskGraph->getStart());

        if (checkAcyclicDependency(*newDependency, this)) {
            Task::removeDependencyBetween(this, taskGraph->getStart());

            if (foundSg) Task::addDependencyBetween(getParentGraph()->getStart(), taskGraph->getStart());
            if (foundTe) Task::addDependencyBetween(this, getParentGraph()->getEnd());

            throw CyclicGraphException();
        }
    }

    bool TaskSystem::Task::checkAcyclicDependency(TaskSystem::TaskDependency dependency,
                                                              TaskSystem::Task *task) {
        if (*task == *(dependency.toTask))
            return true;

        for (std::vector<TaskSystem::TaskDependency *>::iterator it = (dependency.toTask)->toTask.begin();
             it != (dependency.toTask)->toTask.end(); it++) {

            if (checkAcyclicDependency(*(*it), task))
                return true;
        }

        return false;
    }

    bool TaskSystem::Task::operator==(const TaskSystem::Task &rhs) const {
        return taskID == rhs.taskID;
    }

    bool TaskSystem::Task::operator!=(const TaskSystem::Task &rhs) const {
        return !(rhs == *this);
    }

    void TaskSystem::Task::startTask(PThreadPool *pool, void (*callback)(void *), void *callbackArgs) {
        if (dummy) {
            callback(callbackArgs);
        } else {
            pool->executeFunction(execute, this, callback, callbackArgs);
        }
    }

    std::vector<TaskSystem::TaskDependency *> TaskSystem::Task::getToTask() {
        return toTask;
    }

    std::vector<TaskSystem::TaskDependency *> TaskSystem::Task::getFromTask() {
        return fromTask;
    }

    void TaskSystem::Task::setExecute(void (*execute)(void *)) {
        Task::execute = execute;
    }

    TaskSystem::Task::Task(void (*execute)(void *)) : Task(false) {
        this->execute = execute;
    }

    bool TaskSystem::Task::freeDependency() {
        pthread_mutex_lock(&dependencyMutex);

        satisfiedDependencies++;
        bool result = satisfiedDependencies == fromTask.size();

        pthread_mutex_unlock(&dependencyMutex);

        return result;
    }

    void TaskSystem::Task::resetSatDependencies() {
        satisfiedDependencies = 0;
    }

    unsigned int TaskSystem::Task::getTaskID() {
        return taskID;
    }

    TaskSystem::TaskGraph::TaskGraph() {
        Task::addDependencyBetween(&start, &end);

        start.setParentGraph(this);
        end.setParentGraph(this);

        tasks.push_back(&start);
        tasks.push_back(&end);

        parentGraph = nullptr;
    }

    void TaskSystem::TaskGraph::addTask(TaskSystem::Task *task) noexcept(false) {
        //check that the task is not already under a task graph
        if (task->getParentGraph() != nullptr)
            throw TaskElementParentingException();

        if (tasks.size() == 2)
            Task::removeDependencyBetween(&start, &end);

        Task::addDependencyBetween(&start, task);
        Task::addDependencyBetween(task, &end);

        task->setParentGraph(this);

        tasks.push_back(task);
    }

    TaskSystem::TaskGraph::~TaskGraph() {

    }

    void
    TaskSystem::TaskGraph::addDependencyTo(TaskSystem::TaskGraph *taskGraph) noexcept(false) {
        if (getParentGraph() != taskGraph->getParentGraph() || getParentGraph() == nullptr
                || start.getTaskID() == taskGraph->start.getTaskID())
            throw TaskElementParentingException();

        bool foundSg, foundGe;

        foundSg = Task::removeDependencyBetween(getParentGraph()->getStart(), taskGraph->getStart());
        foundGe = Task::removeDependencyBetween(getEnd(), getParentGraph()->getEnd());

        Task::removeDependencyBetween(getEnd(), taskGraph->getStart());

        TaskDependency *newDependency = Task::addDependencyBetween(getEnd(), taskGraph->getStart());

        if (Task::checkAcyclicDependency(*newDependency, getEnd())) {
            Task::removeDependencyBetween(getEnd(), taskGraph->getStart());

            if (foundSg) Task::addDependencyBetween(getParentGraph()->getStart(), taskGraph->getStart());
            if (foundGe) Task::addDependencyBetween(getEnd(), getParentGraph()->getEnd());

            throw CyclicGraphException();
        }
    }

    void TaskSystem::TaskGraph::addDependencyTo(TaskSystem::Task *task) noexcept(false) {
        if (getParentGraph() != task->getParentGraph() && getParentGraph() != nullptr)
            throw TaskElementParentingException();

        bool foundSt, foundGe;

        foundSt = Task::removeDependencyBetween(getParentGraph()->getStart(), task);
        foundGe = Task::removeDependencyBetween(getEnd(), getParentGraph()->getEnd());

        Task::removeDependencyBetween(getEnd(), task);

        TaskDependency *newDependency = Task::addDependencyBetween(getEnd(), task);

        if (Task::checkAcyclicDependency(*newDependency, getEnd())) {
            Task::removeDependencyBetween(getEnd(), task);

            if (foundSt) Task::addDependencyBetween(getParentGraph()->getStart(), task);
            if (foundGe) Task::addDependencyBetween(getEnd(), getParentGraph()->getEnd());

            throw CyclicGraphException();
        }
    }

    TaskSystem::TaskGraph::DummyStartEndTask *TaskSystem::TaskGraph::getStart() {
        return &start;
    }

    TaskSystem::TaskGraph::DummyStartEndTask *TaskSystem::TaskGraph::getEnd() {
        return &end;
    }

    void TaskSystem::TaskGraph::addSubGraph(TaskSystem::TaskGraph *subGraph) {
        if (subGraph->getParentGraph() != nullptr || subGraph->start.getTaskID() == start.getTaskID())
            throw TaskElementParentingException();

        if (subGraphs.empty())
            Task::removeDependencyBetween(&start, &end);

        subGraph->setParentGraph(this);

        Task::addDependencyBetween(getStart(), subGraph->getStart());
        Task::addDependencyBetween(subGraph->getEnd(), getEnd());

        subGraphs.push_back(subGraph);
    }

    void TaskSystem::TaskGraph::resetDependencies() {
        for (std::vector<Task *>::iterator it = tasks.begin(); it != tasks.end(); it++) {
            (*it)->resetSatDependencies();
        }

        for (std::vector<TaskGraph *>::iterator it = subGraphs.begin(); it != subGraphs.end(); it++) {
            (*it)->resetDependencies();
        }
    }

    TaskSystem::TaskDependency::TaskDependency(TaskSystem::Task *fromTask,
                                                           TaskSystem::Task *toTask) : fromTask(
            fromTask), toTask(toTask) {}

    void TaskSystem::executeTaskGraph(TaskSystem::TaskGraph taskGraph) {
        Task *start = taskGraph.getStart();
        Task *end = taskGraph.getEnd();

        taskGraph.resetDependencies();

        class ThreadSafeQueue {
            std::queue<Task *> queue;

            pthread_mutex_t mutex;
            sem_t *sem;
        public:
            ThreadSafeQueue() {
                mutex = PTHREAD_MUTEX_INITIALIZER;
                sem = sem_open("QueueSem", O_CREAT, 0644, 0);
                sem_unlink("QueueSem");
            }

            inline void safePut(Task *task) {
                pthread_mutex_lock(&mutex);
                queue.push(task);
                sem_post(sem);
                pthread_mutex_unlock(&mutex);
            }

            inline Task *safePop() {
                sem_wait(sem);
                pthread_mutex_lock(&mutex);
                Task *task = queue.front();
                queue.pop();
                pthread_mutex_unlock(&mutex);

                return task;
            }
        } taskQueue;

        taskQueue.safePut(start);

        struct CBArgs {
            ThreadSafeQueue *queuePointer;
            Task *taskPointer;

            CBArgs(ThreadSafeQueue *queuePointer, Task *taskPointer) : queuePointer(queuePointer),
                                                                       taskPointer(taskPointer) {}
        };

        void (*callback)(void *) = [](void *args) {
            CBArgs *cbArgs = (CBArgs *) args;

            Task *task = cbArgs->taskPointer;
            ThreadSafeQueue *queue = cbArgs->queuePointer;

            std::vector<TaskDependency *> dependencyList = task->getToTask();
            for (std::vector<TaskDependency *>::iterator it = dependencyList.begin();
                 it != dependencyList.end(); it++) {

                Task *toTask = (*it)->toTask;

                if (toTask->freeDependency()) {
                    queue->safePut(toTask);
                }
            }

            delete cbArgs;
        };

        while (true) {
            Task *task = taskQueue.safePop();


            if (task->getTaskID() == end->getTaskID())
                break;

            task->startTask(pThreadPool, callback, new CBArgs(&taskQueue, task));
        }
    }

    TaskSystem::TaskSystem() {
        pThreadPool = new PThreadPool();
    }

    TaskSystem::TaskSystem(unsigned int numWorkers) {
        pThreadPool = new PThreadPool(numWorkers);
    }

    TaskSystem::~TaskSystem() {
        delete pThreadPool;
    }

    unsigned int TaskSystem::getNumWorkerThreads() {
        return pThreadPool->getNumWorkerThreads();
    }

}