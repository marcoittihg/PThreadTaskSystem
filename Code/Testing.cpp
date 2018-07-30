//
// Created by Marco on 29/07/18.
//

#define BOOST_TEST_MODULE TaskSystemTesting

#include <boost/test/included/unit_test.hpp>

#include "TaskSystem.h"
#include <pthread.h>
#include <thread>

/****************************************************************
 *  TASK TO TASK TESTS
 ****************************************************************/

/**
 * Test that an empty graph execution with only start and end tasks do not crash
 */
BOOST_AUTO_TEST_CASE(test_case_empty){
    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph;
        TaskSystem::TaskSystem taskSystem;

        taskSystem.executeTaskGraph(taskGraph);

    }catch(std::exception exe){
        BOOST_TEST(false);
    }

}

/**
 * Test that a graph is able to execute one custom simple task
 */
BOOST_AUTO_TEST_CASE(test_case_single_task){

    class MyTask : public TaskSystem::TaskSystem::Task{
    public:
        int a;
        MyTask() {a = 0;}
    } myTask;

    myTask.setExecute([](void* arg){
        MyTask* context = (MyTask*) arg;
        context->a++;
    });

    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph;
        TaskSystem::TaskSystem taskSystem;

        taskGraph.addTask(&myTask);

        taskSystem.executeTaskGraph(taskGraph);

    }catch(std::exception exe){
        BOOST_TEST(false);
    }

    BOOST_TEST(myTask.a == 1);
}

/**
 * Test that a graph is able to execute two tasks in parallel
 */
BOOST_AUTO_TEST_CASE(test_case_two_parallel_tasks){

    class MutexTask : public TaskSystem::TaskSystem::Task{
    public:
        sem_t* sem;
        MutexTask* mutexTask2;

        void setMutexTask2(MutexTask *mutexTask) {
            this->mutexTask2 = mutexTask;
        }

        MutexTask() {
            sem = sem_open("TestTwoParallel",O_CREAT,0644,0);
            sem_unlink("TestTwoParallel");
        }

        virtual ~MutexTask() {
            sem_close(sem);
        }
    } mutexTask1, mutexTask2;

    mutexTask1.setMutexTask2(&mutexTask2);
    mutexTask2.setMutexTask2(&mutexTask1);

    mutexTask1.setExecute([](void* arg){
        MutexTask* context = (MutexTask*) arg;

        sem_wait(context->mutexTask2->sem);
        sem_post(context->sem);
    });
    mutexTask2.setExecute([](void* arg){
        MutexTask* context = (MutexTask*) arg;

        sem_post(context->sem);
        sem_wait(context->mutexTask2->sem);
    });

    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph;
        TaskSystem::TaskSystem taskSystem;

        taskGraph.addTask(&mutexTask1);
        taskGraph.addTask(&mutexTask2);

        taskSystem.executeTaskGraph(taskGraph);
    }catch(std::exception exe){
        BOOST_TEST(false);
    }
}

/**
 * Test that a graph is able to execute two dependant tasks and
 * respect the dependency between the two tasks
 */
BOOST_AUTO_TEST_CASE(test_case_two_serial_tasks){
    class MyTask1 : public TaskSystem::TaskSystem::Task{
    public:
        int a;
        MyTask1() {a = 0;}
    } myTask1;

    class MyTask2 : public TaskSystem::TaskSystem::Task{
    public:
        MyTask1* task1;

        void setTask1(MyTask1* task1) {
            this->task1 = task1;
        }

        MyTask2() {}
    } myTask2;

    myTask2.setTask1(&myTask1);

    myTask1.setExecute([](void* arg){
        MyTask1* context = (MyTask1*) arg;

        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        context -> a++;
    });
    myTask2.setExecute([](void* arg){
        MyTask2* context = (MyTask2*) arg;

        BOOST_TEST(context->task1->a == 1);

        context->task1->a++;
    });

    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph;
        TaskSystem::TaskSystem taskSystem;

        taskGraph.addTask(&myTask1);
        taskGraph.addTask(&myTask2);

        myTask1.addDependencyTo(&myTask2);

        taskSystem.executeTaskGraph(taskGraph);

    }catch(std::exception exe){
        BOOST_TEST(false);
    }

    BOOST_TEST(myTask1.a == 2);
}

/**
 * Test that when there are multiple dependencies from many tasks to one
 * the dependencies are respected
 */
BOOST_AUTO_TEST_CASE(test_case_multiple_into_one_dependency){
    class MyTask : public TaskSystem::TaskSystem::Task{
    public:
        int* a;

        void setA(int *a) {
            MyTask::a = a;
        }

        MyTask() {}
    } task1,task2,task3,task4,taskF;

    void (*func)(void*) = [](void* arg){
        MyTask* context = (MyTask*) arg;
        *(context->a) = *(context->a) + 1;
    };

    task1.setExecute(func);
    task2.setExecute(func);
    task3.setExecute(func);
    task4.setExecute(func);

    int n = 0;

    task1.setA(&n);
    task2.setA(&n);
    task3.setA(&n);
    task4.setA(&n);
    taskF.setA(&n);

    taskF.setExecute([](void* arg){
        MyTask* context = (MyTask*) arg;
       BOOST_TEST(*(context->a) == 4);
    });

    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph;
        TaskSystem::TaskSystem taskSystem;

        taskGraph.addTask(&task1);
        taskGraph.addTask(&task2);
        taskGraph.addTask(&task3);
        taskGraph.addTask(&task4);
        taskGraph.addTask(&taskF);

        task1.addDependencyTo(&taskF);
        task2.addDependencyTo(&taskF);
        task3.addDependencyTo(&taskF);
        task4.addDependencyTo(&taskF);

        taskSystem.executeTaskGraph(taskGraph);

    }catch(std::exception exe){
        BOOST_TEST(false);
    }
}

/**
 * Test that when there is a cycle graph of tasks a
 * CyclicGraphException is thrown and the graph still executable
 */
BOOST_AUTO_TEST_CASE(test_case_cyclic_exception_still_executable){
    TaskSystem::TaskSystem::TaskGraph taskGraph;
    TaskSystem::TaskSystem taskSystem;

    try {

        TaskSystem::TaskSystem::Task task1([](void*){});
        TaskSystem::TaskSystem::Task task2([](void*){});
        TaskSystem::TaskSystem::Task task3([](void*){});

        taskGraph.addTask(&task1);
        taskGraph.addTask(&task2);
        taskGraph.addTask(&task3);

        task1.addDependencyTo(&task2);
        task2.addDependencyTo(&task3);
        task3.addDependencyTo(&task1);

    }
    catch(TaskSystem::TaskSystem::CyclicGraphException& exe){
        try {
            taskSystem.executeTaskGraph(taskGraph);
        }catch (std::exception& exe){
            BOOST_TEST(false);
        }
        return;
    }
    catch(std::exception& exe){
        BOOST_TEST(false);
    }

    BOOST_TEST(false);
}

/**
 * Test that when two tasks are under two graphs they can not be linked with a dependency
 * a DependencyTypeNotAllowedException is thrown
 */
BOOST_AUTO_TEST_CASE(test_case_two_tasks_in_two_graphs) {

    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph1, taskGraph2;
        TaskSystem::TaskSystem taskSystem;

        TaskSystem::TaskSystem::Task task1([](void*){});
        TaskSystem::TaskSystem::Task task2([](void*){});

        taskGraph1.addTask(&task1);
        taskGraph2.addTask(&task2);

        task1.addDependencyTo(&task2);

    }
    catch (TaskSystem::TaskSystem::DependencyTypeNotAllowedException &exe) {
        return;
    }
    catch (std::exception &exe) {
        BOOST_TEST(false);
    }
    BOOST_TEST(false);
}

/**
 * Test that a task can not be added twice in the same graph
 * a TaskElementParentingException is thrown and the graph still
 * executable and execute the task once
 */
BOOST_AUTO_TEST_CASE(test_case_a_task_cant_be_added_twice){
    TaskSystem::TaskSystem::TaskGraph taskGraph;
    TaskSystem::TaskSystem taskSystem;
    int a = 0;

    class MyTask : public TaskSystem::TaskSystem::Task{
    public:
        int* n;
        MyTask(int *n) : n(n) {}
    }task1(&a);

    try {
        task1.setExecute([](void* arg){
            MyTask* context = (MyTask*) arg;
            *(context->n) = *(context->n) + 1;
        });

        taskGraph.addTask(&task1);
        taskGraph.addTask(&task1);

    }
    catch (TaskSystem::TaskSystem::TaskElementParentingException &exe) {
        try {
            taskSystem.executeTaskGraph(taskGraph);
        }catch(std::exception& exe){
            BOOST_TEST(false);
        }

        BOOST_TEST(a == 1);

        return;
    }
    catch(std::exception exe){
        BOOST_TEST(false);
    }
    BOOST_TEST(false);
}

/**
 * Test that a task can not added in two different graphs without
 * throw a TaskElementParentingException
 */
BOOST_AUTO_TEST_CASE(test_case_a_task_cant_be_in_two_graphs){
    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph1, taskGraph2;
        TaskSystem::TaskSystem taskSystem;

        TaskSystem::TaskSystem::Task task1([](void*){});

        taskGraph1.addTask(&task1);
        taskGraph2.addTask(&task1);
    }
    catch (TaskSystem::TaskSystem::TaskElementParentingException &exe) {
        return;
    }
    catch(std::exception exe){
        BOOST_TEST(false);
    }
    BOOST_TEST(false);
}

/**
 * Test that a task can not have a dependency with itself without
 * throw a CyclicGraphException and the graph still executable
 */
BOOST_AUTO_TEST_CASE(test_case_a_task_cant_depend_by_itself){
    TaskSystem::TaskSystem::TaskGraph taskGraph;
    TaskSystem::TaskSystem taskSystem;

    try {

        TaskSystem::TaskSystem::Task task1([](void*){});

        taskGraph.addTask(&task1);

        task1.addDependencyTo(&task1);

    }
    catch(TaskSystem::TaskSystem::CyclicGraphException& exe){
        try {
            taskSystem.executeTaskGraph(taskGraph);
        }catch (std::exception& exe){
            BOOST_TEST(false);
        }
        return;
    }
    catch(std::exception& exe){
        BOOST_TEST(false);
    }

    BOOST_TEST(false);
}


/****************************************************************
 *  MISC TASK TO GRAPH AND GRAPH TO TASK TESTS
 ****************************************************************/

/**
 * Test that two graph can be linked and be executed
 */
BOOST_AUTO_TEST_CASE(test_case_two_linked_graphs){

    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph1,taskGraph2;
        TaskSystem::TaskSystem taskSystem;

        taskGraph1.addDependencyTo(&taskGraph2);

        taskSystem.executeTaskGraph(taskGraph1);

    }catch(std::exception exe){
        BOOST_TEST(false);
    }

}
/**
 * Test that two tasks can be inserted in two different graph,
 * the two graphs can be nested and executed in parallel
 */
BOOST_AUTO_TEST_CASE(test_case_subgraph){

    class MutexTask : public TaskSystem::TaskSystem::Task{
    public:
        sem_t* sem;
        MutexTask* mutexTask2;

        void setMutexTask2(MutexTask *mutexTask) {
            this->mutexTask2 = mutexTask;
        }

        MutexTask() {
            sem = sem_open("TestTwoParallel",O_CREAT,0644,0);
            sem_unlink("TestTwoParallel");
        }

        virtual ~MutexTask() {
            sem_close(sem);
        }
    } mutexTask1, mutexTask2;

    mutexTask1.setMutexTask2(&mutexTask2);
    mutexTask2.setMutexTask2(&mutexTask1);

    mutexTask1.setExecute([](void* arg){
        MutexTask* context = (MutexTask*) arg;

        sem_wait(context->mutexTask2->sem);
        sem_post(context->sem);
    });
    mutexTask2.setExecute([](void* arg){
        MutexTask* context = (MutexTask*) arg;

        sem_post(context->sem);
        sem_wait(context->mutexTask2->sem);
    });

    try {
        TaskSystem::TaskSystem::TaskGraph taskGraph1,taskGraph2;
        TaskSystem::TaskSystem taskSystem;

        taskGraph1.addTask(&mutexTask1);
        taskGraph2.addTask(&mutexTask2);

        taskGraph1.addSubGraph(&taskGraph2);

        taskSystem.executeTaskGraph(taskGraph1);
    }catch(std::exception exe){
        BOOST_TEST(false);
    }
}