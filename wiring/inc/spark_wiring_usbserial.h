/**
 ******************************************************************************
 * @file    spark_wiring_usbserial.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_usbserial.c module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#ifndef __SPARK_WIRING_USBSERIAL_H
#define __SPARK_WIRING_USBSERIAL_H

#include "spark_wiring_stream.h"
#include "spark_wiring_platform.h"
#include "usb_hal.h"
#include "system_task.h"
#include "spark_wiring_startup.h"
#include "concurrent_hal.h"

class USBSerial : public Stream
{
public:
	// public methods
    USBSerial();
	USBSerial(HAL_USB_USART_Serial serial, const HAL_USB_USART_Config& conf);

    unsigned int baud();

    operator bool();
    bool isEnabled();
    bool isConnected();

	void begin(long speed = 9600);
	void end();
	int peek();

	virtual size_t write(uint8_t byte);
	virtual int read();
	virtual int availableForWrite(void);
	virtual int available();
	virtual void flush();

	virtual void blockOnOverrun(bool);

#if PLATFORM_THREADING
	os_mutex_recursive_t get_mutex()
	{
		return os_mutex_recursive_t(system_internal(2, nullptr));
	}
#endif

	bool try_lock()
	{
#if PLATFORM_THREADING
		return !os_mutex_recursive_trylock(get_mutex());
#else
		return true;
#endif
	}

	void lock()
	{
#if PLATFORM_THREADING
		os_mutex_recursive_lock(get_mutex());
#endif
	}

	void unlock()
	{
#if PLATFORM_THREADING
		os_mutex_recursive_unlock(get_mutex());
#endif
	}

	using Print::write;

private:
    HAL_USB_USART_Serial _serial;
	bool _blocking;
};

HAL_USB_USART_Config acquireSerialBuffer() __attribute__((weak));
#if Wiring_USBSerial1
HAL_USB_USART_Config acquireUSBSerial1Buffer() __attribute__((weak));
#endif

extern USBSerial& _fetch_usbserial();
#define Serial _fetch_usbserial()

#if Wiring_USBSerial1

extern USBSerial& _fetch_usbserial1();
#define USBSerial1 _fetch_usbserial1()

#define USBSERIAL1_ENABLE() STARTUP(USBSerial1.begin(9600))

#endif /* Wiring_USBSerial1 */

#endif
