#ifndef DRIVER_HPP
#define DRIVER_HPP

#include <string>
#include <iostream>

// Abstract driver interface, mirroring the shape of a real automotive
// driver layer (e.g. an AUTOSAR MCAL/driver module): every concrete
// driver implements init(), read(), and write() against a simulated
// peripheral register/bus.
class Driver {
public:
    explicit Driver(std::string name) : name_(std::move(name)), initialized_(false) {}
    virtual ~Driver() = default;

    virtual void init() = 0;
    virtual int read() = 0;
    virtual void write(int value) = 0;

    const std::string& getName() const { return name_; }

protected:
    std::string name_;
    bool initialized_;
};

// Simulated CAN bus driver. In a real ECU this would talk to a CAN
// controller peripheral (e.g. Bosch M_CAN) over memory-mapped
// registers; here it simulates a shared bus register that tasks
// read/write while holding the bus mutex.
class CanBusDriver : public Driver {
public:
    CanBusDriver() : Driver("CAN_Bus"), busRegister_(0) {}

    void init() override {
        initialized_ = true;
        busRegister_ = 0;
        std::cout << "  [Driver] " << name_ << " initialized.\n";
    }

    int read() override {
        return busRegister_;
    }

    void write(int value) override {
        busRegister_ = value;
        std::cout << "  [Driver] " << name_ << " bus register <- " << value << "\n";
    }

private:
    int busRegister_;
};

// Simulated ADC sensor driver (e.g. wheel-speed or coolant-temp
// sensor input), used conceptually by the ADC polling task.
class AdcSensorDriver : public Driver {
public:
    explicit AdcSensorDriver(std::string sensorName)
        : Driver(std::move(sensorName)), lastSample_(0) {}

    void init() override {
        initialized_ = true;
        lastSample_ = 0;
        std::cout << "  [Driver] " << name_ << " ADC initialized.\n";
    }

    int read() override {
        // Simulated raw ADC sample.
        lastSample_ = (lastSample_ + 37) % 4096;
        return lastSample_;
    }

    void write(int /*value*/) override {
        // ADC input channels are read-only; no-op provided to satisfy
        // the common Driver interface.
    }

private:
    int lastSample_;
};

#endif // DRIVER_HPP
