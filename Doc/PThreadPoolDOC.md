# PThreadPool

Create a pool of **PThreadWorkers** that allow the parallel and thread safe execution of the **executeFunction** calls.  

## Interface

### Constructors  
Create a new *PThreadPool* with a number of workers equal to the maximum number of threads supported by the system.
```cpp
PThreadPool();
```
  
Create a new *PThreadPool* with the number of workers passed as arguments.
```cpp
PThreadPool(unsigned int numWorkerThreads);
```
  
### Execution calls
Wait for a free worker thread and execute the passed function func with args as arguments;
at the end of the execution the worker thread will call the callback with its arguments before wait for an other function to be executed. <br />
[Thread-Safe]
```cpp
void executeFunction(void (*func)(void*), void* args, void (*callback)(void*), void* callbackArgs)
```
  
  
Wait for a free worker thread and execute the passed function func with args as arguments; do not call a callback function.
[Thread-Safe]
```cpp
void executeFunction(void (*func)(void*), void* args)
```
  
### Utility
Return the number of handled Workers.
```cpp
unsigned int getNumWorkerThreads()
```

## Examples

Hello World

```cpp
void HelloWorld(void* args){
	std::cout << "Hello World!" << std::endl;
}

PThreadPool pool;
pool.executeFunction(HelloWorld, nullptr);

```
  