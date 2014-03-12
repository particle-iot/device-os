/**
 ******************************************************************************
 * @file    spark_wiring_time.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    3-March-2014
 * @brief   Time utility functions to set and get Date/Time using RTC
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

#include "spark_wiring_time.h"

/* Time Structure not required : using real-time RTC_GetCounter() & RTC_SetCounter() calls */

/* Date Structure */
struct date_structure
{
	uint8_t day;
	uint8_t month;
	uint16_t year;
} system_date;

/**
 * @brief  Initializes calendar application.
 * @param  None
 * @retval None
 */
void Time_Init(void)
{
	uint32_t i = 0, tmp = 0;

	/* Initialize Date to default */
	system_date.month = 01;
	system_date.day = 01;
	system_date.year = 1970;

	if ((BKP_ReadBackupRegister(BKP_DR7) != 0xFFFF) || (BKP_ReadBackupRegister(BKP_DR8) != 0xFFFF))
	{
		/* Initialize Date from the BKP registers */
		system_date.month = (BKP_ReadBackupRegister(BKP_DR7) & 0xFF00) >> 8;
		system_date.day = (BKP_ReadBackupRegister(BKP_DR7) & 0x00FF);
		system_date.year = BKP_ReadBackupRegister(BKP_DR8);

		if (RTC_GetCounter() / 86399 != 0)
		{
			/* Loop to retrieve and update the elapsed days */
			for (i = 0; i < (RTC_GetCounter() / 86399); i++)
			{
				Time_DateUpdate();
			}

			RTC_SetCounter(RTC_GetCounter() % 86399);

			/* Save the day, month and year in the BKP registers */
			BKP_WriteBackupRegister(BKP_DR8, system_date.year);
			tmp = system_date.month << 8;
			tmp |= system_date.day;
			BKP_WriteBackupRegister(BKP_DR7, tmp);
		}
	}
}

/**
 * @brief  Check whether the passed year is Leap or not.
 * @param  None
 * @retval 1: leap year
 *         0: not leap year
 */
static uint8_t Time_IsLeapYear(uint16_t nYear)
{
	if (nYear % 4 != 0) return 0;
	if (nYear % 100 != 0) return 1;
	return (uint8_t)(nYear % 400 == 0);
}

/**
 * @brief  Updates date when time is 23:59:59.
 * @param  None
 * @retval None
 */
void Time_DateUpdate(void)
{
	if (system_date.month == 1 || system_date.month == 3 || system_date.month == 5 || system_date.month == 7 ||
			system_date.month == 8 || system_date.month == 10 || system_date.month == 12)
	{
		if (system_date.day < 31)
		{
			system_date.day++;
		}
		/* Date structure member: system_date.day = 31 */
		else
		{
			if (system_date.month != 12)
			{
				system_date.month++;
				system_date.day = 1;
			}
			/* Date structure member: system_date.day = 31 & system_date.month =12 */
			else
			{
				system_date.month = 1;
				system_date.day = 1;
				system_date.year++;
			}
		}
	}
	else if (system_date.month == 4 || system_date.month == 6 || system_date.month == 9 ||
			system_date.month == 11)
	{
		if (system_date.day < 30)
		{
			system_date.day++;
		}
		/* Date structure member: system_date.day = 30 */
		else
		{
			system_date.month++;
			system_date.day = 1;
		}
	}
	else if (system_date.month == 2)
	{
		if (system_date.day < 28)
		{
			system_date.day++;
		}
		else if (system_date.day == 28)
		{
			/* Leap year */
			if (Time_IsLeapYear(system_date.year))
			{
				system_date.day++;
			}
			else
			{
				system_date.month++;
				system_date.day = 1;
			}
		}
		else if (system_date.day == 29)
		{
			system_date.month++;
			system_date.day = 1;
		}
	}
}

/*******************************************************************************
 * Function Name  : Wiring_RTC_Interrupt_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function handles RTC global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Wiring_RTC_Interrupt_Handler(void)
{
	/* If counter is equal to 86339: one day was elapsed */
	/*
	 * if ((RTC_GetCounter() / 3600 == 23)
	 * && (((RTC_GetCounter() % 3600) / 60) == 59)
	 * && (((RTC_GetCounter() % 3600) % 60) == 59))
	 */
	if (RTC_GetCounter() == 0x00015180)
	{
		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		/* Reset counter value */
		RTC_SetCounter(0x0);

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		/* Increment the date */
		Time_DateUpdate();
	}
}

/* current hour */
int TimeClass::hour()
{
	return (RTC_GetCounter() / 3600);
}

/* current hour in 12 hour format */
int TimeClass::hourFormat12()
{
	int hr = hour();

	if(hr == 0 )
		return 12; //midnight
	else if( hr  > 12)
		return hr - 12;
	else
		return hr;
}

/* returns true if time now is AM */
uint8_t TimeClass::isAM()
{
	return !isPM();
}

/* returns true if time now is PM */
uint8_t TimeClass::isPM()
{
	return (hour() >= 12);
}

/* current minute */
int TimeClass::minute()
{
	return ((RTC_GetCounter() % 3600) / 60);
}

/* current seconds */
int TimeClass::second()
{
	return ((RTC_GetCounter() % 3600) % 60);
}

/* current day */
int TimeClass::day()
{
	return system_date.day;
}

/* current month */
int TimeClass::month()
{
	return system_date.month;
}

/* current four digit year */
int TimeClass::year()
{
	return system_date.year;
}

/* set the given time as system time */
void TimeClass::setTime(uint32_t datetime)
{
	uint32_t i = 0, tmp = 0;

	/* Disable RTC IRQ */
	NVIC_DisableIRQ(RTC_IRQn);

	/* Initialize Date to default */
	system_date.month = 01;
	system_date.day = 01;
	system_date.year = 1970;

	int days_count = datetime / 86399;

	if (days_count != 0)
	{
		/* Loop to update the date structure */
		for (i = 0; i < days_count; i++)
		{
			Time_DateUpdate();
		}

		RTC_SetCounter(datetime % 86399);
	}
	else
	{
		RTC_SetCounter(datetime);
	}

	BKP_WriteBackupRegister(BKP_DR8, system_date.year);
	tmp = system_date.month << 8;
	tmp |= system_date.day;
	BKP_WriteBackupRegister(BKP_DR7, tmp);

	/* Enable RTC IRQ */
	NVIC_EnableIRQ(RTC_IRQn);
}

TimeClass Time;
