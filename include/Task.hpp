#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <vector>

class PriorityInheritanceMutex; // forward declaration

// AUTOSAR/OSEK-style task states.
enum class TaskState {
    SUSPENDED,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// A step in a task's workload: either a plain CPU burst, or an
// attempt to acquire/release a shared resource (mutex). Modeling
// tasks as a sequence of steps lets the kernel simulate real
// contention over a shared automotive bus resource.
enum class StepType {
    CPU_BURST,
    ACQUIRE_MUTEX,
    RELEASE_MUTEX
};

struct TaskStep {
    StepType type;
    int durationTicks = 0;             // used for CPU_BURST
    PriorityInheritanceMutex* mutex = nullptr; // used for ACQUIRE/RELEASE
};

// Represents one schedulable unit of work, analogous to an
// AUTOSAR OS task (ActivateTask / TerminateTask lifecycle).
// basePriority_ is the task's configured static priority;
// effectivePriority_ can be temporarily boosted by priority
// inheritance while the task holds a resource another, higher
// priority task is waiting on.
class Task {
public:
    Task(int id, std::string name, int basePriority, int arrivalTick, std::vector<TaskStep> steps);

    void activate();     // OSEK/AUTOSAR-style ActivateTask()
    void terminate();    // OSEK/AUTOSAR-style TerminateTask()

    // Runs the task for a single scheduler tick. Returns true if the
    // task blocked or completed a step and the scheduler should
    // re-evaluate who runs next.
    void runOneTick();

    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    int getBasePriority() const { return basePriority_; }
    int getArrivalTick() const { return arrivalTick_; }
    int getEffectivePriority() const { return effectivePriority_; }
    void setEffectivePriority(int p) { effectivePriority_ = p; }
    void resetEffectivePriority() { effectivePriority_ = basePriority_; }

    TaskState getState() const { return state_; }
    void setState(TaskState s) { state_ = s; }

    bool isFinished() const { return state_ == TaskState::TERMINATED; }

    TaskStep& currentStep() { return steps_[stepIndex_]; }
    bool hasMoreSteps() const { return stepIndex_ < steps_.size(); }
    void advanceStep(); // declared here, defined in Task.cpp (resets burst duration as needed)

    static std::string stateToString(TaskState s);

private:
    int id_;
    std::string name_;
    int basePriority_;       // lower number = higher priority (OSEK convention)
    int arrivalTick_;        // simulated tick at which this task is activated
    int effectivePriority_;  // may be temporarily boosted (priority inheritance)
    TaskState state_;
    std::vector<TaskStep> steps_;
    size_t stepIndex_;
    int stepTicksRemaining_;
};

#endif // TASK_HPP
