//
// Created by Marco on 28/07/18.
//

#ifndef CODE_TASKSYSTEM_H
#define CODE_TASKSYSTEM_H

#include "PThreadPool.h"
#include <exception>

namespace TaskSystem {
    /**
     * Allow to exploit task level parallelism by the definition of Tasks and TaskGraphs
     */
    class TaskSystem {
    public:
        class CyclicGraphException;
        class Task;
        class TaskGraph;

    private:

        /** Data of a dependency between two tasks
         */
        struct TaskDependency{
            /** Source task of the dependency
             */
            Task* fromTask;

            /** Target task of the dependency
             */
            Task* toTask;

            TaskDependency(Task *fromTask, Task *toTask);

            virtual ~TaskDependency();
        };

        /** Base class of all the elements of a TaskGraph
         */
        class TaskElement{
        protected:
            /** Parent of a Task or a TaskGraph
             */
            TaskGraph* parentGraph;

        public:

            void setParentGraph(TaskGraph* taskGraph);

            TaskGraph* getParentGraph();

            /**
             * Add a dependency from Task to TaskGraph or from TaskGraph to TaskGraph
             * @param taskGraph The target TaskGraph
             */
            virtual void addDependencyTo(TaskGraph* taskGraph) noexcept(false) = 0;

            /**
             * Add a dependency from Task to Task or from TaskGraph to Task
             * @param task
             */
            virtual void addDependencyTo(Task* task) noexcept(false) = 0;
        };

        /** ThreadPool for the execution of the Tasks
         */
        PThreadPool* pThreadPool;

    public:
        struct CyclicGraphException: std::exception{
        public:
            CyclicGraphException() {}
            CyclicGraphException(const CyclicGraphException&) noexcept {}
            CyclicGraphException& operator= (const CyclicGraphException& ) noexcept{return *this;}

            const char* what() const noexcept {
                return const_cast<char *>(" A GraphTask must not be cyclic! The dependency has been removed ");
            }
        };

        struct TaskElementParentingException : std::exception{
        public:
            TaskElementParentingException() {}
            TaskElementParentingException(const TaskElementParentingException&) noexcept {}
            TaskElementParentingException& operator= (const TaskElementParentingException& ) noexcept{return *this;}

            const char* what() const noexcept {
                return const_cast<char *>("Dependency type is not allowed");
            }
        };

        /** Define a task that can be executed by the TaskSystem
        */
        class Task : public TaskElement {
        private:
            /**
             * Identifier of the task
             */
            unsigned int taskID;

            /** True if the task is dummy and it do not execute code
             */
            bool dummy;

            /** dependencies that start from this task
             */
            std::vector<TaskDependency*> fromTask;

            /**
             * Dependencies that end in this task
             */
            std::vector<TaskDependency *> toTask;

            /**
             * Mutex used for the join call
             */
            pthread_mutex_t dependencyMutex;

            /**
             * Number of already satisfied dependencies
             */
            unsigned int satisfiedDependencies;

            /** Function to be executed
             */
            void (*execute)(void*);

        public:
            Task();

            Task(bool dummy);

            Task(void (*execute)(void*));

            virtual ~Task();

            void setExecute(void (*execute)(void*));

            /**
             * Right call to start the execution of the task
             */
            void startTask(PThreadPool* pool,void (*callback)(void*),void* callbackArgs);

            /**
             * @return True if the task is a dummy task
             */
            bool isDummy();

            /**
             * Free one dependency of the task
             * @return True if all the dependencies of the task are free
             */
            bool freeDependency();

            /**
             * Reset the number of satisfied dependencies
             */
            void resetSatDependencies();

            std::vector<TaskDependency *> getToTask();

            std::vector<TaskDependency *> getFromTask();

            unsigned int getTaskID();

            /**
             * Add a dependency between two tasks
             * @param taskStart Start task of the dependency
             * @param taskEnd End task of the dependency
             */
            static TaskDependency* addDependencyBetween(Task* taskStart, Task* taskEnd);

            /**
             * Remove all the direct dependencies between two tasks if any
             * @param taskStart Start task of the dependency
             * @param taskEnd End task of the dependency
             * @return True if at least one dependency has been removed
             */
            static bool removeDependencyBetween(Task* taskStart, Task* taskEnd);

            /**
             * Check if through the given dependency is possible to reach the given task
             * @param dependency The dependency to explore
             * @param task The task to check
             */
            static bool checkAcyclicDependency(TaskDependency dependency, Task* task);


            bool operator==(const Task &rhs) const;
            bool operator!=(const Task &rhs) const;

            void addDependencyTo(TaskGraph *taskGraph) noexcept(false) override;
            void addDependencyTo(Task *task) noexcept(false) override;
        };


        /** Graph of tasks to be executed
        */
        class TaskGraph : public TaskElement{

            class DummyStartEndTask : public Task{
            public:
                DummyStartEndTask() : Task(true){
                }

            private:
            };

            /** Subgraphs contained in this Graph
             */
            std::vector<TaskGraph*> subGraphs;

            /** Tasks directlly in this graph
             */
            std::vector<Task*> tasks;

            /** Start and End dummy tasks
             */
            DummyStartEndTask start;
            DummyStartEndTask end;

        public:
            TaskGraph();

            ~TaskGraph();

            /**
             * Add the new task to the taskGraph
             * @param task
             */
            void addTask(Task* task) noexcept (false);

            void addSubGraph(TaskGraph* subGraph) noexcept (false);

            void addDependencyTo(TaskGraph* taskGraph) noexcept(false) override;
            void addDependencyTo(Task* task) noexcept(false) override;

            /**
             * Reset all the dependencies to unsatisfied
             */
            void resetDependencies();

            DummyStartEndTask* getStart();

            DummyStartEndTask* getEnd();
        };


    public:
        TaskSystem();

        TaskSystem(unsigned int numWorkers);

        virtual ~TaskSystem();

        /**
         * Execute the TaskGraph passed as parameter
         * @param taskGraph The graph to be executed
         */
        void executeTaskGraph(TaskGraph* taskGraph);

        unsigned int getNumWorkerThreads();
    };

}


#endif //CODE_TASKSYSTEM_H
