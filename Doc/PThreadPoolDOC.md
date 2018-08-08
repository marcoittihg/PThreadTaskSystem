# PThreadPool

Create a pool of *PThreadWorkers* and allow the thread safe execution of the *executeFunction* calls.

## Interface

### Constructors

```cpp
PThreadPool();
```
Create a new *PThreadPool* with a number of workers equal to the maximum number threads supported by the system.



```cpp
PThreadPool(unsigned int numWorkerThreads);
```
Create a new *PThreadPool* with the number of workers passed as arguments.




### Execution calls

```cpp
void executeFunction(void (*func)(void*), void* args, void (*callback)(void*), void* callbackArgs)
```
Wait for a free worker thread and execute tht passed function func with args as arguments.
At the end of the execution call the passed callback with its arguments.



```cpp
void executeFunction(void (*func)(void*), void* args)
```
Wait for a free worker thread and execute tht passed function func with args as arguments; do not call a callback function.




### Utility

```cpp
unsigned int getNumWorkerThreads()
```
Return the number of created Workers.
