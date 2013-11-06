//Tinker Application
#include "application.h"


int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String pin);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

void setup()
{
	//This function is called once
	//Setup the Tinker application here

	//Register all the Tinker functions
	Spark.function("digitalread", tinkerDigitalRead);
	Spark.function("digitalwrite", tinkerDigitalWrite);

	Spark.function("analogread", tinkerAnalogRead);
	Spark.function("analogwrite", tinkerAnalogWrite);

}

void loop()
{
	//This will run in a loop
}


/*******************************************************************************
 * Function Name  : tinkerDigitalRead
 * Description    : This functions returns a digital value of the pin passed as 
 						  an argument to the function.
 * Input          : Pin Number
 * Output         : None.
 * Return         : Value of the pin (0 or 1) in INT form
 *******************************************************************************/
int tinkerDigitalRead(String pin)
{
	//String pin(command);
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		pinMode(pinNumber, INPUT_PULLDOWN);
		return digitalRead(pinNumber);
	}
	else if (pin.startsWith("A"))
	{
		pinMode(pinNumber+10, INPUT_PULLDOWN);
		return digitalRead(pinNumber+10);
	}
	return -1;
}

/*******************************************************************************
 * Function Name  : tinkerDigitalWrite
 * Description    : 
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int tinkerDigitalWrite(String pin)
{
	//String pin(command);
	bool value = 0;
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.substring(3,7) == "HIGH") value = 1;
	else if(pin.substring(3,6) == "LOW") value = 0;
	else return -1;

	if(pin.startsWith("D"))
	{
		pinMode(pinNumber, OUTPUT);
		digitalWrite(pinNumber, value);
		return 1;
	}
	else if(pin.startsWith("A"))
	{
		pinMode(pinNumber+10, OUTPUT);
		digitalWrite(pinNumber+10, value);
		return 1;
	}
	else return 0;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogRead
 * Description    : 
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int tinkerAnalogRead(String pin)
{
	//String pin(command);
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		pinMode(pinNumber, INPUT);
		return analogRead(pinNumber);
	}
	else if (pin.startsWith("A"))
	{
		pinMode(pinNumber+10, INPUT);
		return analogRead(pinNumber+10);
	}
	return -1;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogWrite
 * Description    : 
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int tinkerAnalogWrite(String command)
{
	//String pin(command);
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	// if (command.charAt(4) == '\0')
	// {
	// 	pinMode(D0, OUTPUT);
	// 	digitalWrite(D0, HIGH);
	// }
	// else
	// {
	// 	pinMode(D0, OUTPUT);
	// 	digitalWrite(D0, LOW);
	// }
	String value = command.substring(3);

	if(command.startsWith("D"))
	{
		pinMode(pinNumber, OUTPUT);
		analogWrite(pinNumber, value.toInt());
		return 1;
	}
	else if(command.startsWith("A"))
	{
		pinMode(pinNumber+10, OUTPUT);
		analogWrite(pinNumber+10, value.toInt());
		return 1;
	}
	else return -1;
}
