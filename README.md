# Mini RTOS Kernel: Priority Inheritance Protocol

A from-scratch simulation of the core internals of a preemptive, fixed-priority
automotive RTOS kernel (the kind of scheduling model behind OSEK/AUTOSAR OS),
built to demonstrate a real, historically significant RTOS bug — **priority
inversion** — and its standard fix, the **Priority Inheritance Protocol**.

> Priority inversion isn't a toy problem: this exact bug caused repeated total
> system resets on NASA's 1997 Mars Pathfinder mission. It remains a classic
> topic in real-time systems and automotive ECU design interviews.

## What this demonstrates

- **A working preemptive priority scheduler** — the kernel always runs the
  highest-priority `READY` task, genuinely preempting lower-priority tasks
  mid-execution as higher-priority tasks arrive.
- **The Priority Inheritance Protocol** — when a high-priority task blocks on
  a mutex held by a lower-priority task, the kernel temporarily boosts the
  holder's effective priority to match, so it can finish and release the
  resource instead of being starved out by unrelated medium-priority tasks.
- **OSEK/AUTOSAR-style task lifecycle** — tasks move through
  `SUSPENDED → READY → RUNNING → BLOCKED → TERMINATED`, activated at simulated
  arrival ticks, mirroring `ActivateTask()` / `TerminateTask()` semantics.
- **A simulated automotive driver layer** — an abstract `Driver` interface
  with `CanBusDriver` and `AdcSensorDriver` implementations, giving the
  shared resource in the demo (the CAN bus mutex) a concrete hardware
  context instead of being an arbitrary lock.

## The scenario

Three tasks contend for a shared CAN-bus mutex:

| Task | Priority | Role |
|---|---|---|
| `EngineControlTask` | 1 (highest) | Needs brief, critical CAN-bus access |
| `InfotainmentTask` | 2 (medium) | Long-running, never touches the bus |
| `DiagnosticsTask` | 3 (lowest) | Grabs the bus first, holds it a while |

**Without priority inheritance**, this produces unbounded priority inversion:
Diagnostics holds the mutex Engine needs, but Diagnostics can't run to finish
and release it because Infotainment — a task with *nothing to do with the
bus* — keeps preempting it. The highest-priority task in the system ends up
stuck behind a low-priority task, indirectly blocked by a medium-priority one.

**With priority inheritance** (implemented here): the instant Engine blocks
on the mutex, Diagnostics' effective priority is boosted to Engine's (the
highest in the system). Diagnostics immediately preempts Infotainment,
finishes quickly, and hands the mutex straight back to Engine — bounding the
inversion instead of letting it run indefinitely.

## Structure

```
include/
  Task.hpp                       # Task, TaskState, TaskStep
  PriorityInheritanceMutex.hpp   # the inheritance-aware mutex
  Scheduler.hpp                  # the preemptive kernel loop
  Driver.hpp                     # simulated CAN bus / ADC driver layer
src/
  Task.cpp
  PriorityInheritanceMutex.cpp
  Scheduler.cpp
  main.cpp                       # stages the priority-inversion scenario
Makefile
```

## Build & run

```bash
make run
```

or manually:

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp src/Task.cpp src/Scheduler.cpp src/PriorityInheritanceMutex.cpp -o rtos_demo
./rtos_demo
```

## Sample output (abridged)

```
[t=0]  DiagnosticsTask ACTIVATED (base priority 3)
[t=0]  DiagnosticsTask ACQUIRES mutex 'CAN_Bus_Mutex'
[t=2]  InfotainmentTask ACTIVATED — preempts DiagnosticsTask
[t=3]  EngineControlTask ACTIVATED — preempts InfotainmentTask
[t=4]  EngineControlTask BLOCKS on mutex, held by DiagnosticsTask
       -> PRIORITY INHERITANCE: DiagnosticsTask boosted to priority 1 (was 3)
[t=4]  SCHEDULED: DiagnosticsTask (effective priority 1)   <- now preempts Infotainment
[t=12] DiagnosticsTask RELEASES mutex (priority restored to 3)
       -> EngineControlTask acquires it next
[t=14] EngineControlTask RELEASES mutex
[t=15] EngineControlTask -> TERMINATED
```

Note how Diagnostics visibly jumps from priority 3 to priority 1 the moment
Engine blocks on it — and drops back to 3 the instant it releases the mutex.

## Personal note

This is a self-study project built to understand real-time operating system
concepts — preemptive scheduling, mutex contention, and priority inheritance
— that come up in automotive/embedded firmware engineering, ahead of an
interview for a Graduate Engineer Trainee role focused on automotive
platform software.
