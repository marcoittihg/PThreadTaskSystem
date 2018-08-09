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

Execute the TaskGraph passed as argument, the method call returns when all the Tasks of the TaskGraph have been executed.
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
The Task should be a dummy Task if its purpose is just to lower the number of dependencies of the Graph.

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
```cpp
Task(void (*execute)(void*));
```

#### Dependencies:

Add a dependency between the Task and the Task passed as argument.
Can throw a *TaskElementParentingException* the two Tasks are not under the same Graph and a *CyclicGraphException* when the just added dependency lead to a cyclic Graph.
If a *CyclicGraphException* is thrown the parent TaskGraph is restored to the situation before the call of the method.
```cpp
void addDependencyTo(Task* task);
```

Add a dependency between the Task and the Task graph passed as argument.
Can throw a *TaskElementParentingException* when the parent of the Task and the parent of the TaskGraph are not the same and a *CyclicGraphException* when the just added dependency lead to a cyclic Graph.
If a *CyclicGraphException* is thrown the TaskGraph is restored to the situation before the call of the method.
```cpp
void addDependencyTo(TaskGraph* taskGraph);
```

#### Others:

Return an *unsigned int* that is the unique identifier of the Task.
```cpp
unsigned int getTaskID();
```

Set the function passed as argument as the function that is intended to be executed for this Task.
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
If a *CyclicGraphException* is thrown the TaskGraph is restored to the situation before the call of the method.
```cpp
void addDependencyTo(Task* task);
```

Add a dependency between the two TaskGraphs.
Can throw a *TaskElementParentingException* when the parent of the two Graph is not the same, when the two Graphs have no parent and when the two TaskGraphs are the same Graph.
Thwrow a *CyclicGraphException* when the just added dependency lead to a cyclic Graph.
If a *CyclicGraphException* is thrown the parent TaskGraph is restored to the situation before the call of the method.
```cpp
void addDependencyTo(TaskGraph* taskGraph);
```
