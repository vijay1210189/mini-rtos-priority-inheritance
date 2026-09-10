#ifndef PRIORITY_INHERITANCE_MUTEX_HPP
#define PRIORITY_INHERITANCE_MUTEX_HPP

#include <string>
#include <vector>
#include "Task.hpp"

// A mutex that implements the Priority Inheritance Protocol: when a
// high-priority task blocks trying to acquire a resource held by a
// lower-priority task, the owner's effective priority is temporarily
// boosted to match the highest-priority waiter. This prevents
// "priority inversion" — the classic RTOS bug where a medium-priority
// task can starve a high-priority task indefinitely by preempting a
// low-priority task that is holding a resource the high-priority task
// needs. (This exact bug caused the Mars Pathfinder's total system
// resets in 1997, and priority inheritance is the textbook fix.)
class PriorityInheritanceMutex {
public:
    explicit PriorityInheritanceMutex(std::string name);

    // Attempts to acquire the mutex on behalf of 'task'. Returns true
    // if acquired immediately. If the mutex is held by a lower or
    // equal priority task, that owner's effective priority is boosted
    // to 'task' priority (inheritance), the requester is queued, and
    // false is returned (caller should mark the task BLOCKED).
    bool tryAcquire(Task* task);

    // Releases the mutex held by 'task'. Restores the releasing
    // task's original priority, and if another task was waiting,
    // hands ownership to the highest-priority waiter (which becomes
    // READY again).
    // Returns the task that should be woken (or nullptr if none).
    Task* release(Task* task);

    Task* getOwner() const { return owner_; }
    const std::string& getName() const { return name_; }

private:
    std::string name_;
    Task* owner_;
    std::vector<Task*> waiting_; // FIFO of tasks blocked on this mutex
};

#endif // PRIORITY_INHERITANCE_MUTEX_HPP
