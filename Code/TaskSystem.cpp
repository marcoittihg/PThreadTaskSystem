//
// Created by Marco on 28/07/18.
//

#include <iostream>
#include "TaskSystem.h"

TaskSystem::TaskSystem::Task::Task() : Task(false){}

TaskSystem::TaskSystem::Task::Task(bool dummy) : dummy(dummy) {
    parentGraph = nullptr;
    inQueue = false;

    joinMutex = PTHREAD_MUTEX_INITIALIZER;

    static unsigned int idIncrement = 0;

    taskID = idIncrement++;
}

TaskSystem::TaskSystem::Task::~Task() {
}

bool TaskSystem::TaskSystem::Task::isDummy() {
    return dummy;
}

TaskSystem::TaskSystem::TaskDependency* TaskSystem::TaskSystem::Task::addDependencyBetween(TaskSystem::TaskSystem::Task *taskStart,
                                                        TaskSystem::TaskSystem::Task *taskEnd) {

    TaskDependency* newDependency = new TaskDependency(taskStart, taskEnd);

    taskStart->toTask.emplace_back(newDependency);
    taskEnd->fromTask.emplace_back(newDependency);

    return newDependency;
}

void TaskSystem::TaskSystem::Task::removeDependencyBetween(TaskSystem::TaskSystem::Task *taskStart,
                                                           TaskSystem::TaskSystem::Task *taskEnd) {

    std::vector<TaskDependency*>::iterator it = taskStart->toTask.begin();
    while(it != taskStart->toTask.end()){
        if((*it)->toTask == taskEnd){
            it = taskStart->toTask.erase(it);
        }else{
            it++;
        }
    }

    it = taskEnd->fromTask.begin();
    while(it != taskEnd->fromTask.end()){
        if((*it)->fromTask == taskStart){
            TaskDependency* dependency = *it;
            it = taskEnd->fromTask.erase(it);

            delete dependency;
        }else{
            it++;
        }
    }

}

void TaskSystem::TaskSystem::Task::addDependencyTo(TaskSystem::TaskSystem::Task *task) noexcept(false) {
    //check that the two tasks are under the same TaskGraph
    if(getParentGraph() != task->getParentGraph())
        throw DependencyTypeNotAllowedException();

    Task::removeDependencyBetween(this, task);
    Task::removeDependencyBetween(this, getParentGraph()->getEnd());
    Task::removeDependencyBetween(getParentGraph()->getStart(), task);

    TaskDependency* newDependency = Task::addDependencyBetween(this, task);

    if(checkAcyclicDependency(*newDependency, this)){
        Task::removeDependencyBetween(this, task);
        throw CyclicGraphException();
    }
}

void TaskSystem::TaskSystem::Task::addDependencyTo(TaskSystem::TaskSystem::TaskGraph *taskGraph) noexcept(false) {
    if (taskGraph->getParentGraph() != nullptr && taskGraph->getParentGraph() != getParentGraph())
        throw DependencyTypeNotAllowedException();

    TaskGraph *oldParent = taskGraph->getParentGraph();

    if (oldParent == nullptr) {
        //Add the new task graph under the parent of this task
        taskGraph->setParentGraph(getParentGraph());
        getParentGraph()->addSubGraph(taskGraph);

        Task::removeDependencyBetween(getParentGraph()->getStart(),taskGraph->getStart());
    } else {
        //Create dependencies
        Task::removeDependencyBetween(this, taskGraph->getStart());

        Task::removeDependencyBetween(getParentGraph()->getStart(), taskGraph->getStart());
    }

    Task::removeDependencyBetween(this, getParentGraph()->getEnd());
    TaskDependency* newDependency = Task::addDependencyBetween(this, taskGraph->getStart());

    if(checkAcyclicDependency(*newDependency, this)){
        Task::removeDependencyBetween(this, taskGraph->getStart());
        throw CyclicGraphException();
    }
}

bool TaskSystem::TaskSystem::Task::checkAcyclicDependency(TaskSystem::TaskSystem::TaskDependency dependency,
                                                          TaskSystem::TaskSystem::Task* task) {
    if(*task == *(dependency.toTask))
        return true;

    for (std::vector<TaskSystem::TaskSystem::TaskDependency*>::iterator it = (dependency.toTask)->toTask.begin();
            it != (dependency.toTask)->toTask.end(); it++) {

        if(checkAcyclicDependency(*(*it), task))
            return true;
    }

    return false;
}

bool TaskSystem::TaskSystem::Task::operator==(const TaskSystem::TaskSystem::Task &rhs) const {
    return taskID == rhs.taskID;
}

bool TaskSystem::TaskSystem::Task::operator!=(const TaskSystem::TaskSystem::Task &rhs) const {
    return !(rhs == *this);
}

void TaskSystem::TaskSystem::Task::startTask(PThreadPool* pool) {
    if(dummy) return;

    pthread_mutex_lock(&joinMutex);
    pool->executeFunction(execute, this, &joinMutex);
}

void TaskSystem::TaskSystem::Task::join() {
    pthread_mutex_lock(&joinMutex);
    pthread_mutex_unlock(&joinMutex);
}

std::vector<TaskSystem::TaskSystem::TaskDependency *> TaskSystem::TaskSystem::Task::getToTask()  {
    return toTask;
}

bool TaskSystem::TaskSystem::Task::isInQueue() {
    return inQueue;
}

void TaskSystem::TaskSystem::Task::setInQueue(bool value) {
    inQueue = value;
}

std::vector<TaskSystem::TaskSystem::TaskDependency *> TaskSystem::TaskSystem::Task::getFromTask() {
    return fromTask;
}

void TaskSystem::TaskSystem::Task::setExecute(void (*execute)(void *)) {
    Task::execute = execute;
}

TaskSystem::TaskSystem::Task::FuncPointer TaskSystem::TaskSystem::Task::getExecute() {
    return execute;
}

TaskSystem::TaskSystem::Task::Task(void (*execute)(void *)) : Task(false){
    this->execute = execute;
}

TaskSystem::TaskSystem::TaskGraph::TaskGraph() {
    Task::addDependencyBetween(&start, &end);

    parentGraph = nullptr;
}

void TaskSystem::TaskSystem::TaskGraph::addTask(TaskSystem::TaskSystem::Task* task) noexcept (false){
    //check that the task is not already under a task graph
    if(task->getParentGraph() != nullptr)
        throw TaskElementParentingException();

    if(tasks.empty())
        Task::removeDependencyBetween(&start, &end);

    Task::addDependencyBetween(&start, task);
    Task::addDependencyBetween(task, &end);

    task->setParentGraph (this);

    tasks.push_back(task);
}

TaskSystem::TaskSystem::TaskGraph::~TaskGraph() {

}

void TaskSystem::TaskSystem::TaskGraph::addDependencyTo(TaskSystem::TaskSystem::TaskGraph *taskGraph) noexcept(false) {
    if(getParentGraph() != taskGraph->getParentGraph())
        throw DependencyTypeNotAllowedException();

    if(getParentGraph() != nullptr){
        Task::removeDependencyBetween(getParentGraph()->getStart(), taskGraph->getStart());
        Task::removeDependencyBetween(getEnd(), getParentGraph()->getEnd());
        Task::removeDependencyBetween(getEnd(), taskGraph->getStart());
    }

    TaskDependency* newDependency = Task::addDependencyBetween(getEnd(), taskGraph->getStart());

    if(Task::checkAcyclicDependency(*newDependency, getEnd())){
        Task::removeDependencyBetween(getEnd(), taskGraph->getStart());
        throw CyclicGraphException();
    }
}

void TaskSystem::TaskSystem::TaskGraph::addDependencyTo(TaskSystem::TaskSystem::Task *task) noexcept(false) {
    if(getParentGraph() != task->getParentGraph() && getParentGraph() != nullptr)
        throw DependencyTypeNotAllowedException();

    if(getParentGraph() == nullptr){
        //Add the new task graph under the parent of this task
        setParentGraph(task->getParentGraph());
        getParentGraph()->addSubGraph(this);

        Task::removeDependencyBetween(getParentGraph()->getStart(), task);
        Task::removeDependencyBetween(getEnd(), getParentGraph()->getEnd());

    }else{
        Task::removeDependencyBetween(getParentGraph()->getStart(), task);
        Task::removeDependencyBetween(getEnd(), getParentGraph()->getEnd());
        Task::removeDependencyBetween(getEnd(), task);
    }

    TaskDependency* newDependency = Task::addDependencyBetween(getEnd(), task);

    if(Task::checkAcyclicDependency(*newDependency, getEnd())){
        Task::removeDependencyBetween(getEnd(), task);
        throw CyclicGraphException();
    }
}

TaskSystem::TaskSystem::TaskGraph::DummyStartEndTask *TaskSystem::TaskSystem::TaskGraph::getStart() {
    return &start;
}

TaskSystem::TaskSystem::TaskGraph::DummyStartEndTask *TaskSystem::TaskSystem::TaskGraph::getEnd() {
    return &end;
}

void TaskSystem::TaskSystem::TaskGraph::addSubGraph(TaskSystem::TaskSystem::TaskGraph *subGraph) {
    if(subGraph->getParentGraph() != nullptr)
        throw TaskElementParentingException();

    if(subGraphs.empty())
        Task::removeDependencyBetween(&start, &end);

    subGraph->setParentGraph(this);

    Task::addDependencyBetween(getStart(), subGraph->getStart());
    Task::addDependencyBetween(subGraph->getEnd(), getEnd());

    subGraphs.push_back(subGraph);
}

TaskSystem::TaskSystem::TaskDependency::TaskDependency(TaskSystem::TaskSystem::Task *fromTask,
                                                       TaskSystem::TaskSystem::Task *toTask) : fromTask(
        fromTask), toTask(toTask) {}

void TaskSystem::TaskSystem::executeTaskGraph(TaskSystem::TaskGraph taskGraph) {
    Task* start = taskGraph.getStart();
    Task* end = taskGraph.getEnd();

    std::queue<Task*> taskQueue;

    std::vector<TaskDependency*> dependencyList = start->getToTask();

    for (std::vector<TaskDependency*>::iterator it = dependencyList.begin(); it != dependencyList.end() ; it++) {
        TaskDependency* dependency = *it;

        Task* t= (dependency->toTask);
        if(!(t->isInQueue())) {
            taskQueue.push(t);
            t->setInQueue(true);
        }
    }

    while(!taskQueue.empty()){
        Task* newTask = taskQueue.front();
        taskQueue.pop();

        //wait for previous tasks
        dependencyList = newTask->getFromTask();
        for(std::vector<TaskDependency *>::iterator it = dependencyList.begin();
                it != dependencyList.end(); it++){
            Task* task = (*it)->fromTask;
            task->join();
        }

        //Execute
        newTask->startTask(pThreadPool);

        //Add next tasks
        dependencyList = newTask->getToTask();
        for (std::vector<TaskDependency *>::iterator it = dependencyList.begin();
                it != dependencyList.end(); it++) {
            Task *task = (*it)->toTask;

            if (!((task)->isInQueue())) {
                taskQueue.push(task);
                task->setInQueue(true);
            }
        }

        //Free the task values
        newTask->setInQueue(false);
    }
}

TaskSystem::TaskSystem::TaskSystem() {
    pThreadPool = new PThreadPool();
}

TaskSystem::TaskSystem::~TaskSystem() {
    delete pThreadPool;
}