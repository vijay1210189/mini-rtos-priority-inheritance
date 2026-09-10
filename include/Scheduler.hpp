#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <vector>
#include <memory>
#include "Task.hpp"

// A minimal preemptive, fixed-priority RTOS kernel simulation.
// Each tick, the scheduler selects the READY task with the highest
// effective priority (lowest number) and runs it for one tick —
// genuinely preempting a lower-priority task if a higher-priority
// one becomes READY. ACQUIRE_MUTEX / RELEASE_MUTEX steps are
// intercepted here (rather than inside Task) because resolving them
// correctly requires visibility into mutex ownership and the
// priority-inheritance boost logic.
class Scheduler {
public:
    void addTask(std::shared_ptr<Task> task);

    // Runs the simulation for a fixed number of ticks, printing a
    // timeline of scheduling decisions, preemptions, blocking events,
    // and priority-inheritance boosts as they happen.
    void run(int totalTicks);

private:
    Task* pickHighestPriorityReadyTask();
    void handleNonCpuStep(Task* task, int currentTick);

    std::vector<std::shared_ptr<Task>> tasks_;
    Task* currentRunningTask_ = nullptr;
};

#endif // SCHEDULER_HPP
