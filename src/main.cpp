#include <iostream>
#include <memory>
#include "Task.hpp"
#include "Scheduler.hpp"
#include "PriorityInheritanceMutex.hpp"
#include "Driver.hpp"

// ---------------------------------------------------------------
// Demo scenario: the classic RTOS "priority inversion" problem,
// and its fix via the Priority Inheritance Protocol.
//
// Three tasks share a CAN-bus mutex, modeled after a real
// automotive setup:
//   - EngineControlTask   (highest priority — priority 1)
//   - InfotainmentTask    (medium priority  — priority 2)
//   - DiagnosticsTask     (lowest priority  — priority 3)
//
// Timeline:
//   t=0  DiagnosticsTask starts, immediately acquires the CAN bus
//        mutex, and begins a long low-priority diagnostic read.
//   t=2  InfotainmentTask arrives. Being higher priority than
//        Diagnostics, it PREEMPTS it — even though Diagnostics is
//        holding a resource a critical task will soon need.
//   t=3  EngineControlTask arrives — the highest-priority task in
//        the system. It does brief setup, then tries to acquire the
//        CAN bus mutex... and BLOCKS, because Diagnostics still
//        holds it.
//
// Without priority inheritance, this is a textbook priority
// inversion: the highest-priority task (Engine) is stuck waiting on
// the lowest-priority task (Diagnostics) to finish, but Diagnostics
// itself can't run because the *medium*-priority task (Infotainment)
// keeps preempting it. Engine ends up effectively blocked behind a
// task with lower priority than itself for an unbounded time — this
// exact bug caused repeated total system resets on the 1997 Mars
// Pathfinder mission.
//
// With priority inheritance: the moment Engine blocks on the mutex,
// Diagnostics' effective priority is boosted to match Engine's (the
// highest in the system). This lets Diagnostics preempt Infotainment,
// finish quickly, release the mutex, and hand control straight back
// to Engine — bounding the priority inversion instead of letting it
// run indefinitely.
// ---------------------------------------------------------------

int main() {
    // Simulated automotive driver layer — instantiated to give the
    // scenario a concrete hardware-facing context, even though the
    // scheduling demo itself only needs the shared mutex.
    CanBusDriver canBus;
    canBus.init();
    AdcSensorDriver wheelSpeedSensor("WheelSpeedSensor");
    wheelSpeedSensor.init();

    std::cout << "\n";

    PriorityInheritanceMutex canBusMutex("CAN_Bus_Mutex");

    Scheduler scheduler;

    // DiagnosticsTask (lowest priority = 3): acquires the bus first
    // and holds it for a long, low-priority diagnostic read.
    auto diagnosticsTask = std::make_shared<Task>(
        1, "DiagnosticsTask", /*basePriority=*/3, /*arrivalTick=*/0,
        std::vector<TaskStep>{
            {StepType::ACQUIRE_MUTEX, 0, &canBusMutex},
            {StepType::CPU_BURST, 10, nullptr},
            {StepType::RELEASE_MUTEX, 0, &canBusMutex},
            {StepType::CPU_BURST, 1, nullptr}
        });

    // InfotainmentTask (medium priority = 2): doesn't touch the bus
    // at all, but its mere presence is what turns simple blocking
    // into unbounded priority inversion, by repeatedly preempting
    // Diagnostics while it holds the mutex Engine needs.
    auto infotainmentTask = std::make_shared<Task>(
        2, "InfotainmentTask", /*basePriority=*/2, /*arrivalTick=*/2,
        std::vector<TaskStep>{
            {StepType::CPU_BURST, 8, nullptr}
        });

    // EngineControlTask (highest priority = 1): brief setup, then
    // needs the CAN bus for a short, critical control-loop update.
    auto engineTask = std::make_shared<Task>(
        3, "EngineControlTask", /*basePriority=*/1, /*arrivalTick=*/3,
        std::vector<TaskStep>{
            {StepType::CPU_BURST, 1, nullptr},
            {StepType::ACQUIRE_MUTEX, 0, &canBusMutex},
            {StepType::CPU_BURST, 2, nullptr},
            {StepType::RELEASE_MUTEX, 0, &canBusMutex},
            {StepType::CPU_BURST, 1, nullptr}
        });

    scheduler.addTask(diagnosticsTask);
    scheduler.addTask(infotainmentTask);
    scheduler.addTask(engineTask);

    scheduler.run(/*totalTicks=*/30);

    return 0;
}
