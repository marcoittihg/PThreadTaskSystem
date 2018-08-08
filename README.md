# PThreadTaskSystem
*POSIX Thread* based task system that allow to define and execute a *TaskGraph* of user defined *Tasks* and *SubGraphs*.

## PThreadPool
Manage a pool of pthread workers to handle incoming execution requests.

[DOC](../master/DOC/PThreadPoolDOC.md)

## TaskSystem
Built on top of the PThreadPool allow to create custom tasks and define dependencies among them.

[DOC](../master/DOC/TaskSystemDOC.md)