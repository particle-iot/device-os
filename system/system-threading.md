
# System Threading

System threading involves running the application and the system task on
separate pre-emptive threads so that the system does not block the application code
from executing for long periods of time.

System threading is compile-time optional and is made available in the system firmware depending
upon the platform and if threading support is available.

System threading is available on the Photon, P1 and Electron starting from firmware release 0.4.6.


## Enabling System Threading on Supported Platforms

When compiled in on a supporting platform, system threading can then be enabled/disabled at runtime.
The default is presently that system threading is disabled, so that it can be included with
our primary system firmware without changing the behavior of existing applications,
while allowing testers to enable the feature on a per-application basis.

System threading is enabled in application code by adding

```
SYSTEM_THREAD(ENABLED);
```

System threading is disabled when the system is in safe-mode.


### System Thread state

The system provides these methods for getting/setting the system thread state:

```
void system_thread_set_state(spark::feature::State feature, void* reserved);
spark::feature::State system_thread_get_state(void*);
```

These are used by the system to take action conditional whether system threading
is enabled.

### System threading behavior

When system threading is enabled, application execution changes compared to
non-threaded execution:

- `setup()` is executed immediately regardless of the system mode, which means
setup may execute before the Netwoork or Cloud is connected.

- System modes `SEMI_AUTOMTIC` and `MANUAL` behave identically - either of these
modes result in the system not automatically starting Networking or a Cloud
connection. `AUTOMATIC` mode connects to the cloud as soon as possible,
but doesn't block the application thread as described in the previous point.

- In `MANUAL` mode there is no need to call `Particle.process()` - when
threading is enabled this is a no-operation. The system thread takes care of
calling `Particle.process()` itself.

- `loop()` is called repeatedly independent from the current state of the
network or cloud connection. The system does not block  `loop()` waiting
for the network or cloud to be available.

- Cloud functions registered with `Particle.function()` execute on the application
thread in between calls to `loop()`. (This is also true in non-threaded mode.)
A long running cloud function will block the application thread (since it is application code)
but not the system code, so cloud connectivity is maintained.
> We are considering an enhancement to `Particle.function()` that would allow
the `Particle.function()` call to specify that the function is executed on a separate
thread, so is not blocked by application code.



## System Threading Design

System threading is realized by using the [Active Object](https://en.wikipedia.org/wiki/Active_object) pattern.
Active Object instances are created for the application loop and the system loop
so these loops execute independently.

The system thread and application thread are given equal priority so the scheduler
will time-slice evenly between them should both be involved in CPU-bound activities.

Here's how the active object pattern works, when the application calls a system method:

- The application makes a function call, such as
`WiFi.connect()`, which calls the system function `network_connect(<parameters>)`

- The system function isn't executed directly, but instead a message is posted to the
system thread queue - 'please run network_connect(<parameters>)`

- The application code is given a future for the function call - an object that
will indicate when the function has executed and any result returned.
The future can be used by the application to
wait for the result (in the case of a synchronous function call), or the
application can simply ignore it for `fire and forget` function calls.
To keep the application unblocked as much as possible, the application code rarely waits on the result,
and hence most functions execute asynchronously.

- The system thread continually pulls requests from it's queue. It eventually gets the
`network_connect()` request posted by the application and the corresponding future.
The system thread executes the requested function with the parameters specified
and sets the result on the future.

- To avoid deadlock due a cyclic [wait-for](https://en.wikipedia.org/wiki/Wait-for_graph graph],
there are rules about which threads can be blocked waiting for
another thread:

 - the application thread may block waiting on the system thread: it can
call system functions synchronously or asynchronously

 - we don't allow the system thread to block waiting on the application thread. All
  application functions called by the system must be called asynchronously.

 - neither thread may call the other while owning a shared (mutex-guarded) resource. This is future
proofing - there are presently no such resources, but we will almost certainly introduce
them later to provide thread-safe access to system peripherals.

This is the case for when system threading is enabled. When disabled, the logic
above is short-circuited and the method call is executed directly by the application thread
(just as it always has been before system threading existed.)

## System Threading Priorities

RTOS task priorities go from 0 (idle task) to 9 (system monitor).
Low priority numbers denote low priority tasks.

On the Photon, the thread priorities are:

 Priority      |      Thread
:-------------:|:---------------:
(highest)<br>9 | Monitor<br>WICED
8              |
7              | Network
6              |
5              | Worker 2
4              |
3              | Worker 1
2              | Application<br>System<br>Timer
1              |
0<br>(lowest)  | Idle


## System Threading Implementation

The implementation of system threading meets these requirements:

- compile-time conditional
- run-time conditional
- minimal code change
- builds on standard C++ threading primitives

### Active Objects

The Active Object pattern is implemented in `system/src/active_object.cpp`, with
most of the implementation being in the header file.

The active object runs a loop that fetches a message from the queue and
executes the function described in the message.

When there are no messages in the queue, a background function is executed.


### Starting the Active Objects

In `system/src/main.cpp` `main()`, the threading status is determined based on
the threading flag and safe mode. When threading is enabled, the system creates
two active objects

- The `SystemThread` object. This runs on a newly created thread. The background function
for the thread is set to `System_Idle()` (the system function called by `Particle.process()`.
The system background loop doesn't have to be executed as often as possible, so the system
will wait for 100 milliseconds for a message to arrive before running the background loop.

- The `ApplicationThread` object. This runs on the calling thread (simply to avoid
creating a new thread unnecessarily.) The background function for the thread
runs the application's `startup()` and `loop()` code. The application loop should
be the primary focus, so the application object only polls the queue for messages
rather than waiting (wait time to take an object from the queue is 0).

### Marshaling Calls to other Threads

Functions that deal with networking and cloud connectivity are designated
system functions. When a system function is called on the system thread,
then it is called directly. When a system function is called on some other thread,
then the call is marshaled to the system thread.

The function itself takes care of marshaling to the system thread if needed. It does
this by adding a small block of code at the start of the function. A system
function like this:

```
void start_the_frobnicator(int how_much)
{
   FROB_NOW = 1;
   twiddle(how_much);
}
```

Then becomes:

```
void start_the_frobnicator(int how_much)
{
   // code to marshal to the system thread
   if (false==SystemThread.isCurrentThread()) {
     auto future = SystemThread.invoke([]-> { start_the_frobnicator(how_much); }));
     return;  // don't wait on future - asynchronous
   }

   FROB_NOW = 1;
   twiddle(how_much);
}
```

The result is that when the function is called on the system thread, it's executed directly.
When the function is not called on the system thread, then post a message to
the system thread to run this function.

That's quite a bit of code to add to each thread (and this elides over details to keep
the example clear.) To avoid boilerplate code in each system method, we
have developed macros to make this simpler.

### Marshaling Macros

The marshaling macros provide a simple way to declare and implement
that a function should execute on the system or application thread,
without having to write boilerplate code.

Let's take `Particle.subscribe()` as an example. Since this performs networking
activity it's run on the system thread. The Wiring API in the application code
calls the system function `spark_subscribe()` in the system dynalib,
which is a good example of a non-trivial system function:

```
bool spark_subscribe(const char *eventName, EventHandler handler, void* handler_data,
        Spark_Subscription_Scope_TypeDef scope, const char* deviceID, void* reserved)
{
    auto event_scope = convert(scope);
    bool success = spark_protocol_add_event_handler(sp, eventName, handler, event_scope, deviceID, handler_data);
    if (success && spark_cloud_flag_connected())
    {
        register_event(eventName, event_scope, deviceID);
    }
    return success;
}
```

To declare that this function should be executed on the system thread, we only
need add one macro:

```
    SYSTEM_THREAD_CONTEXT_SYNC(spark_subscribe(eventName, handler, handler_data, scope, deviceID, reserved));
```

This declares that the function call should be made in the context of the system
thread, and synchronously. The full function then looks like this:


```
bool spark_subscribe(const char *eventName, EventHandler handler, void* handler_data,
        Spark_Subscription_Scope_TypeDef scope, const char* deviceID, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_subscribe(eventName, handler, handler_data, scope, deviceID, reserved));
    auto event_scope = convert(scope);
    bool success = spark_protocol_add_event_handler(sp, eventName, handler, event_scope, deviceID, handler_data);
    if (success && spark_cloud_flag_connected())
    {
        register_event(eventName, event_scope, deviceID);
    }
    return success;
}
```

The macro performing a number of steps:

1. The macro evalutes to empty if system threading isn't compiled in (so the function executes as normal)
2. The macro checks if system threading is enabled, if it's not it runs the rest of the function normally.
3. The macro checks if the current thread is the system thread - if it is, the rest of the function is executed on the calling thread.
4. The macro has determined the need to marshal the call to the system thread:
 - It converts the function call to a lambda object that encapsulates the function call
 - The lambda is wrapped in a message with a future, which is posted to the system thread queue.
 - Since this particular call is synchronous (the function returns a bool value),
 the macro waits on the future for the result, and returns that as the original function call result.

 The macros are defined and described in more detail in `system/inc/system_threading.h`.


### Cloud functions

Cloud functions are registered by the application, triggered by the system in response
to an API call in the cloud.

To ensure application developers don't find themselves stuck in the quagmire of
multi-threaded implementation, we have chosen to keep the default case simple and
have preserved the existing non-threaded behavior regarding Cloud functions:
Cloud functions execute on the application thread in between invocations of loop().
(The alternative is to have the cloud functions execute on a separate thread - this is being considered
as a non-default case that application developers can opt-in.)

Executing cloud
functions on the system thread is not considered since this could lead to application
code blocking the cloud connection, which system threading is intended to prevent.

The cloud communications are handled on the system thread, and so the initial
request from the cloud in the cloud protocol layer is executing on the system
thread. The system thread cannot directly run the application function, so it posts
a message to the application thread to execute the function on the application
thread. The system thread does not wait for execution.  Instead, the system thread
includes in the execution request to the application thread a lambda
that is executed on the application thread after the application Cloud function.
This lambda takes the application result, and passes this together with a lambda from the communications layer
onto the system thread. When executed on the system thread, the lambda from the
cloud communications layer
takes the application function result and the original token of the function call and
sends the appropriate protocol response to the function call.

TODO: a diagram is worth 1000 words! ;-)

As with the system thread marshaling, when no threading is enabled the same sequence
of events happens, but without hopping between threads.

### Non-marshaled functions

Many system functions, such as `network_listening()` `spark_cloud_connect()` are
implemented by setting or retrieving flags. These functions were already asynchronous
by design before system threading, and so they are not marshaled to the system thread - setting or retrieving
these flags on the application thread is sufficient and avoids the overhead of
marshaling the request to another thread.



## Future Improvements

### Avoid Busy-Waiting

Some coding styles have the application thread busy-waiting for events, e.g.

```
WiFi.connect();
while (!WiFi.ready());
```

Busy waiting uses up processing cycles unnecessarily and could be achieved
by blocking the thread until the condition is satisfied.

We have added `waitUntil(WiFi.connected);` in anticipation of this, although
presently the implementation still uses busy waiting.


### Running Cloud Functions on a separate thread

We might allow the user to register Cloud functions to `Particle.subscribe` and
`Particle.function` so that they execute on separate thread.

This would require:

- a new syntax to declare these functions run on a "function thread".
- a new function thread active object to execute application callback functions
- documentation, education that these functions behave more like ISRs and so
should expect to run concurrently with application code

The ideal syntax is to declare the handler asynchronous:

```
Particle.subscribe("event", asynchronous(handler));
```

I don't know if this is feasible.

This is not as nice, but definitely feasible:

```
Particle.subscribe(CALL_ASYNCHRONOUS, "event", handler);
```


### Provide "I don't care" call for functions returning results.

`Particle.subscribe()` returns a true/false success result. Most code is written
that doesn't check this result, so there's little point waiting for it, particularly
as that waiting will block the application thread while the system connects to the network/cloud.

A couple of solutions:

1. add asynchronous versions for each function (not jazzed about this):

```
Particle.subscribeAsync("event", theHandler);
```

or

```
Particle.subscribe("event", theHandler, ASYNC);
```

(not to be confused with the use of CALL_ASYNCHRONOUS in the previous section - here the registration function
is asynchronous, while the previous section discussed calling the registered function asynchronously.)


2. Have a generic way to run code asynchronously on the system thread

```
ON_SYSTEM_THREAD(
   Particle.subscribe("event", theHandler);
   Particle.subscribe("metoo", anotherHandler);
);
```


