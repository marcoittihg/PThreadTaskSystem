# PThreadTaskSystem
*POSIX Thread* based task system that allow to define and execute a user defined *TaskGraph*.

## PThreadPool
Manage a pool of pthread workers to handle incoming execution requests.

[DOC](../master/Doc/PThreadPoolDOC.md)

## TaskSystem
Built on top of the PThreadPool, allow to create custom tasks and create dependencies among them.

[DOC](../master/Doc/TaskSystemDOC.md)