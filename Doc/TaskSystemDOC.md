# TaskSystem


## Interface
  
### TaskSystem
Allow the execution of a user defined *TaskGraph* without having to code the synchronization mechanism among Tasks.

#### Constructors:
Create a new *TaskSystem* with a number of workers equal to the maximum number of threads supported by the system.
```cpp
TaskSystem();
```

Create a new *TaskSystem* with the number of workers passed as argument.
```cpp
TaskSystem(unsigned int numWorkers);
```

#### Execution

Execute the TaskGraph passed as argument, the method call return when all the Tasks of the TaskGraph have been executed.
```cpp
void executeTaskGraph(TaskGraph taskGraph);
```

#### Others:

Return the number of workers handled by the ThreadPool of the TaskSystem
```cpp
unsigned int getNumWorkerThreads();
```

### Task
A *Task* is the base element of a Graph, contain a function to be executed when all its incoming dependencies are satisfied and a dummy flag that is True if the task is not intended to execute code.
The Task should be a dummy Task if do not execute code and its purpose is just to lower the number of dependencies of the Graph.

#### Constructors:

Create an empty simple Task with an empty execution function.
```cpp
Task();
```


Create a Task with the dummy flag set as the passed argument.
```cpp
Task(bool dummy);
```


Create a Task that will execute the function passed as argument.
The argument passed to the executed function is the pointer to the task object.
```cpp
Task(void (*execute)(void*));
```

#### Dependencies:

Add a dependency between the Task and the Task passed as argument.
Can throw a *TaskElementParentingException* the two Tasks are not under the same Graph and a *CyclicGraphException* when the just added dependency lead to a cyclic Graph.
If a *CyclicGraphException* is thrown the parent TaskGraph is restored to the situation before the method call.
```cpp
void addDependencyTo(Task* task);
```

Add a dependency between the Task and the Task graph passed as argument.
Can throw a *TaskElementParentingException* when the parent of the Task and the parent of the TaskGraph are not the same and a *CyclicGraphException* when the just added dependency lead to a cyclic Graph.
If a *CyclicGraphException* is thrown the TaskGraph is restored to the situation before the method call.
```cpp
void addDependencyTo(TaskGraph* taskGraph);
```

#### Others:

Return an *unsigned int* that is a unique identifier of the Task.
```cpp
unsigned int getTaskID();
```

Set the function passed as argument as the function that is intended to be executed for this Task.
The argument passed to the executed function is a pointer to the task object.
```cpp
void setExecute(void (*execute)(void*));
```

Return the value of the dummy flag of the Task.
```cpp
bool isDummy();
```

### TaskGraph

A *TaskGraph* is a container for Tasks and subTaskGraphs, its purpose is to represent the dependencies among Tasks.
A *TaskGraph* always have two dummy tasks that are End and Start; these two tasks are necessary for its execution.


#### Constructors:

Create a new empty graph without a parent.
```cpp
TaskGraph();
```


#### Elements:

Add the Task passed as argument as a Task of the TaskGraph; the passed Task is added without dependencies.
Can throw a *TaskElementParentingException* when the Task passed as argument is already under a TaskGraph
```cpp
void addTask(Task* task);
```

Add the TaskGraph passed as argument as a subGraph of the TaskGraph; the passed TaskGraph is added without dependencies.
Can throw a *TaskElementParentingException* when the TaskGraph passed as argument is already under a TaskGraph and when the TaskGraph and the passed TaskGraph are the same Graph.
```cpp
void addSubGraph(TaskGraph* subGraph);
```

#### Dependencies:


Add a dependency between the TaskGraph and the Task passed as argument.
Can throw a *TaskElementParentingException* when the parent of the Task and the parent of the TaskGraph are not the same and a *CyclicGraphException* when the just added dependency lead to a cyclic Graph.
If a *CyclicGraphException* is thrown the TaskGraph is restored to the situation before the method call.
```cpp
void addDependencyTo(Task* task);
```

Add a dependency between the two TaskGraphs.
Can throw a *TaskElementParentingException* when the parent of the two Graph is not the same, when the two Graphs have no parent and when the two TaskGraphs are the same Graph.
Thwrow a *CyclicGraphException* when the just added dependency lead to a cyclic Graph.
If a *CyclicGraphException* is thrown the parent TaskGraph is restored to the situation before the method call.
```cpp
void addDependencyTo(TaskGraph* taskGraph);
```

### Utilities:

Return the start and end indexes of each worker to equally split the total ammount of work.
```cpp
inline void splitEqually(unsigned long totWork, unsigned int numWorkers, std::vector<std::pair<unsigned long, unsigned long>>* out){
```

## Example 

### Hello World!
```cpp
TaskSystem::TaskSystem::Task HelloWorld([](void*){
    std::cout << "HelloWorld!" << std::endl;
});

TaskSystem::TaskSystem taskSystem;
TaskSystem::TaskSystem::TaskGraph taskGraph;

taskGraph.addTask(&HelloWorld);
taskSystem.executeTaskGraph(taskGraph);

```


### Sum of two array of floats
```cpp
TaskSystem::TaskSystem taskSystem;
TaskSystem::TaskSystem::TaskGraph taskGraph;

class TaskSumFloats : public TaskSystem::TaskSystem::Task{
public:
    unsigned long startIndex;
    unsigned long endIndex;

    float* a;
    float* b;
    float* c;

    TaskSumFloats(unsigned long startIndex, unsigned long endIndex, float *a, float *b, float *c) : startIndex(
            startIndex), endIndex(endIndex), a(a), b(b), c(c) {}

};

unsigned int arraySize = 1000000;

float* a = new float[arraySize];
float* b = new float[arraySize];
float* c = new float[arraySize];

/*
 .
 .  Arrays initialization
 .
*/

std::vector<std::pair<unsigned long, unsigned long>> indexes;

TaskSystem::splitEqually(arraySize, 4, &indexes);

TaskSumFloats task1(indexes.at(0).first, indexes.at(0).second,a,b,c);
TaskSumFloats task2(indexes.at(1).first, indexes.at(1).second,a,b,c);
TaskSumFloats task3(indexes.at(2).first, indexes.at(2).second,a,b,c);
TaskSumFloats task4(indexes.at(3).first, indexes.at(3).second,a,b,c);

void (*sum)(void*) = [](void* args){
    TaskSumFloats* context = (TaskSumFloats*) args;

    for (int i = context->startIndex; i <= context->endIndex; ++i) {
        context->c[i] = context->a[i] + context->b[i];
    }
};

task1.setExecute(sum);
task2.setExecute(sum);
task3.setExecute(sum);
task4.setExecute(sum);

taskGraph.addTask(&task1);
taskGraph.addTask(&task2);
taskGraph.addTask(&task3);
taskGraph.addTask(&task4);

taskSystem.executeTaskGraph(taskGraph);
```
