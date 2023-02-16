/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "rtl8721d.h"
#ifdef __cplusplus
}
#endif
#include "interrupts_hal.h"
#include "interrupts_irq.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "check.h"
#include "scope_guard.h"
#include "module_info.h"

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#include "atomic_section.h"
#include "spark_wiring_vector.h"
using spark::Vector;
#endif

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
#include "mcp23s17.h"
#endif
using namespace particle;
#endif

#include <algorithm>

extern uintptr_t link_ram_interrupt_vectors_location[];
static uint32_t hal_interrupts_handler_backup[MAX_VECTOR_TABLE_NUM] = {};

namespace {

typedef enum interrupt_state_t {
    INT_STATE_DISABLED,
    INT_STATE_ENABLED,
    INT_STATE_SUSPENDED
} interrupt_state_t;

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

class InterruptConfig {
public:
    struct InterruptCallback {
        bool operator==(const InterruptCallback& callback) {
            return handler == callback.handler;
        }

        bool operator!=(const InterruptCallback& callback) {
            return handler != callback.handler;
        }

        hal_interrupt_handler_t handler = nullptr;
        void* data = nullptr;
        uint8_t chainPriority = 0; // The lower the value, the higher the priority
    };

    InterruptConfig()
            : state(INT_STATE_DISABLED),
              callbacks_() {
    }
    ~InterruptConfig() = default;

    void sortHandlersByPriority() {
        // NOTE: has to be called under ATOMIC_BLOCK
        std::sort(callbacks_.begin(), callbacks_.end(), [](const InterruptCallback& lhs, const InterruptCallback& rhs) {
            // Ascending order
            return lhs.chainPriority < rhs.chainPriority;
        });
    }

    int appendHandler(hal_interrupt_handler_t handler, void* data, uint8_t chainPriority) {
        CHECK_TRUE(handler, SYSTEM_ERROR_INVALID_ARGUMENT);

        InterruptCallback cb;
        cb.handler = handler;
        cb.data = data;
        cb.chainPriority = chainPriority;
        bool needAppend = true;
        ATOMIC_BLOCK() {
            for (auto& callback: callbacks_) {
                if (!callback.handler) {
                    callback = cb;
                    // Used an empty entry
                    sortHandlersByPriority();
                    needAppend = false;
                    break;
                }
            }
        }
        if (!needAppend) {
            return 0;
        }

        // Need to increase the size of the vector, which we cannot do from an ISR context
        CHECK_FALSE(hal_interrupt_is_isr(), SYSTEM_ERROR_INVALID_STATE);
        decltype(callbacks_) temp;
        bool done = false;
        while (!done) {
            CHECK_TRUE(temp.reserve(callbacks_.capacity() + 1), SYSTEM_ERROR_NO_MEMORY);
            ATOMIC_BLOCK() {
                if (temp.capacity() == (callbacks_.capacity() + 1)) {
                    // No modifications occured, we can safely copy here
                    temp.insert(0, callbacks_);
                    // This will not cause an realloc call, as we've already reserved + 1 size
                    temp.append(cb);
                    std::swap(temp, callbacks_);
                    sortHandlersByPriority();
                    done = true;
                }
            }
            if (done) {
                break;
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    ssize_t handlers() const {
        size_t count = 0;
        ATOMIC_BLOCK() { 
            for (const auto& callback: callbacks_) {
                if (callback.handler) {
                    count++;
                }
            }
        }
        return count;
    }

    int removeHandler(hal_interrupt_handler_t handler) {
        CHECK_TRUE(handler, SYSTEM_ERROR_INVALID_ARGUMENT);
        ATOMIC_BLOCK() { 
            for (auto& callback: callbacks_) {
                if (callback.handler == handler) {
                    callback = InterruptCallback();
                }
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    void clearHandlers() {
        ATOMIC_BLOCK() { 
            for (auto& callback: callbacks_) {
                callback = InterruptCallback();
            }
        }
    }

    void executeHandlers() const {
        for (auto& callback : callbacks_) {
            if (callback.handler) {
                callback.handler(callback.data);
            }
        }
    }

public:
    interrupt_state_t state;
    InterruptMode mode;

private:
    Vector<InterruptCallback> callbacks_;
};

InterruptConfig interruptsConfig[TOTAL_PINS];

#else

struct {
    interrupt_state_t           state;
    hal_interrupt_callback_t    callback;
    InterruptMode               mode;
} interruptsConfig[TOTAL_PINS];

#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

int parseMode(InterruptMode mode, uint32_t* trigger, uint32_t* polarity) {
    switch(mode) {
        case RISING: {
            *trigger = GPIO_INT_Trigger_EDGE;
            *polarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
            break;
        }
        case FALLING: {
            *trigger = GPIO_INT_Trigger_EDGE;
            *polarity = GPIO_INT_POLARITY_ACTIVE_LOW;
            break;
        }
        case CHANGE: {
            *trigger = GPIO_INT_Trigger_BOTHEDGE;
            *polarity = GPIO_INT_POLARITY_ACTIVE_LOW;
            break;
        }
        default: {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
    }
    return SYSTEM_ERROR_NONE;
}

void gpioIntHandler(void* data) {
    uint16_t pin = (uint32_t)data;
    if (!hal_pin_is_valid(pin)) {
        return;
    }
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    interruptsConfig[pin].executeHandlers();
#else
    if (interruptsConfig[pin].callback.handler) {
        interruptsConfig[pin].callback.handler(interruptsConfig[pin].callback.data);
    }
#endif
}

// Particle implementation of GPIO_INTMode (without the PAD_PullCtrl() call)
void GPIO_INTMode_HAL(u32 GPIO_Pin, u32 GPIO_Port, u32 GPIO_ITEnable, u32 GPIO_ITTrigger, u32 GPIO_ITPolarity, u32 GPIO_ITDebounce) {
    uint32_t pinMask = 1 << (GPIO_Pin & 0x1f);
    uint32_t portBase = (GPIO_Port == RTL_PORT_A) ? (uint32_t)GPIOA_BASE : (uint32_t)GPIOB_BASE;
    uint32_t tempReg = 0;

    // Explicitly do NOT set PULL UP/PULL DOWN PADCTL register

    // SET GPIO_INT_BOTHEDGE reg
    tempReg = HAL_READ32(portBase, 0x68);
    if (GPIO_ITTrigger == GPIO_INT_Trigger_BOTHEDGE) {
        HAL_WRITE32(portBase, 0x68, tempReg | pinMask);
    } else {
        HAL_WRITE32(portBase, 0x68, tempReg & ~pinMask);
        // SET GPIO_INTTYPE_LEVEL reg only if not BOTH EDGE
        tempReg = HAL_READ32(portBase, 0x38);
        if (GPIO_ITTrigger == GPIO_INT_Trigger_EDGE) {
            HAL_WRITE32(portBase, 0x38, tempReg | pinMask);
        } else {
            HAL_WRITE32(portBase, 0x38, tempReg & ~pinMask);
        }
    }
    // SET GPIO_INT_POLARITY reg
    tempReg = HAL_READ32(portBase, 0x3c);
    if (GPIO_ITPolarity == GPIO_INT_POLARITY_ACTIVE_HIGH) {
        HAL_WRITE32(portBase, 0x3c, tempReg | pinMask);
    } else {
        HAL_WRITE32(portBase, 0x3c, tempReg & ~pinMask);
    }
    // SET GPIO_DEBOUNCE reg
    tempReg = HAL_READ32(portBase, 0x48);
    if (GPIO_ITDebounce == GPIO_INT_DEBOUNCE_ENABLE) {
        HAL_WRITE32(portBase, 0x48, tempReg | pinMask);
    } else {
        HAL_WRITE32(portBase, 0x48, tempReg & ~pinMask);
    }
    // SET GPIO_INTEN reg
    tempReg = HAL_READ32(portBase, 0x30);
    if (GPIO_ITEnable == ENABLE) {
        HAL_WRITE32(portBase, 0x30, tempReg | pinMask);
    } else {
        HAL_WRITE32(portBase, 0x30, tempReg & ~pinMask);
    }
}

// Particle implementation of GPIO_Init (which calls our GPIO_INTMode_HAL to avoid PAD_PullCtrl() call)
// NOTE: Only for use with GPIO_Mode_INT, do not use for GPIO_Mode_IN/GPIO_Mode_OUT
// NOTE: hal_pin_is_valid(pin) must be called prior to this!
void GPIO_Init_HAL(GPIO_InitTypeDef *GPIO_InitStruct, u32 GPIO_Port) {
    if (GPIO_InitStruct->GPIO_Mode != GPIO_Mode_INT) {
        return;
    }

    Pinmux_Config(GPIO_InitStruct->GPIO_Pin, PINMUX_FUNCTION_GPIO); // force GPIO mode
    GPIO_INTMode_HAL(GPIO_InitStruct->GPIO_Pin, GPIO_Port, 1 /*ENABLE*/, GPIO_InitStruct->GPIO_ITTrigger,
            GPIO_InitStruct->GPIO_ITPolarity, GPIO_InitStruct->GPIO_ITDebounce);
}

} // anonymous

void hal_interrupt_init(void) {
    // FIXME: when io expender is used, the TOTAL_PINS includes the pins on the expender
    for (int i = 0; i < TOTAL_PINS; i++) {
        interruptsConfig[i].state = INT_STATE_DISABLED;
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
        interruptsConfig[i].callback.handler = nullptr;
        interruptsConfig[i].callback.data = nullptr;
#endif
    }
#if defined (ARM_CORE_CM0)
    // Just in case
    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
#endif
}

void hal_interrupt_uninit(void) {
    // FIXME: when io expender is used, the TOTAL_PINS includes the pins on the expender
    for (int i = 0; i < TOTAL_PINS; i++) {
        if (interruptsConfig[i].state == INT_STATE_ENABLED) {
            const uint32_t rtlPin = hal_pin_to_rtl_pin(i);
            GPIO_INTMode(rtlPin, DISABLE, 0, 0, 0);
            GPIO_INTConfig(rtlPin, DISABLE);
            interruptsConfig[i].state = INT_STATE_DISABLED;
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
            interruptsConfig[i].callback.handler = nullptr;
            interruptsConfig[i].callback.data = nullptr;
#endif
        }
    }
}

// FIXME: attaching interrupt on KM4 side will also enable interrupt on KM0 (since it can wake up KM0 for now)?
int hal_interrupt_attach(uint16_t pin, hal_interrupt_handler_t handler, void* data, InterruptMode mode, hal_interrupt_extra_configuration_t* config) {
    CHECK_TRUE(hal_pin_is_valid(pin), SYSTEM_ERROR_INVALID_ARGUMENT);
    if (config && (config->version < HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_3 && config->appendHandler)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    hal_pin_info_t* pinInfo = hal_pin_map() + pin;

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (pinInfo->type == HAL_PIN_TYPE_MCU) {
#endif
        const uint32_t rtlPin = hal_pin_to_rtl_pin(pin);

        if ((pinInfo->gpio_port == RTL_PORT_A && pinInfo->gpio_pin == 27) ||
                (pinInfo->gpio_port == RTL_PORT_B && pinInfo->gpio_pin == 3)) {
            Pinmux_Swdoff();
        }

        GPIO_INTConfig(rtlPin, DISABLE);

        GPIO_InitTypeDef  GPIO_InitStruct = {};
        GPIO_InitStruct.GPIO_Pin = rtlPin;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_INT;
        parseMode(mode, &GPIO_InitStruct.GPIO_ITTrigger, &GPIO_InitStruct.GPIO_ITPolarity);

        if (pinInfo->gpio_port == RTL_PORT_A) {
            InterruptRegister(GPIO_INTHandler, GPIOA_IRQ, (u32)GPIOA_BASE, 5);		
            InterruptEn(GPIOA_IRQ, 5);
        } else if (pinInfo->gpio_port == RTL_PORT_B) {
            InterruptRegister(GPIO_INTHandler, GPIOB_IRQ, (u32)GPIOB_BASE, 5);		
            InterruptEn(GPIOB_IRQ, 5);
        } else {
            // Should not get here
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }

        GPIO_Init_HAL(&GPIO_InitStruct, pinInfo->gpio_port);
        GPIO_UserRegIrq(rtlPin, (VOID*)gpioIntHandler, (void*)((uint32_t)pin));
        GPIO_INTMode_HAL(rtlPin, pinInfo->gpio_port, ENABLE, GPIO_InitStruct.GPIO_ITTrigger, GPIO_InitStruct.GPIO_ITPolarity, GPIO_INT_DEBOUNCE_ENABLE);

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        if (!config || (config && !(config->appendHandler))) {
            interruptsConfig[pin].clearHandlers();
        }
        if (config) {
            CHECK(interruptsConfig[pin].appendHandler(handler, data, config->chainPriority));
        } else {
            CHECK(interruptsConfig[pin].appendHandler(handler, data, 0));
        }
#else
        interruptsConfig[pin].callback.handler = handler;
        interruptsConfig[pin].callback.data = data;
#endif
        interruptsConfig[pin].state = INT_STATE_ENABLED;
        interruptsConfig[pin].mode = mode;
        hal_pin_set_function(pin, PF_DIO);

        GPIO_INTConfig(rtlPin, ENABLE);

        return SYSTEM_ERROR_NONE;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (pinInfo->type == HAL_PIN_TYPE_IO_EXPANDER) {
        return Mcp23s17::getInstance().attachPinInterrupt(pinInfo->gpio_port, pinInfo->gpio_pin, mode, static_cast<particle::Mcp23s17InterruptCallback>(handler), data);
    }
#endif
    else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif
}

int hal_interrupt_detach(uint16_t pin) {
    return hal_interrupt_detach_ext(pin, 0, nullptr);
}

int hal_interrupt_detach_ext(uint16_t pin, uint8_t keepHandler, void* reserved/*handler*/) {
    CHECK_TRUE(hal_pin_is_valid(pin), SYSTEM_ERROR_INVALID_ARGUMENT);
    
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    if (pinInfo->type == HAL_PIN_TYPE_MCU) {
#endif

        bool disable = true;
        if (!keepHandler) {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
            auto handler = (hal_interrupt_handler_t)reserved;
            if (handler) {
                interruptsConfig[pin].removeHandler(handler);
            } else {
                interruptsConfig[pin].clearHandlers();
            }
            if (interruptsConfig[pin].handlers() > 0) {
                disable = false;
            }
#else
            interruptsConfig[pin].callback.handler = nullptr;
            interruptsConfig[pin].callback.data = nullptr;
#endif
        }

        if (disable) {
            const uint32_t rtlPin = hal_pin_to_rtl_pin(pin);
            GPIO_INTMode(rtlPin, DISABLE, 0, 0, 0);
            GPIO_INTConfig(rtlPin, DISABLE);
            interruptsConfig[pin].state = INT_STATE_DISABLED;
        }

        hal_pin_set_function(pin, PF_NONE);

        return SYSTEM_ERROR_NONE;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (pinInfo->type == HAL_PIN_TYPE_IO_EXPANDER) {
        return Mcp23s17::getInstance().detachPinInterrupt(pinInfo->gpio_port, pinInfo->gpio_pin);
    } 
#endif
    else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif
}

void hal_interrupt_enable_all(void) {
    // FIXME: this only enables GPIO interrupt, while the API name is ambiguous
    NVIC_EnableIRQ(GPIOA_IRQ);
    NVIC_EnableIRQ(GPIOB_IRQ);
}

void hal_interrupt_disable_all(void) {
    // FIXME: this only disables GPIO interrupt, while the API name is ambiguous
    NVIC_DisableIRQ(GPIOA_IRQ);
    NVIC_DisableIRQ(GPIOB_IRQ);
}

void hal_interrupt_suspend(void) {
    for (int i = 0; i < TOTAL_PINS; i++) {
        if (interruptsConfig[i].state == INT_STATE_ENABLED) {
            const uint32_t rtlPin = hal_pin_to_rtl_pin(i);
            GPIO_INTMode(rtlPin, DISABLE, 0, 0, 0);
            GPIO_INTConfig(rtlPin, DISABLE);
            interruptsConfig[i].state = INT_STATE_SUSPENDED;
        }
    }
}

void hal_interrupt_restore(void) {
    for (int i = 0; i < TOTAL_PINS; i++) {
        const uint32_t rtlPin = hal_pin_to_rtl_pin(i);
        // In case that interrupt is enabled by sleep
        GPIO_INTConfig(rtlPin, DISABLE);
        if (interruptsConfig[i].state == INT_STATE_SUSPENDED) {
            uint32_t trigger = 0, polarity = 0;
            parseMode(interruptsConfig[i].mode, &trigger, &polarity);
            GPIO_INTMode(rtlPin, ENABLE, trigger, polarity, GPIO_INT_DEBOUNCE_ENABLE);
            GPIO_INTConfig(rtlPin, ENABLE);
            interruptsConfig[i].state = INT_STATE_ENABLED;
        } else {
            interruptsConfig[i].state = INT_STATE_DISABLED;
        }
    }
}

int hal_interrupt_set_direct_handler(IRQn_Type irqn, hal_interrupt_direct_handler_t handler, uint32_t flags, void* reserved) {
    // FIXME: the maxinum IRQn for KM0 is not identical with KM4
    if (irqn < NonMaskableInt_IRQn || irqn > GDMA0_CHANNEL5_IRQ_S) {
        return 1;
    }

    int32_t state = HAL_disable_irq();

    SCOPE_GUARD ({
        HAL_enable_irq(state);
    });

    if (handler == nullptr && (flags & HAL_INTERRUPT_DIRECT_FLAG_RESTORE)) {
        // Restore old handler only if one was backed up
        uint32_t old_handler = hal_interrupts_handler_backup[IRQN_TO_IDX(irqn)];
        if (old_handler) {
            __NVIC_SetVector(irqn, (uint32_t)old_handler);
            hal_interrupts_handler_backup[IRQN_TO_IDX(irqn)] = 0;
        }
    } else {
        // If there is currently a handler backup: Return error
        CHECK_FALSE(hal_interrupts_handler_backup[IRQN_TO_IDX(irqn)], SYSTEM_ERROR_ALREADY_EXISTS);
        
        // If there is a current handler, back it up
        uint32_t current_handler = __NVIC_GetVector(irqn);
        if (current_handler) {
            hal_interrupts_handler_backup[IRQN_TO_IDX(irqn)] = current_handler;    
        }    
    }

    if (flags & HAL_INTERRUPT_DIRECT_FLAG_DISABLE) {
        // Disable
        __NVIC_DisableIRQ(irqn);
    } else if (flags & HAL_INTERRUPT_DIRECT_FLAG_ENABLE) {
        __NVIC_SetVector(irqn, (uint32_t)handler);
    }

    return 0;
}
