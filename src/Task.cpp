#include "Task.hpp"

Task::Task(int id, std::string name, int basePriority, int arrivalTick, std::vector<TaskStep> steps)
    : id_(id), name_(std::move(name)), basePriority_(basePriority),
      arrivalTick_(arrivalTick), effectivePriority_(basePriority),
      state_(TaskState::SUSPENDED),
      steps_(std::move(steps)), stepIndex_(0), stepTicksRemaining_(0) {
    if (!steps_.empty() && steps_[0].type == StepType::CPU_BURST) {
        stepTicksRemaining_ = steps_[0].durationTicks;
    }
}

void Task::activate() {
    state_ = TaskState::READY;
    stepIndex_ = 0;
    resetEffectivePriority();
    if (!steps_.empty() && steps_[0].type == StepType::CPU_BURST) {
        stepTicksRemaining_ = steps_[0].durationTicks;
    }
}

void Task::terminate() {
    state_ = TaskState::TERMINATED;
}

void Task::advanceStep() {
    ++stepIndex_;
    // Whenever we land on a new CPU_BURST step — whether advancing
    // from a finished burst, or from an ACQUIRE/RELEASE event handled
    // externally by the Scheduler — (re)initialize its tick counter.
    if (hasMoreSteps() && currentStep().type == StepType::CPU_BURST) {
        stepTicksRemaining_ = currentStep().durationTicks;
    }
}

void Task::runOneTick() {
    // Only meaningful for CPU_BURST steps; ACQUIRE/RELEASE steps are
    // intercepted and handled directly by the Scheduler, which has
    // visibility into the mutex state needed for priority inheritance.
    if (currentStep().type != StepType::CPU_BURST) return;

    if (stepTicksRemaining_ > 0) {
        --stepTicksRemaining_;
    }

    if (stepTicksRemaining_ <= 0) {
        advanceStep();
    }
}

std::string Task::stateToString(TaskState s) {
    switch (s) {
        case TaskState::SUSPENDED:  return "SUSPENDED";
        case TaskState::READY:      return "READY";
        case TaskState::RUNNING:    return "RUNNING";
        case TaskState::BLOCKED:    return "BLOCKED";
        case TaskState::TERMINATED: return "TERMINATED";
    }
    return "UNKNOWN";
}
