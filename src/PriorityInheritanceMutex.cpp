#include "PriorityInheritanceMutex.hpp"
#include <algorithm>

PriorityInheritanceMutex::PriorityInheritanceMutex(std::string name)
    : name_(std::move(name)), owner_(nullptr) {}

bool PriorityInheritanceMutex::tryAcquire(Task* task) {
    if (owner_ == nullptr) {
        owner_ = task;
        return true;
    }

    if (owner_ == task) {
        return true; // already owns it (not expected in this demo, but safe)
    }

    // Mutex is held by someone else: queue this task, and apply
    // priority inheritance if the requester has a numerically lower
    // (i.e. more urgent) priority than the current owner's effective
    // priority. Lower number = higher priority (OSEK convention).
    waiting_.push_back(task);

    if (task->getEffectivePriority() < owner_->getEffectivePriority()) {
        owner_->setEffectivePriority(task->getEffectivePriority());
    }

    return false;
}

Task* PriorityInheritanceMutex::release(Task* task) {
    if (owner_ != task) {
        return nullptr; // releasing a mutex we don't own; no-op in this demo
    }

    // Restore the releasing task's own priority — any boost it
    // received from inheritance no longer applies once it gives up
    // the resource.
    task->resetEffectivePriority();

    if (waiting_.empty()) {
        owner_ = nullptr;
        return nullptr;
    }

    // Hand ownership to the highest-priority (lowest number) waiter.
    auto it = std::min_element(waiting_.begin(), waiting_.end(),
        [](Task* a, Task* b) { return a->getEffectivePriority() < b->getEffectivePriority(); });

    Task* nextOwner = *it;
    waiting_.erase(it);
    owner_ = nextOwner;
    nextOwner->resetEffectivePriority(); // it now owns the resource at its own priority

    return nextOwner; // caller (Scheduler) should move this task back to READY
}
