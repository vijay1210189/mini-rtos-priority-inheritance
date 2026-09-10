#include "Scheduler.hpp"
#include "PriorityInheritanceMutex.hpp"
#include <iostream>
#include <algorithm>

void Scheduler::addTask(std::shared_ptr<Task> task) {
    tasks_.push_back(std::move(task));
}

Task* Scheduler::pickHighestPriorityReadyTask() {
    Task* best = nullptr;
    for (auto& t : tasks_) {
        if (t->getState() != TaskState::READY) continue;
        if (!best || t->getEffectivePriority() < best->getEffectivePriority()) {
            best = t.get();
        }
    }
    return best;
}

void Scheduler::run(int totalTicks) {
    std::cout << "=========================================\n";
    std::cout << " Mini RTOS Kernel Simulation\n";
    std::cout << " (Preemptive fixed-priority scheduling +\n";
    std::cout << "  Priority Inheritance Protocol)\n";
    std::cout << "=========================================\n";

    for (int tick = 0; tick < totalTicks; ++tick) {
        // OSEK/AUTOSAR-style ActivateTask(): any task whose arrival
        // tick has been reached transitions SUSPENDED -> READY.
        for (auto& t : tasks_) {
            if (t->getState() == TaskState::SUSPENDED && t->getArrivalTick() <= tick) {
                t->activate();
                std::cout << "[t=" << tick << "] " << t->getName()
                          << " ACTIVATED (base priority " << t->getBasePriority() << ")\n";
            }
        }

        // Resolve any instantaneous ACQUIRE/RELEASE transitions before
        // spending CPU time this tick. These don't consume simulated
        // execution time — only CPU_BURST steps do.
        bool progressed = true;
        while (progressed) {
            progressed = false;
            Task* candidate = pickHighestPriorityReadyTask();
            if (!candidate) break;

            if (!candidate->hasMoreSteps()) {
                candidate->terminate();
                std::cout << "[t=" << tick << "] " << candidate->getName()
                          << " has no more steps -> TERMINATED\n";
                progressed = true;
                continue;
            }

            TaskStep& step = candidate->currentStep();

            if (step.type == StepType::ACQUIRE_MUTEX) {
                bool acquired = step.mutex->tryAcquire(candidate);
                if (acquired) {
                    std::cout << "[t=" << tick << "] " << candidate->getName()
                              << " ACQUIRES mutex '" << step.mutex->getName() << "'\n";
                    candidate->advanceStep();
                } else {
                    candidate->setState(TaskState::BLOCKED);
                    Task* owner = step.mutex->getOwner();
                    std::cout << "[t=" << tick << "] " << candidate->getName()
                              << " BLOCKS on mutex '" << step.mutex->getName()
                              << "' held by " << owner->getName() << "\n";
                    if (owner->getEffectivePriority() == candidate->getEffectivePriority()) {
                        std::cout << "         -> PRIORITY INHERITANCE: " << owner->getName()
                                  << " boosted to priority " << owner->getEffectivePriority()
                                  << " (was " << owner->getBasePriority() << ")\n";
                    }
                }
                progressed = true;
                continue;
            }

            if (step.type == StepType::RELEASE_MUTEX) {
                Task* woken = step.mutex->release(candidate);
                std::cout << "[t=" << tick << "] " << candidate->getName()
                          << " RELEASES mutex '" << step.mutex->getName()
                          << "' (priority restored to " << candidate->getEffectivePriority() << ")\n";
                candidate->advanceStep();
                if (woken) {
                    woken->setState(TaskState::READY);
                    std::cout << "         -> " << woken->getName()
                              << " acquires it next and is now READY (priority "
                              << woken->getEffectivePriority() << ")\n";
                }
                progressed = true;
                continue;
            }

            break; // CPU_BURST step: handled below, consumes real tick time
        }

        Task* runner = pickHighestPriorityReadyTask();
        if (!runner) {
            std::cout << "[t=" << tick << "] (idle)\n";
            continue;
        }

        bool wasAlreadyRunning = (currentRunningTask_ == runner);
        if (!wasAlreadyRunning) {
            std::cout << "[t=" << tick << "] SCHEDULED: " << runner->getName()
                      << " (effective priority " << runner->getEffectivePriority() << ")";
            if (currentRunningTask_ && currentRunningTask_ != runner &&
                currentRunningTask_->getState() == TaskState::READY) {
                std::cout << "  [preempted " << currentRunningTask_->getName() << "]";
            }
            std::cout << "\n";
        }
        currentRunningTask_ = runner;

        runner->setState(TaskState::RUNNING);
        runner->runOneTick();

        if (runner->isFinished()) {
            std::cout << "[t=" << tick << "] " << runner->getName() << " -> TERMINATED\n";
            currentRunningTask_ = nullptr;
        } else {
            runner->setState(TaskState::READY);
        }
    }

    std::cout << "\n--- Final task states ---\n";
    for (auto& t : tasks_) {
        std::cout << "  " << t->getName() << ": " << Task::stateToString(t->getState()) << "\n";
    }
}
