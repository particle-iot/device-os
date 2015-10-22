/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include "modem/mdm_hal.h"

extern MDMElectronSerial electronMDM;

PMIC power;
FuelGauge fuel;

// ALL_LEVEL, TRACE_LEVEL, DEBUG_LEVEL, WARN_LEVEL, ERROR_LEVEL, PANIC_LEVEL, NO_LOG_LEVEL
//Serial1DebugOutput debugOutput(115200, ALL_LEVEL);

/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

int sleep(String command);
void setupPower(void);
void updatePower(void);


SYSTEM_MODE(MANUAL);

/* This function is called once at start up ----------------------------------*/
void setup()
{
    RGB.control(true);
    RGB.color(0,0,255);
    delay(1000);
    Serial1.begin(9600);
    delay(200);
    power.begin();
    delay(500);

    Serial1.println("Disabling PMIC watchdog timer");
    power.disableWatchdog();
    delay(50);

    //Serial1.println("Enabling OTG/PMID");
    //power.enableOTG();
    //delay(50);

    //Serial1.println("disabling buck regulator");
    //power.disableBuck();
    //delay(50);


    Serial1.print("power on returned:");
    Serial1.println(electronMDM.powerOn("8934076500002587657"));
    //setupPower();
    //delay(200);
    //fuel.sleep();

    /*
    RGB.color(0,0,255);
    delay(200);
    RGB.color(0,255,0);
    delay(200);
    RGB.color(255,0,0);
    delay(200);
    RGB.color(0,0,0);
   
    power.begin();
    delay(100);

    if (fuel.getVersion() == 3) {
        RGB.color(0,255,0);
    }
    else {
        RGB.color(255,0,0);
    }
    delay(2000);
    fuel.sleep();
    delay(100);
*/
    //electronMDM.powerOff();
    
    //System.sleep(SLEEP_MODE_DEEP,60);
    //setupPower();

    //electronMDM.setDebug(3); // enable this for debugging issues

    //delay(3000);
    //DEBUG_D("\e[0;36mHello from the Electron! Boot time is: %d\r\n",millis());

    //Particle.connect(); // blocking call to connect

    //Register all the Tinker functions
    //Particle.function("digitalread", tinkerDigitalRead);
    //Particle.function("digitalwrite", tinkerDigitalWrite);

    //Particle.function("analogread", tinkerAnalogRead);
    //Particle.function("analogwrite", tinkerAnalogWrite);

    //Particle.function("sleep",sleep);
    

    //Particle.connect(); // blocking call to connect


    //Serial1.begin(9600);
    //Serial1.println("Setup complete");
    //delay(1000);

    Serial1.print("Fuel Gauge Version Number: ");
    Serial1.println(fuel.getVersion());
    delay(200);
    
    fuel.sleep();
    delay(200);

    //Serial1.print("power off returned:");
    //Serial1.println(electronMDM.powerOff());
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    //updatePower();
    
    //Serial1.print("power off returned:");
    //Serial1.println(electronMDM.powerOff());
    //RGB.color(0,0,255);
    //delay(200);
    //RGB.color(0,255,0);
    //delay(200);
    //RGB.color(255,0,0);
    //delay(200);
    //Serial1.println(power.readPowerONRegister(),BIN);
    if (Serial1.available() > 0)
    {
        char inByte = Serial1.read();
        Serial1.print("I rxed this shit:");
        Serial1.println(inByte);
        if (inByte == 'o') {
            Serial1.println("char o received");
            power.enableOTG();
            delay(50);
        }
        if (inByte == 'd') {
            Serial1.println("char d received");
            power.disableCharging();
            delay(50);
        }
        if (inByte == 'e') {
            Serial1.println("char d received");
            power.enableCharging();
            delay(50);
        }
    }
    //System.sleep(SLEEP_MODE_DEEP,60);

}

void setupPower() {

    
    Serial1.println("Entered power setup");

    Serial1.println("Initializing PMIC");
    power.begin(); 
    delay(500);

    Serial1.println("Setting input voltage limit");
    power.setInputVoltageLimit(4120);
    delay(50);

    Serial1.println("Disabling PMIC watchdog timer");
    power.disableWatchdog();
    delay(50);

    Serial1.println("Disabling DPDM detection");
    power.disableDPDM();
    delay(50);

    //Serial1.println("Setting input current limit");
    //power.setInputCurrentLimit(100);
    //delay(50);

    //Serial1.println("disabling buck regulator");
    //power.disableBuck();
    //delay(50);

    //Serial1.println("turning off battery FET");
    //power.disableBATFET();
    //delay(20);


    //power.setChargeCurrent(0,0,0,0,0,0); //512
    //delay (50);

    //Serial1.println("Forcing charge enable");
    //power.enableCharging();
    //delay(50);

}


void updatePower(void) {

    //Serial1.println("Setting input voltage limit");
    //power.setInputVoltageLimit(4);
    //delay(50);

    Serial1.println("Setting input current limit");
    power.setInputCurrentLimit(500);
    delay(50);

    //Serial1.println("Force DPDM detection");
    //power.enableDPDM();
    //delay(500);

    //Serial1.println("Forcing charge enable");
    //power.enableCharging();
    //delay(50);

    //Serial1.println("Enabling the buck regulator");
    //power.enableBuck();
    //delay(50);

    //This will run in a loop
    Serial1.print("Power good: ");
    Serial1.println(power.isPowerGood());

    Serial1.print("System Status: ");
    Serial1.println(power.getSystemStatus(), BIN);

    byte status = 0;
    status = power.getSystemStatus();
    if (status & 0b00000001) Serial1.println("In regulation: YES");
    else Serial1.println("In regulation: NO");

    //if (status & 0b00000010) Serial1.println("HOT");
    //else Serial1.println("Normal");

    if (status & 0b00000100) Serial1.println("Power: Good");
    else Serial1.println("Power: Bad");

    if (status & 0b00000100) Serial1.println("In DPM mode: YES");
    else Serial1.println("In DPM mode: NO");

    if ((status & 0b00110000) == 0x00) //00
        Serial1.println("Not charging");
    if ((status & 0b00110000) == 0x30) //11
        Serial1.println("Charge termination done");
    if ((status & 0b00110000) == 0x10) //01
        Serial1.println("Pre charging");
    if ((status & 0b00110000) == 0x20) //10
        Serial1.println("Fast charging");

    if ((status & 0b11000000) == 0x00) //00
        Serial1.println("no input/dpdm detection incomplete");
    if ((status & 0b11000000) == 0x40) //01
        Serial1.println("USB Host");
    if ((status & 0b11000000) == 0x80) //10
        Serial1.println("Adaptor Port");
    if ((status & 0b11000000) == 0xC0) //11
        Serial1.println("OTG");



    Serial1.print("Input Register: ");
    Serial1.println(power.readInputSourceRegister(), BIN);

    Serial1.print("Term/Timer Register: ");
    Serial1.println(power.readChargeTermRegister(), BIN);

    Serial1.print("Misc Operation Register: ");
    Serial1.println(power.readOpControlRegister(), BIN);

    Serial1.print("Charge current control register: ");
    Serial1.println(power.getChargeCurrent(), BIN);

    Serial1.println(" ---------- ");

    delay(2000);
}

int sleep(String command) {

    Serial1.println("Going to sleep");
    delay(200);
    if(command == "1")
        System.sleep(SLEEP_MODE_DEEP,60);

    return 1;
}
/*******************************************************************************
 * Function Name  : tinkerDigitalRead
 * Description    : Reads the digital value of a given pin
 * Input          : Pin
 * Output         : None.
 * Return         : Value of the pin (0 or 1) in INT type
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerDigitalRead(String pin)
{
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
    return -2;
}

/*******************************************************************************
 * Function Name  : tinkerDigitalWrite
 * Description    : Sets the specified pin HIGH or LOW
 * Input          : Pin and value
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerDigitalWrite(String command)
{
    bool value = 0;
    //convert ascii to integer
    int pinNumber = command.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (pinNumber< 0 || pinNumber >7) return -1;

    if(command.substring(3,7) == "HIGH") value = 1;
    else if(command.substring(3,6) == "LOW") value = 0;
    else return -2;

    if(command.startsWith("D"))
    {
        pinMode(pinNumber, OUTPUT);
        digitalWrite(pinNumber, value);
        return 1;
    }
    else if(command.startsWith("A"))
    {
        pinMode(pinNumber+10, OUTPUT);
        digitalWrite(pinNumber+10, value);
        return 1;
    }
    else return -3;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogRead
 * Description    : Reads the analog value of a pin
 * Input          : Pin
 * Output         : None.
 * Return         : Returns the analog value in INT type (0 to 4095)
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerAnalogRead(String pin)
{
    //convert ascii to integer
    int pinNumber = pin.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (pinNumber< 0 || pinNumber >7) return -1;

    if(pin.startsWith("D"))
    {
        return -3;
    }
    else if (pin.startsWith("A"))
    {
        return analogRead(pinNumber+10);
    }
    return -2;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogWrite
 * Description    : Writes an analog value (PWM) to the specified pin
 * Input          : Pin and Value (0 to 255)
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerAnalogWrite(String command)
{
    //convert ascii to integer
    int pinNumber = command.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (pinNumber< 0 || pinNumber >7) return -1;

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
    else return -2;
}
