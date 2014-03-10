/**
 ******************************************************************************
 * @file    spark_wiring_calendar.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    3-March-2014
 * @brief   Calendar utility functions to set and get Date/Time using RTC
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

#include "spark_wiring_calendar.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_rtc.h"

/* Time Structure definition */
struct rtc_time_t
{
	uint8_t sec_l;
	uint8_t sec_h;
	uint8_t min_l;
	uint8_t min_h;
	uint8_t hour_l;
	uint8_t hour_h;
};
struct rtc_time_t time_struct;

/* Alarm Structure definition */
struct rtc_alarm_t
{
	uint8_t sec_l;
	uint8_t sec_h;
	uint8_t min_l;
	uint8_t min_h;
	uint8_t hour_l;
	uint8_t hour_h;
};
struct rtc_alarm_t alarm_struct;

/* Date Structure definition */
struct rtc_date_t
{
	uint8_t month;
	uint8_t day;
	uint16_t year;
};
struct rtc_date_t date_s;

/**
 * @brief  Initializes calendar application.
 * @param  None
 * @retval None
 */
void Calendar_Init(void)
{
	uint32_t i = 0, tmp = 0;

	if ((BKP_ReadBackupRegister(BKP_DR7) == 0xFFFF) && (BKP_ReadBackupRegister(BKP_DR8) == 0xFFFF))
	{
		/* Initialize Date structure */
		/* Date not configured. Set to default */
		date_s.month = 01;
		date_s.day = 01;
		date_s.year = 2014;
		/* Or get the current date/time using NTP or via Cloud Handshake and set the BKP registers */
		BKP_WriteBackupRegister(BKP_DR8, date_s.year);
		tmp = date_s.month << 8;
		tmp |= date_s.day;
		BKP_WriteBackupRegister(BKP_DR7, tmp);
	}
	else
	{
		/* Initialize Date structure */
		date_s.month = (BKP_ReadBackupRegister(BKP_DR7) & 0xFF00) >> 8;
		date_s.day = (BKP_ReadBackupRegister(BKP_DR7) & 0x00FF);
		date_s.year = BKP_ReadBackupRegister(BKP_DR8);

		if (RTC_GetCounter() / 86399 != 0)
		{
			for (i = 0; i < (RTC_GetCounter() / 86399); i++)
			{
				Calendar_DateUpdate();
			}
			RTC_SetCounter(RTC_GetCounter() % 86399);
			BKP_WriteBackupRegister(BKP_DR8, date_s.year);
			tmp = date_s.month << 8;
			tmp |= date_s.day;
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
static uint8_t Calendar_IsLeapYear(uint16_t nYear)
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
void Calendar_DateUpdate(void)
{
	if (date_s.month == 1 || date_s.month == 3 || date_s.month == 5 || date_s.month == 7 ||
			date_s.month == 8 || date_s.month == 10 || date_s.month == 12)
	{
		if (date_s.day < 31)
		{
			date_s.day++;
		}
		/* Date structure member: date_s.day = 31 */
		else
		{
			if (date_s.month != 12)
			{
				date_s.month++;
				date_s.day = 1;
			}
			/* Date structure member: date_s.day = 31 & date_s.month =12 */
			else
			{
				date_s.month = 1;
				date_s.day = 1;
				date_s.year++;
			}
		}
	}
	else if (date_s.month == 4 || date_s.month == 6 || date_s.month == 9 ||
			date_s.month == 11)
	{
		if (date_s.day < 30)
		{
			date_s.day++;
		}
		/* Date structure member: date_s.day = 30 */
		else
		{
			date_s.month++;
			date_s.day = 1;
		}
	}
	else if (date_s.month == 2)
	{
		if (date_s.day < 28)
		{
			date_s.day++;
		}
		else if (date_s.day == 28)
		{
			/* Leap year */
			if (Calendar_IsLeapYear(date_s.year))
			{
				date_s.day++;
			}
			else
			{
				date_s.month++;
				date_s.day = 1;
			}
		}
		else if (date_s.day == 29)
		{
			date_s.month++;
			date_s.day = 1;
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
	if ((RTC_GetCounter() / 3600 == 23) && (((RTC_GetCounter() % 3600) / 60) == 59) &&
			(((RTC_GetCounter() % 3600) % 60) == 59)) /* 23*3600 + 59*60 + 59 = 86339 */
	{
		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		/* Reset counter value */
		RTC_SetCounter(0x0);

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		/* Increment the date */
		Calendar_DateUpdate();
	}
}

/*
int hour() // the hour now
{
	//To Do
}

int hour(time_t t) // the hour for the given time
{
	//To Do
}

int hourFormat12() // the hour now in 12 hour format
{
	//To Do
}

int hourFormat12(time_t t) // the hour for the given time in 12 hour format
{
	//To Do
}

uint8_t isAM() // returns true if time now is AM
{
	//To Do
}

uint8_t isAM(time_t t) // returns true if given time is AM
{
	//To Do
}

uint8_t isPM() // returns true if PM
{
	//To Do
}

uint8_t isPM(time_t t) // returns true if PM
{
	//To Do
}

int minute()
{
	//To Do
}

int minute(time_t t) // the minute for the given time
{
	//To Do
}

int second()
{
	//To Do
}


int second(time_t t) // the second for the given time
{
	//To Do
}

int day()
{
	//To Do
}

int day(time_t t) // the day for the given time (0-6)
{
	//To Do
}

int weekday() // Sunday is day 1
{
	//To Do
}

int weekday(time_t t)
{
	//To Do
}


int month()
{
	//To Do
}

int month(time_t t)
{
	//To Do
}


int year()
{
	//To Do
}

int year(time_t t)
{
	//To Do
}


time_t now()
{
	//To Do
}


void setTime(time_t t)
{
	//To Do
}

void  setTime(int hr,int min,int sec,int dy, int mnth, int yr)
{
	//To Do
}


void adjustTime(long adjustment)
{
	//To Do
}
*/
