
#include "gpio_hal.h"
#include <string>
#include <utility>
#include "boost_signals2_wrap.h"

using std::string;

/**
 * Emulates GPIO pins as files in the current directory.
 *
 * digital pins: 0 or 1
 * analog pins: 0 through 4095
 * writing to the file is only possible when the pin is in INPUT mode
 * OUTPUT pins are read only.
 *
 */
template<typename T> struct Pin
{
    virtual void setValue(T value)=0;
    virtual T getValue()=0;

    virtual PinMode getMode()=0;
    virtual void setMode(PinMode mode)=0;

    virtual string name()=0;

    typedef std::pair<T,T> value_range;

    virtual value_range range()=0;

    typedef boost::signals2::signal<void(Pin<T>&)> pin_signal;
    pin_signal notify;

    void changed()
    {
        this->notify(*this);
    }

};

template<typename T> class AbstractPin : public Pin<T>
{
    string pin_name;
    PinMode mode;

    using base_pin = Pin<T>;

public:

    AbstractPin(string name)
    {
        this->pin_name = name;
        mode = INPUT;
    }

    virtual string name() override { return this->pin_name; }

    virtual PinMode getMode() override
    {
        return mode;
    }

    virtual void setMode(PinMode mode) override
    {
        this->mode = mode;
    }

    T constrain(T value)
    {
        typename base_pin::value_range r = this->range();
        if (value<r.first)
            value = r.first;
        if (value>r.second)
            value = r.second;
        return value;
    }
};



static string make_name(const char* prefix, unsigned number)
{
    char buf[10];
    sprintf(buf, "%s%d", prefix, number);
    return buf;
}

/**
 * An in-memory GPIO pin. Exposes a signal instance that can be used to notify clients of change.
 */
template<typename T, T min, T max> class GPIOPin : public AbstractPin<T>
{
    PinMode mode;
    T value;

public:
    using base = AbstractPin<T>;
    using value_range = typename base::value_range;


    GPIOPin(string name) : base(name), mode(PIN_MODE_NONE), value(0)
    {
        this->setMode(INPUT);
        this->setValue(0);
    }

    virtual void setValue(T value) override {
        value = this->constrain(value);
        if (value!=this->value) {
            this->value = value;
            this->changed();
        }
    }

    virtual void setMode(PinMode mode) override {
        if (mode!=this->getMode()) {
            base::setMode(mode);
            this->changed();
        }
    }

    virtual T getValue() override {
        return value;
    }

    virtual value_range range() override
    {
        return value_range(min, max);
    }

};

template<typename T> class GPIOFilePin : public AbstractPin<T>
{
public:

    void setValue(T value)
    {
        value = constrain(value);
    }

};


using std_pin_t = uint16_t;
using DigitalPin = GPIOPin<std_pin_t, 0, 2>;
using AnalogPin = GPIOPin<std_pin_t, 0, 4096>;
using StdPin = Pin<std_pin_t>;


class GpioPinMap
{
    StdPin* pins[24];

    void assignPin(pin_t id, StdPin* pin) {
        pins[id] = pin;
    }

public:
    GpioPinMap()
    {
        for (pin_t i=0; i<8; i++) {
            assignPin(D0+i, new DigitalPin(make_name("D",i)));
        }
        for (pin_t i=0; i<8; i++) {
            assignPin(A0+i, new AnalogPin(make_name("A",i)));
        }
    }

    StdPin& operator[](pin_t index)
    {
        return *pins[index];
    }

    void initialize()
    {
    }
};

GpioPinMap PIN_MAP;


PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction)
{
    return PF_DIO;
}

void HAL_Pin_Mode(pin_t pin, PinMode mode)
{
    PIN_MAP[pin].setMode(mode);
}

PinMode HAL_Get_Pin_Mode(pin_t pin)
{
    return PIN_MAP[pin].getMode();
}

void HAL_GPIO_Write(pin_t pin, uint8_t value)
{
    return PIN_MAP[pin].setValue(value);
}

int32_t HAL_GPIO_Read(pin_t pin)
{
    return PIN_MAP[pin].getValue();
}

uint32_t HAL_Pulse_In(pin_t pin, uint16_t value)
{
	return 0;
}

