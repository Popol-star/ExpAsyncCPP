# ExpAsyncCPP
A simple C++20 experimental async library 

The repository aim to show a possible way to implement a asynchronous environment using coroutines.

Async.h contains:\
-AsyncCoroutine: coroutines used in this library.\
-Executor: Interface defining an coroutines executors.\
-Awaitable: Template abstract class defining a waitable premitive.

AsyncExecutor.h/cpp contains:\
Executor implementations.\
-A blocking executor (block the thread).\
-A nonblocking executor (Can be usefull for video games).

SingleShot.h contains:\
-Single producer, single consumer messaging premitive. Only one element can be sended.\
This premitive is used to send a message threadsafely.

UnboundedSPSC.h contains:\
-Single producer, single consumer messaging premitive. Multiple elements can be sended.\
This premitive is used to send a messages threadsafely.