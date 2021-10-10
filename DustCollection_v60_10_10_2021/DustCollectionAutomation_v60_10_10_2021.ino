
/*
   This code is for the project at
   http://www.iliketomakestuff.com/how-to-automate-a-dust-collection-system-arduino
   All of the components are list on the url above.

  This script was created by Bob Clagett for I Like To Make Stuff
  For more projects, check out iliketomakestuff.com

  Includes Modified version of "Measuring AC Current Using ACS712"
  http://henrysbench.capnfatz.com/henrys-bench/arduino-current-measurements/acs712-arduino-ac-current-tutorial/

  Parts of this sketch were taken from the keypad and servo sample sketches that comes with the keypad and servo libraries.

  Uses https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library

  This new version of the code ortiginally written by Bob Clagett is modified to make it easier to implement in a new shop, and not have to re-write code
  when moving the machines around.   The DustGatesDefinition.cfg  is to be stored on an SD card and manage the configuration of the top level parameters
  have a section for dust gates and a final section for votage switches.   The parameter in each voltage switch maps to a main dust gate for that maching,
  and the code below rolls through the comma delimited bit map to set all the other gates open or closed based on the 0,1 flag

  Because of this config file, there is no need to modify code if you are taking a machine off line, moving it around or changing gate configuration.
  Simply modify the layout in the gate definition, gate bitmap or voltage sensor definition.    Using a '#" at the beinning of the voltage sensor or gate
  will inactivate the unit, as the code below ignores commented lines.

  When an Amerpage chnage is detected, the main gate index is used to look up the main gate in the structure.   Upon finding it, the gate bitmap is parsed
  and gates are opened and closed.

  Each voltage sensor is mapped to an analog port for monitoring.   That port is set in the voltage sensor config definition, and can be modified if you
  want to add/remove machines.


  NEW VERSION - Cory Wiegert   06/20/2020
    instead of hard coding the dust gates and machines in the setup, will use an SD Card and configure the machines via delimited config line
    can add and configure the dust gates dynamically, instead of having to recompile and upload new code

              Cory D. Wiegert  07/19/2020   v5.0    Adding the wifi and Blynk libraries and code to be able to control individual gates and dust collector
                                                    with the mobile app configured on the phone.   moved most of the setup and reading of the config file
                                                    to functions instead of running it all in 1 routine in setup()
              Cory D. Wiegert 07/24/2020    v5.1    Adding Blynk setup as stand alone function for a new Mobile app to control gates and dust collector manually

              Cory D. Wiegert 07/27/2020    v5.11   Need to debug the checkForAmperageChange function to ensure it is using ampThreshold as a compare

              Cory D. Wiegert 07/31/2020    v5.12    Implemented the check for the change threshold from the config file.   fixed the bug in BLYNK_WRITE(V1)
              Cory D. Wiegert 08/03/2020    v5.2    Added a tab menu to the Blynk app, to set the configuration of gate limits.
                                                    Virtual buttons V18-V22
              Cory D. Wiegert 08/11/2020    v5.3    Added encryption to the Blynk configuration string from the config file.
                                                    Also added Encrypt tab on the Blynk app,  virtual Buttons V30 - V40
              Cory D. Wiegert 11/28/2020    v5.3.01   changed the delay on the open and close, testing the speed of the gate open and closing
              Cory D. Wiegert 08/22/2021    v5.4    Refactoring code for include libraries.   Not tested on production environment
              Cory D. Wiegert 10/07/2021    v5.4.5  code refactored, reading from EEPROM.   Need to have EEPROM_Writer_DustoConnector to put values into EEPROM
              Cory D. Weigert 10/10/2021    v6.0    finished new Blynk controls for setting gate configuration through Blynk app and not having to code into the txt file
                                                    The Config screen on the Blynk app now has a "add gate" funciton, as well as a save to EERPOM for the open, close, gatemap and 
                                                    gate name fields.    If a new button is not activated, the save to EERPROM will over write the values from the UI to EEPROM
                                                    the save button will store the values in the running memory structures

                                                    Before trying to run this program, run EEPROM_Writer_DustCollector_v60_10_10_2021.ino
                                                    That file will read the gate configuration, load the encrypted WIFI, and set all the congiruation to EEPROM

                                                    Alternatively, you can load the reserve file, DustCollectorEEPROM.cfg while will load a binary copy of the EEPROM image

****************************************************************************************************************************************/
#define BLYNK_PRINT Serial
#include <Adafruit_PWMServoDriver.h>
#include "DustCollectorGlobals.h"
#include <ESP8266_Lib.h>
#include <EEPROM.h>
#include <BlynkSimpleShieldEsp8266.h>
#define EspSerial Serial1


Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();                 // called this way, it uses the default address 0x40

// you can also call it with a different address you want
//Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);

// Hardware Serial on Mega, Leonardo, Micro...
ESP8266     wifi(&EspSerial);
BlynkTimer  timer;
WidgetTerminal terminal(V8);
WidgetTerminal confTerm(V22);
WidgetTerminal encryptTerm(V40);


BLYNK_WRITE(V1)       // turn on and off dust collector button
{
  if (manualOveride == true)
    if (param.asInt() == 1)           // button is pressed to on
      if (collectorIsOn == false)     // collector is not running
        turnOnDustCollection();
      else if (collectorIsOn == true) //  collector is running
        turnOffDustCollection();
}

BLYNK_WRITE(V2)     // Gate drop down, selecting which gate is opened or closed
{
  blynkSelectedGate = param.asInt() - 1;

}

BLYNK_WRITE(V5)   // button for opening a gate.   Will set highlight back to middle button after processing click
{
  if (manualOveride == true)
    if (param.asInt() == 1)
    {

      terminal.print ( "opening machine => ");
      terminal.println ( blastGate[blynkSelectedGate].gateName);
      openGate (blynkSelectedGate);
      Blynk.virtualWrite(V3, blastGate[blynkSelectedGate].openPos);
      Blynk.syncVirtual(V3);
      runGateMap (blynkSelectedGate);
      Blynk.virtualWrite(V5, 2 );     // set the control to hold, so we can use the open and close buttons again
    }
    else if (param.asInt() == 3)
    {
      terminal.print("   param ==> ");
      terminal.println (param.asInt());
      closeGate (blynkSelectedGate, false);
      Blynk.virtualWrite(V4, blastGate[blynkSelectedGate].closePos);
      Blynk.syncVirtual(V4);
      Blynk.virtualWrite(V5, 2);   // set the control to hold, so we can use the open and close buttons again
    }
}

BLYNK_WRITE(V6)     // changes operation from automated monitoring | manual use of Blynk app
{
  if (param.asInt() == 1)
    manualOveride = true;
  else
    manualOveride = false;
}

BLYNK_WRITE(V7)   //   text on screen to close all the gates in the system.   Similar to a reinitialization
{
  if (manualOveride == true)
    if (param.asInt() == 1)
    {
      turnOffDustCollection();
      closeAllGates(false);
    }
}

BLYNK_WRITE (V9)      // text button on screen to clear the terminal widget
{
  terminal.clear();
}

BLYNK_WRITE(V12)    // button that reads the voltage sensors of selected outlet
{
  terminal.println(param.asInt());
  if (param.asInt() == 2)
  {
    for (int c = 0; c <= 20; c++)
    {
      Serial.println(getVPP(inspectionPin));
      terminal.println(getVPP(inspectionPin));
      terminal.flush();
    }
    Blynk.virtualWrite(V12, 1);
    //delay(5000);                 //want to be able to walk from band saw to computer
  }
}

BLYNK_WRITE(V11)    //  menu drop down to choose which outlet to monitor
{
  inspectionPin = toolSwitch[param.asInt() - 1].voltSensor;
}

BLYNK_WRITE (V15)   //  text button to reset the SD card.   Useful if making changes to the config file, doesn't require a restart of the Arduino script
{
}

BLYNK_WRITE(V16)    // menu text, turns on the debug tracing.    Don't need to set the config file, can turn off debugging by clicking button again
{
  if (param.asInt() == 1)
  {
    dust.DEBUG = true;
    Serial.begin (500000);
    delay(500);
  }
  else
    dust.DEBUG = false;
}

BLYNK_WRITE(V18)    //  drop down for which gate to configure - lists the blas gates (on the config screen)
{
  blynkSelectedGate = param.asInt() - 1;  
  Blynk.virtualWrite(V13, blastGate[blynkSelectedGate].gateID);
  Blynk.virtualWrite(V10, blastGate[blynkSelectedGate].pwmSlot);
  Blynk.virtualWrite(V44, blastGate[blynkSelectedGate].gateName);
  Blynk.virtualWrite(V20, blastGate[blynkSelectedGate].openPos);
  Blynk.virtualWrite(V21, blastGate[blynkSelectedGate].closePos);
  Blynk.virtualWrite(V45, blastGate[blynkSelectedGate].gateConfig);
  Blynk.syncVirtual(V20);
  Blynk.syncVirtual(V21);
  Blynk.syncVirtual(V45);
  Blynk.syncVirtual(V13);
  Blynk.syncVirtual(V10);
  Blynk.syncVirtual(V44);
}

BLYNK_WRITE(V19)    //  open close toggle switch to tell the gate to open or close (on config screen)
{
  if (manualOveride == true)
  {
    if (param.asInt() == 1)
    {
      confTerm.print ( "Opening machine => ");
      confTerm.println ( blastGate[blynkSelectedGate].gateName);
      confTerm.print ("    setting PWM ==> ");
      confTerm.print( blastGate[blynkSelectedGate].pwmSlot);
      confTerm.print (" position ==> ");
      confTerm.println(highPos);
      confTerm.flush();
      pwm.setPWM(blastGate[blynkSelectedGate].pwmSlot, 0, (highPos - 20));
      delay(20);
      for (int c = (highPos - 20); c <= highPos; c += 1)
        pwm.setPWM (blastGate[blynkSelectedGate].pwmSlot, 0, c);

      //Blynk.syncVirtual(V3);

      Blynk.virtualWrite(V19, 2 );     // set the control to hold, so we can use the open and close buttons again
    }
    else if (param.asInt() == 3)
    {
      confTerm.print ( "Closing machine => ");
      confTerm.println ( blastGate[blynkSelectedGate].gateName);
      confTerm.print ("    setting PWM ==> ");
      confTerm.print( blastGate[blynkSelectedGate].pwmSlot);
      confTerm.print (" position ==> ");
      confTerm.println(lowPos);
      confTerm.flush();
      pwm.setPWM(blastGate[blynkSelectedGate].pwmSlot, 0, (lowPos - 20));
      delay(20);
      for (int c = (lowPos - 20); c <= lowPos; c += 1)
        pwm.setPWM (blastGate[blynkSelectedGate].pwmSlot, 0, c);
    }
    Blynk.virtualWrite(V19, 2);   // set the control to hold, so we can use the open and close buttons again
  }
}

BLYNK_WRITE (V20)   //  number field on config screen used to set the open limit on the servero 
{                   //  modified v6.0
  if (!gateAdded)
    {
      highPos = param.asInt();
      blastGate[blynkSelectedGate].openPos = highPos;
    }
  else
    blastGate[dust.NUMBER_OF_GATES + 1].openPos = param.asInt();
}

BLYNK_WRITE (V21)   //  number field on the config screen used to set the close limit on the servo
{                   //  modified v6.0
   if (!gateAdded)
    {
      lowPos = param.asInt();
      blastGate[blynkSelectedGate].closePos = lowPos;
    }
  else
    blastGate[dust.NUMBER_OF_GATES + 1].closePos = param.asInt();

}

BLYNK_WRITE(V23)    //  button used to clear the terminal on the config screen
{
  confTerm.clear();
}

BLYNK_WRITE(V30)    //  Text field on the encrypt screen used to input the wifi config file name
{

}
BLYNK_WRITE(V31)    //  toggle control on the encrypt screen used to tell whether to decrypt or encrypt the input file name
{                   //  modified v6.0 --- commented out, the ENCRYPT screen will not work
  /*char   delim[2] = {char(222), '\0'};
  char   space = char(223);
  char   encryptString[96] = {'\0'};

  if (param.asInt() == 1)
  {
    // figure out what the decrypt algorythm is going to be
  }
  else if (param.asInt() == 3)
  {
    encryptTerm.println(blynkWIFIConnect.ssid);
    encryptTerm.flush();

    encryptString[0] = char(222);
    encryptString[1] = char(174);
    encryptString[2] = char(222);
    strcat(encryptString, ssid);
    encryptTerm.println(ssid);
    encryptTerm.flush();
    encryptTerm.println(encryptString);
    encryptTerm.flush();
    strcat(encryptString, delim);
    strcat(encryptString, pass);
    strcat(encryptString, delim);
    strcat(encryptString, server);
    strcat(encryptString, delim);
    strcat(encryptString, port);
    strcat(encryptString, delim);
    strcat(encryptString, ESPSpeed);
    strcat(encryptString, delim);
    strcat(encryptString, BlynkConnection);
    strcat(encryptString, delim);
    strcat(encryptString, auth);
    strcat(encryptString, delim);
    for (int x = 0; x <= sizeof(encryptString); x++)
      if (encryptString[x] == '\0')
        encryptString[x] = 'z';
    encryptTerm.println(encryptString);
    encryptTerm.flush();
    Serial.println(encryptString);
    Serial.println(sizeof(encryptString));
    aes128_enc_multiple(cypherKey, encryptString, 96);
    Serial.println(encryptString);
    aes128_dec_multiple(cypherKey, encryptString, 96);
    Serial.println(encryptString);

  }
  //  after work is finished,
  Blynk.virtualWrite(V31, 2);*/
}

BLYNK_WRITE (V32)   // populate the blynkWIFIConnect structure with the values from the screen
{                   //  modified v6.0
  if (param.asInt() == 1)   
  {                     
    BLYNK_WRITE(V33);
    BLYNK_WRITE(V34);
    BLYNK_WRITE (V35);
    BLYNK_WRITE (V36);
    BLYNK_WRITE (V37);
    BLYNK_WRITE (V38);
    BLYNK_WRITE (V39);
    writeWIFIToEEPROM();
  }
}

BLYNK_WRITE (V33)   //  text input field to set the SSID for the wifi connection.   Set by decrypt, or entered manually
  {
    strcpy(blynkWIFIConnect.ssid, param.asStr());
    encryptTerm.println(blynkWIFIConnect.ssid);
    encryptTerm.flush();
  }

BLYNK_WRITE(V34)    //  text input on encrypt screen to set the wifi password.   decrypted or seet manually
{
  strcpy(blynkWIFIConnect.pass, param.asStr());
  encryptTerm.println(blynkWIFIConnect.pass);
  encryptTerm.flush();
}
BLYNK_WRITE(V35)    //  text input on encrypt screen to set the blynk server connection  decrypted or set manually
{
  strcpy(blynkWIFIConnect.server, param.asStr());
  encryptTerm.println(blynkWIFIConnect.server);
  encryptTerm.flush();
}
BLYNK_WRITE(V36)    //  text input on encrypt screen to set the blynk server port (local)  decrypted or set manually
{
  strcpy(blynkWIFIConnect.port, param.asStr());
  encryptTerm.println(blynkWIFIConnect.port);
  encryptTerm.flush();
}

BLYNK_WRITE (V37)   //  text input on encrypt screen to set the speed of the wifi shield   decrypted or set manually
{
  strcpy(blynkWIFIConnect.ESPSpeed, param.asStr());
  encryptTerm.println (blynkWIFIConnect.ESPSpeed);
  encryptTerm.flush();
}
BLYNK_WRITE(V38)    //  text input on encrypt screen to set the boolean for local or blynk server    decrypted or set manually
{
  strcpy(blynkWIFIConnect.BlynkConnection, param.asStr());
  encryptTerm.println(blynkWIFIConnect.BlynkConnection);
  encryptTerm.flush();
}

BLYNK_WRITE(V39)    //  text input on encrypt screen to set the blynk app auth key.   decrypted or set manually
{
  strcpy( blynkWIFIConnect.auth, param.asStr());
  encryptTerm.println(blynkWIFIConnect.auth);
  encryptTerm.flush();
}

BLYNK_WRITE (V41)   //  button to open, decrypt and read file named in V30 
  {                 //  modified v6.0
    if ( param.asInt() == 1)
      {
        char     delim[2] = {char(222), '\0'};         // delimeter for parsing the servo config
        
        startWifiFromConfig ('^', delim, 0);         
      } // end of If Param == 1
}

BLYNK_WRITE (V42)   // Create memory space for new gate to be added to the system.   Must save to EEPROM for gate to be active
{                   //  modified v6.0
  if (param.asInt() == 1)
    {
      gateAdded = true;
      Blynk.virtualWrite(V20, 0);
      Blynk.virtualWrite (V21, 0);
      Blynk.virtualWrite (V45, "0000000000000");
      Blynk.virtualWrite (V13, -1);
      Blynk.virtualWrite (V10, -1);
      Blynk.virtualWrite (V44, "WHAT IS MY NAME?");
    }
}

BLYNK_WRITE (V24)   // Added in v6.0  It's the button to write the new gate to EEPROM
{                   //  modified v6.0
  if (gateAdded)
    {
      eeAddress = SET_CONFIG;
      EEPROM.put (eeAddress, dust);
      eeAddress += sizeof(dust);
      EEPROM.write(EEPROM.length()-10, eeAddress);
    }
  eeAddress = GATE_ADDRESS;           
  for (int addy =0; addy < gates; addy ++)
    {
      EEPROM.put (eeAddress, blastGate[addy]);
      eeAddress += sizeof(blastGate[addy]);
    }
  EEPROM[EEPROM.length() - 8] = eeAddress;
}

BLYNK_WRITE (V25)   //  Added in v 6.0 button to save values from the UI to the local variables
{                   //  modified v6.0
  if (param.asInt() == 1)
    {
      if (gateAdded)
        {
          dust.NUMBER_OF_GATES += 1;
          gates += 1;
          gateAdded = false;
        }
      BlynkParamAllocated clearout(256);    // valiable to clear the existing values from the menu dropdown        
      Blynk.setProperty(V2, "labels", clearout);     // clear all existing values from menu
      Blynk.setProperty(V18, "labels", clearout);      // clear all existing values from gate menue
      for (int i = 0; i < dust.NUMBER_OF_GATES; i++)
        clearout.add (blastGate[i].gateName);
      Blynk.setProperty(V2, "labels", clearout);   // populate the menu with the name of each outlet
      Blynk.virtualWrite (V2, 3);   // Highlights the sander as the selected gate
      Blynk.setProperty(V18, "labels", clearout);
      Blynk.syncVirtual(V2);
      Blynk.syncVirtual(V18);
    }    
}

BLYNK_WRITE(V13)    //  control to configure the GATEID from the blynk UI.    added in v 6.0
  {                 //  modified v6.0
    if (gateAdded)
      {
        if (dust.NUMBER_OF_GATES +1 < 20)
          blastGate[dust.NUMBER_OF_GATES+1].gateID = param.asInt();
      }
    else
      blastGate[blynkSelectedGate].gateID = param.asInt();
  }
BLYNK_WRITE(V10)    //  control to configure the pwmSlot from the blynk UI.   added in v6.0
  {               //  modified v6.0
    if (gateAdded)
      {
        if (dust.NUMBER_OF_GATES +1 < 20)
          blastGate[dust.NUMBER_OF_GATES+1].pwmSlot = param.asInt();
      }
    else
      blastGate[blynkSelectedGate].pwmSlot = param.asInt();
  }
BLYNK_WRITE(V44)    //  control to configure the gateName from the Blynk UI   added in v 6.0
  {
    if (gateAdded)
      if (dust.NUMBER_OF_GATES +1 < 20)
        strcpy(blastGate[dust.NUMBER_OF_GATES+1].gateName,param.asStr());
      else
        strcpy(blastGate[blynkSelectedGate].gateName, param.asStr());
  }

BLYNK_WRITE(V45)    //  control to configure the gate bitmap from the Blynk UI   added in v6.0
  {
    if (gateAdded)
      {
        if (dust.NUMBER_OF_GATES +1 < 20)
          strcpy(blastGate[dust.NUMBER_OF_GATES+1].gateConfig, param.asStr());
      }
    else
      strcpy(blastGate[blynkSelectedGate].gateConfig, param.asStr());
  }

void setup()
{

  // You should get Auth Token in the Blynk App.
  // Go to the Project Settings (nut icon).
  //  char  auth[] = "-dv99jBjBpvacaTas2NNEEHs50c4aVzP";


    Serial.begin(500000);
    delay(1000);
    readConfigEEPROM();     //  modified v6.0
    readGatesEEPROM ();     //  modified v6.0
    readOutletsEEPROM();    //  modified v6.0
    readWIFIConfig();       //  modified v6.0

    if (dust.DEBUG == true)
    {
      for (int index = 0; index < dust.NUMBER_OF_GATES; index++)
      {
        writeDebug (blastGate[index].gateName, 1);
        writeDebug ("   " + String (blastGate[index].gateID) + "      Gate pwmSlot ==> " + String(blastGate[index].pwmSlot) , 0);
        writeDebug("    " + String (blastGate[index].openPos) + "    " + String (blastGate[index].closePos) + "     ", 1);
        writeDebug ("    " + String (blastGate[index].openClose) + "     " + String(blastGate[index].gateConfig), 1);
        writeDebug (String (toolSwitch[index].tool) + "    " + String (toolSwitch[index].switchID) + "   ", 1);
        writeDebug ("    " + String (toolSwitch[index].voltSensor) + "    " + String(toolSwitch[index].voltBaseline) + "    ", 1);
        writeDebug ("    " + String(toolSwitch[index].mainGate), 1);
        writeDebug ("tools ==> " + String(index) + "   VCC from file ==> " + String (toolSwitch[index].VCC), 1);
      }
    }

    pwm.begin();
    pwm.setPWMFreq(60);  // Default is 1000mS
    pinMode ( dust.dustCollectionRelayPin, OUTPUT);
    startWifiFromConfig ('^^', '^', 1);      // read the wifi section of the config file, start the wifi, and connect to the blynk server
    // initialize all the gates, dust collector and the baseline voltage for the switches          
    resetVoltageSwitches();
    setBlynkControls();
    turnOffDustCollection();
    closeAllGates (true);
    
//  }

}
/***************************************************
      MAIN Loop
 ***************************************************/
void loop()
{
  int   gateIndex = 0;
  int   activeTool = 50;
  if (manualOveride == false)
  {
    for (int i = 0; i < dust.NUMBER_OF_TOOLS; i++) //  loop through all the tools to see if current has started flowing
    {
      if (toolSwitch[i].switchID != -1)     //  only look at uncommented tools.   the # as first character of cfg file will disable switch
      {
        if ( checkForVoltageChange(i) )
        {
          if (toolSwitch[i].isON == true)
          {
            if (dust.DEBUG == true)
              writeDebug(String (toolSwitch[i].tool) + "  has been turned off, dust collector should go off", 1);

            // the running tool is off, and we need to stop looking at other tools and turn off the dust collector */

            toolSwitch[i].isON = false;
            toolON = false;
            i = dust.NUMBER_OF_TOOLS;
            activeTool = 50;
            break;
          }                 // end of toolsSwitch[i].isON
          activeTool = toolSwitch[i].switchID;
          gateIndex = toolSwitch[i].mainGate;
          toolSwitch[i].isON = true;
          toolON = true;
          i = dust.NUMBER_OF_TOOLS;
          openGate(gateIndex);
        } else                  // end of checkForVoltageChange
        {
          //  either there is no current which has started, or the machine is still running - need to test for which
          if (toolSwitch[i].isON == true)
          {
            i = dust.NUMBER_OF_TOOLS;
            if (dust.DEBUG == true)
            {
              writeDebug(String (toolSwitch[i].tool) + "  is still running, dumping out of checking loop", 1);
              writeDebug("    The current active tool is ==>  " + String(activeTool), 1);
            }
          }   // end of thet toolSwitch[i].isON
        }   // end of else
      }  // end of testing active tools.   This is the if statement above which test for active tool
    }                   // end of For loop where we are monitoring the tools
    if (activeTool != 50 )
    {
      // if activeTool is not = 50, it means there is current flowing through the sensor.   read the gate config map, open and close the appropriate gates

      if (collectorIsOn == false)
        runGateMap (gateIndex);

    } else                          // else of if(activeTool != 50)
    { // This means there was a current change, likely from machine ON to machine OFF
      if (collectorIsOn == true && toolON == false)
      {
        delay(dust.DC_spindown);
        turnOffDustCollection();
        activeTool = 50;
        resetVoltageSwitches();
      }
    }       // End of else section
  }                 // end of check for manual overide
  Blynk.run();
  timer.run();

}   //   end of loop()

/***********************************************************************
 *    in previous versions of the app, startWiFiFromConfig was used to 
 *    encrypt/decript the WIFI config txt file.   If there is to be a backup
 *    ability to restore the EEPROM, we either need to encode an SSID, Password, 
 *    auth key in the EEPROM tool, or, shift the BLYNK app to use the Blynk.Edgent model
 *    to store the blynk WIFI in the EEPROM of the wifi shield
 * 
 *    Really think through the options here and decide best way forward
 * **************************************************************************/
void startWifiFromConfig (char sectDelim, char delim[], int WifiStart)
  {
    int      index;
   if (dust.DEBUG == true)
    {
      writeDebug ("auth == > " + String (blynkWIFIConnect.auth), 1);
      writeDebug ("wifi coms speed ==> " + String(blynkWIFIConnect.ESPSpeed) + "  | Connection ==> " + String(blynkWIFIConnect.BlynkConnection), 1);
      writeDebug ("ssid ==> " + String(blynkWIFIConnect.ssid) + "  |  pass ==> " + String(blynkWIFIConnect.pass) + "  | server ==> " + String (blynkWIFIConnect.server) + " | serverPort ==> " + String(blynkWIFIConnect.serverPort), 1);
      //delay(500);
    }
    // Set ESP8266 baud rate
    EspSerial.begin(blynkWIFIConnect.speed);
     if (WifiStart)
      {
        if ( strcmp(blynkWIFIConnect.BlynkConnection, "Local") == 0 )
          {
            if (dust.DEBUG)
              writeDebug("trying to connect to the local server",1);
            Blynk.begin(blynkWIFIConnect.auth, wifi, blynkWIFIConnect.ssid, blynkWIFIConnect.pass, blynkWIFIConnect.server, blynkWIFIConnect.serverPort);
            
          //  Blynk.begin("UemFhawjdXVMaFOaSpEtM91N9ay1SzAI", wifi, "Everest", "<pass>", "192.168.1.66", <port>);
          }
        else if (strcmp(blynkWIFIConnect.BlynkConnection,"Blynk") == 0)
          {
            Blynk.begin(blynkWIFIConnect.auth, wifi, blynkWIFIConnect.ssid, blynkWIFIConnect.pass, "blynk-cloud.com", 80);
          }
        
      }  // end of if Wifi start  // end of if Wifi start
    ////  modified v6.0    using the UI now, instead of decrypting file in the Blynk UI
    Blynk.virtualWrite(V33, blynkWIFIConnect.ssid);
    Blynk.virtualWrite(V34, blynkWIFIConnect.pass);
    Blynk.virtualWrite(V35, blynkWIFIConnect.server);
    Blynk.virtualWrite(V36, blynkWIFIConnect.serverPort);
    Blynk.virtualWrite(V37, blynkWIFIConnect.ESPSpeed);
    Blynk.virtualWrite(V38, blynkWIFIConnect.BlynkConnection);
    Blynk.virtualWrite(V39, blynkWIFIConnect.auth);
    Blynk.syncVirtual(V33);
    Blynk.syncVirtual(V34);
    Blynk.syncVirtual(V35);
    Blynk.syncVirtual(V36);
    Blynk.syncVirtual(V37);
    Blynk.syncVirtual(V38);

}

/**************************************************
      void  setBlynkControls()
       used to set the lookup dropdown and other controls on the
       blynk app.  That is the manual controller for the gates and
       manually control the dust collector
 ****************************************************/
void setBlynkControls()
{
  BlynkParamAllocated clearout(256);    // valiable to clear the existing values from the menu dropdown
  BlynkParamAllocated switches(256);
  BlynkParamAllocated items(256); // list length, in bytes

  Blynk.setProperty(V11, "labels", clearout);    // clear all existing values from menu
  Blynk.setProperty(V2, "labels", clearout);     // clear all existing values from menu
  Blynk.setProperty(V18, "labels", clearout);      // clear all existing values from gate menue
  for (int i = 0; i < dust.NUMBER_OF_TOOLS; i++)
    items.add (toolSwitch[i].tool);
  Blynk.setProperty (V11, "labels", items);      // populate menu with the name of each tool
  Blynk.virtualWrite (V11, 1);
  for (int i = 0; i < dust.NUMBER_OF_GATES; i++)
    switches.add (blastGate[i].gateName);
  Blynk.setProperty(V2, "labels", switches);   // populate the menu with the name of each outlet
  Blynk.virtualWrite (V2, 3);
  Blynk.setProperty(V18, "labels", switches);
  Blynk.syncVirtual(V2);
  if (manualOveride == false)
    Blynk.virtualWrite(V6, false);
  else
    Blynk.virtualWrite(V6, true);
  Blynk.virtualWrite(V3, 0);
  Blynk.virtualWrite(V4, 0);
  Blynk.virtualWrite(V5, 2);
  Blynk.virtualWrite(V13, 0);
  Blynk.virtualWrite (V12, 1);
  Blynk.syncVirtual(V12);
  Blynk.virtualWrite(V16, 0);
  Blynk.syncVirtual(V16);
  Blynk.virtualWrite(V20, 0);
  Blynk.virtualWrite(V21, 0);
  Blynk.virtualWrite(V19, 2);
  Blynk.virtualWrite(V30, cypherKey);
  Blynk.syncVirtual(V30);
  Blynk.virtualWrite(V31, 2);
  Blynk.virtualWrite(V33, blynkWIFIConnect.ssid);
  Blynk.virtualWrite(V34, blynkWIFIConnect.pass);
  Blynk.virtualWrite(V35, blynkWIFIConnect.server);
  Blynk.virtualWrite(V36, blynkWIFIConnect.port);
  Blynk.virtualWrite(V37, blynkWIFIConnect.speed);
  Blynk.virtualWrite(V38, blynkWIFIConnect.auth);


  terminal.clear();
  confTerm.clear();
  encryptTerm.clear();
}

/**********************************************************
     void  runGateMap (num activeTool)
         sets all the gates open/close for the active tool
 **********************************************************/
void  runGateMap (int gateIndex)
{

  if (collectorIsOn == false)
    turnOnDustCollection();
  for (int s = 0; s < dust.NUMBER_OF_GATES; s++)
  {
    if (dust.DEBUG == true)
    {
      writeDebug ("gate map ==> " + String(blastGate[gateIndex].gateConfig), 1);
      writeDebug ("gate # clycling through map ==> " + String(s), 1);
      //delay(500);
    }
    if (blastGate[gateIndex].gateConfig[s] == '1')
      openGate(s);                     // if there is a '1' in the s index of the array, open the gate
    else
      closeGate(s, false);                     //   if there is a '0' in the s index of the array, close the gate
  }                         // end of the for loop
}

/*********************************************************
      void  closeAllGates( boolean initialize)
         quick function to loop through all the gates and close them
 *******************************************************************/
void  closeAllGates( boolean  initialize)
{
  for (uint8_t w = 0; w < dust.NUMBER_OF_GATES; w++)
    closeGate(w, initialize);

}

/*********************************************************
     boolean  checkForAmperageChange (int which)
         used to check the current sensors on the outlets.
         If current flow is detected, function returns true
         if no current, function returns FALSE
         This version has removed getVPP, instead it is a single read
         of the analog pin
         -->set the new baseline.   If the tool is turned off, there should be a delta from the running
         current and cause the logic to see a current change and respond true --> there is a current change
 ***********************************************************************/
boolean checkForVoltageChange(int which)
{
  float Voltage = 0;
  float VRMS = 0;
  float AmpsRMS = 0;

  Voltage = getVPP (toolSwitch[which].voltSensor);
  VRMS = abs(Voltage - (toolSwitch[which].VCC * 0.5) + .000);              //  set it to 0.000 if you want 0 amps at 0 volts
  AmpsRMS = (VRMS * 1000) / dust.mVperAmp;             // Sensitivirty defined by the rojojax model (.xxx)   See config file
  if (dust.DEBUG == true)
  {
    writeDebug ("*****************************\n", 1);
    writeDebug ("tool name ==> " +  String(toolSwitch[which].tool), 1);
    writeDebug ("   raw reading from sensor ==> ", 0);
    Serial.println (Voltage);
    writeDebug ("   baseline voltage from init ==> " + String (toolSwitch[which].voltBaseline), 1);
    writeDebug("___________________", 1);
    terminal.print ( toolSwitch[which].tool);
    terminal.print ("  voltage ==> ");
    terminal.println (Voltage);
  }
  /*********************************************
       modify this piece to check for appropriate change off the last parameter
       of the outlet sensor config line.   Right now, the bandsaw is not drawing enoug
       current to keep the collector on -  need to debug this
       toolSwitch[counter].ampThreshold
    *************************************************/
  if (abs(Voltage - toolSwitch[which].voltBaseline) > toolSwitch[which].voltDiff)
  {
    if ( toolSwitch[which].isON == true)
      return false;
    else
      return true;
  } else
  {
    //if the tool is on, we have to tell the calling condition we have turned the tool off, so we should return there has been a voltage change
    if (toolSwitch[which].isON == true)
      return true;
    else
      return false;
  }
}

/****************************************************
      turnOnDustCollection

 ********************************************/
void turnOnDustCollection() {
  if (dust.DEBUG == true)
    writeDebug ("turnOnDustCollection", 1);
  digitalWrite(dust.dustCollectionRelayPin, 1);
  collectorIsOn = true;
  Blynk.virtualWrite(V1, 1);
  if (manualOveride == true)
  {
    terminal.println( "turning dust collector on");
    terminal.flush();
  }
}

/*************************************************
   void turnOffDustCollection ()
 ************************************/
void turnOffDustCollection() {
  if (dust.DEBUG == true)
  {
    writeDebug("turnOffDustCollection", 1);

  }

  digitalWrite(dust.dustCollectionRelayPin, 0);
  collectorIsOn = false;
  Blynk.virtualWrite(V1, false);
  if (manualOveride == true)
  {
    terminal.println("turning dust collector off");
    terminal.flush();
  }
}

/**********************************************
   float getVPP (int sensor)
        used to be used to flatten the reading on the sesor
        no longer implemented in this code.   The function
        samples the analog pin a number of times, and returns
        the average value of the readings
 *********************************************************/
float getVPP(int sensor)
{
  float result;

  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here

  uint32_t start_time = millis();
  while ((millis() - start_time) < 300) //sample for .3 Sec
  {
    readValue = analogRead(sensor);
    // see if you have a new maxValue
    if (readValue > maxValue)
    {
      /*record the maximum sensor value*/
      maxValue = readValue;
    }
    if (readValue < minValue)
    {
      /*record the maximum sensor value*/
      minValue = readValue;
    }
  }

  // Subtract min from max
  result = maxValue - minValue;
  return result;
}

/****************************************************
   void closeGate ( uint8_t num)
       closes the gate passed in num.   The gate will speed to just
       before the bump stop and slows down to close the last
       little bit
 **********************************************************/
void closeGate(uint8_t num, boolean initialize)
{
  if (dust.DEBUG == true)
  {
    Serial.print (F("closeGate " ));
    Serial.println(blastGate[num].pwmSlot);
    // delay (50);
  }

  if (blastGate[num].openClose == HIGH || initialize == true)
  {
    if (manualOveride == true)
    {
      terminal.print("    closing gate ");
      terminal.println(blastGate[num].pwmSlot);
      terminal.flush();
    }
    pwm.setPWM(blastGate[num].pwmSlot, 0, blastGate[num].closePos - 20);
    delay(100);
    //        delay(20);    the above line is a test for a longer delay and quicker response on the gates
    for (int i = blastGate[num].closePos - 20; i <= blastGate[num].closePos; i += 1)
      pwm.setPWM (blastGate[num].pwmSlot, 0 , i);
    blastGate[num].openClose = LOW;
  }

}

/****************************************************
    void openeGate ( uint8_t num)
        opens the gate passed in num.   The gate will speed to just
        before the bump stop and slows down to open the last
        little bit
  **********************************************************/

void openGate(int num)
{
  int c;
  if (blastGate[num].openClose == LOW)
  {
    if (manualOveride == true)
    {
      terminal.print("    opening gate ");
      terminal.println(blastGate[num].pwmSlot);
      terminal.flush();
    }
    pwm.setPWM(blastGate[num].pwmSlot, 0, (blastGate[num].openPos - 20));
    delay(100);
    //       delay(20);  the above line is a test for a longer delay and quicker response on the gates
    for (c = (blastGate[num].openPos - 20); c <= blastGate[num].openPos; c += 1)
      pwm.setPWM (blastGate[num].pwmSlot, 0, c);
  }
  blastGate[num].openClose = HIGH;
}

/*********************************************************
     void   resetVoltageSwitches ()
         used to recalibrate the votage switches to a baseline
         so we can understand what flatline amperage we are calculating
         a difference from

 *********************************************************/
void  resetVoltageSwitches()
{

  float     reading;

  for (int i = 0; i < dust.NUMBER_OF_TOOLS; i++) // record the baseline amperage for each voltage sensor
    toolSwitch[i].voltBaseline = getVPP(toolSwitch[i].voltSensor);
}

/**********************************************************
 *  fuction to get the configuration data for the gates
 *  gates are initialized into EEPROM through the EEPROM_Dustcollection app
 *  and modified through the blink app associated with this solution
 * ********************************************************/
void readGatesEEPROM ()
  {
    eeAddress = GATE_ADDRESS;  
    for (int x = 0;  x < dust.NUMBER_OF_GATES; x++)
      {
        EEPROM.get(eeAddress, blastGate[x]);
        eeAddress += sizeof (blastGate[x]);
      }
  }

/**********************************************************
 *   function to read outlet configuration from EEPROM.  
 *    just like the gates, the outlet config initilization is 
 *    done through a separate app, and can be modified through 
 *    the blink app.   No UI associated to dust collection app
 *    the BLYNK app config and UI are the only way to interact
 *    with the system
 * ***********************************************************/
void readOutletsEEPROM()
  {
    eeAddress = OUTLET_ADDRESS;
    for (int x=0; x< outlets; x++)
      {
        EEPROM.get(eeAddress, toolSwitch[x]);
        eeAddress += sizeof(toolSwitch[x]);
      }
  }

/**********************************************************
 *  function to read the initial setup config for the system
 *  reading it from EEPROM.   The initial configuration is initialized
 *  to EEPROM through the EEPROM_DustCollection app and the 
 *  config txt file.   Once initialized, the parameters can be modified
 *  through teh BLYNK app
 * ***********************************************************/
void readConfigEEPROM()
  { 
    EEPROM.get (SET_CONFIG, dust);
  }

/***********************************************************
 *  function to read the wifi configuration for the app
 *  a wifi shield and connection is required for the BLYNK app 
 *  to connect to this microcontroller code.   The parameters are initilized
 *  through the EEPROMM_dustcollection code, with the config file being
 *  encrypted/decrypted by that code.    Once set, the parameters can be 
 *  modified on the WIFI section of the BLYNK app.   If a new initilzation
 *  txt file is required, an SD shield must be connected and an explicit encryption
 *  function from the BLYNK app is required to save the new parameters to the 
 *  initilziation file.
 * **************************************************************/
void readWIFIConfig()
  {
    EEPROM.get(WIFI_ADDRESS, blynkWIFIConnect);
    //blynkWIFIConnect.speed = atol(blynkWIFIConnect.ESPSpeed);
  }
/*************************************************************
 *  if there is an update to the WIFI settings in the BLYNK app 
 *  write the new values to the EEPROM at the WIFI_ADDRESS
 * ************************************************************/
void writeWIFIToEEPROM()
  {
    EEPROM.put(WIFI_ADDRESS, blynkWIFIConnect);
    EEPROM[EEPROM.length()-2] = WIFI_ADDRESS+sizeof(blynkWIFIConnect);
  }  