## WIFI config for the EEPROM version ##

This app is used to initialize or chanage the dust collection configuration and load all the settings to EEPROM.   The dust collection app will not read the SD card, it will pull all settings from EEPROM, and the Blynk app defined below will be used to change those settings once your MCU is initilized.   To initilze the MCU, install the DustGatesDefinition v53.cfg onto an SD card, and load this .ino onto the board.   Setup will read all the config sections and give you menu options to load read sections of the confiuration.   

In the [global variables](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/EEPROM_Writer_DustCollector/DustCollectorGlobals.h) you should set the EERPOM addresses where you want each section stored.   Be careful of adding gates or outlets, as you will need enough space for the additional structures.   There is enough memory space for 16 outlets and 20 blast gates.  and the baseline configuration can be loaded as an EEPROM image if you use option R in the main menu.   Option R will read DustCollectorEEPROM.cfg and load it byte by byte to EEPROM.   If you open the file in VSCode, you will be able to see the various parameters define in raw data format.   THere is a section towards the end of the file where a number of # signs are highlighed in a stream by an IP address.   That is ssid passcode placeholder.  Modify that, modify your IP address, update the auth code and you can start the wifi config on initialize of the MCU


This version will decrypt the [Config File](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/EEPROM_Writer_DustCollector/DustWifi%20v53.cfg) through the Blynk app, and set the wifi connection parameters through the Blynk app.   However the [parameter definition](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/DustWifi%20v53%20--%20keep%20this%20around%20for%20restore.cfg) has the original format of how parameters are set in the structure.    in Setup, the EEPROM app will decrypt and read the wifi config file name found in the main app configuration file.   The last line of the file is the name of the wifi config file.   To initialize the system from scratch, the startWifiFromConfig() will need a hard coded connection string to define how to link the arduino to the Blynk app.  Once that connection string is set, you can use the Encrypt section of the blynk app to reset the parameters in the EEPROM file.    

Alternatively, open the .ino file, search for startWifiFromConfig() select whether you are running local or to the blynk server, and redefine the commented line 639.   The line is currently commented and used for the reference on how to hard code parameters for initilization.   Documentation can be found [here](http://docs.blynk.cc/#getting-started-getting-started-with-the-blynk-app) and can be a bit tricky to figure out.   first and formost, connect the blynk app to "A" blynk server - whether local or the blynk.cloud server - and connect the blynk app to the server - then generate a key.   That key, will be the auth key in the parameters setting.

[Blynk Application QR Code](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/BlynkApplication.jpeg) is the baseline code for the app created specifically for this system.   If you use the app, it should conenct directly to the Virtual Pin configration defined in the event handlers of the main line code

**_____________________________________________________**

***Initial Install of hardware and configuring EEPROM***

**_____________________________________________________**

When doing a clean install, there are several steps involved in getting the automation running correctly.   After all the piping, gates, wiring and electronics are installed, it's time to configure the servo's to open and close appropriately.   As documented in the config section of the [Readme.md](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/README.md) the # of gates, servos, outlets, machines, and gatemaps have been set in the config file, and it needs to be loaded to EEPROM.    

Load this .ino to the Arduino and run it.   The menu options are set in runtime order... run A,B,C in subsequent order, but I would suggest to not load any custom bits.   
```
Serial.println("");
Serial.println (F("enter an <X> if you want to clear the EEPROM"));
Serial.println(F("enter an <A> if you want to LLOOAADDD the settings to EEPROM"));
Serial.println(F("enter an <B> if you want to RREEAADD the settings from EEPROM"));
Serial.println(F("enter an <C> if you what the GATES section LLOOADDEEEDD"));
Serial.println(F("enter an <D> if you want to RREEAADD the GATES config from EEPROM"));
Serial.println(F("enter an <E> if you want to LLOOAADD the OUTLETS section to EEPROM"));
Serial.println(F("enter an <F> if you want to RREEAADD the  OUTLET config from EERPOM"));
Serial.println(F("enter an <G> if you want to LLLOOAADD the WIFI settings"));
Serial.println(F("enter an <H> if you want to RREEEAADDD the WIFI settings"));
Serial.println(F("enter <L> if you want to see what's in EEPROM"));
Serial.println(F("enter an <S> to export the EEPROM for archiving"));
Serial.println(F("enter an <R> to recover EEPROM from file"));
```
Once the menu items are complete, all of the set points for what free memory is available, and where to start righting in EEPROM are maintained int eh global variables held in DustCollectorGlobas.h, and the end of each section is held in EEPROM cells defined in the code using  EEPROM.length()-1, -10, -8.  For instance, when a new gate is added, the last position of the gate section needs to be update to account for the structure being added to EEPROM

```
BLYNK_WRITE (V24)   //  Added in v6.0  It's the button to write the new gate to EEPROM
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
```

Once the EEPROM is loaded, the main Dust Collection .ino will need to be loaded to the Arduino.   Again, I used the Mega platform because of the amount of memory needed to handle all the events from the BLYNK app, and the arrays held for the gate bitmaps.   Typically, the main gate for each machine is the gate closest to the machine and linked to the Switch ID defined below.    the logic of the system is Main gate drives everything from the machine back to the dust collector with an "end of the line back" programming logic. 

Now, with the main app running, I suggest disconnecting the servo's from the gates.   Even though the open/close values are set in the config, the way in which you set the servo's in the hangers will determine the exact values you want for the open/close limits.   If there are limit values which are outside the range of movement for your gate length of travel, the servos will continuously attempt to get move to the limit value, and burn itself up.   The servo arm will be pegged against a stop, but the PWM will continue to push steps, thereby burining up the electronics.   

With the BLYNK app in manual mode, navigate to teh gate config, select the gate, and manually open / close the servo gate and get close to where you think your limits are.   Once you are close, connect the gate and test / reset your limits until you get the appropriate travel on each gate.    The gatemap field on the BLYNK app is sequential order mapping for the gates defined by gate # in EEPROM.   I find it easiest to put the gates # into the system sequentially, as it simplifies debugging the open / close pattern for the gates.    Once you have your limits set, populate the GateMap field with the exact number of gates you have defined in the NumberOfGates variable in your globals.    a '1' will open the corresponding gate, a '0' will close the gate.   

Repeat the above for all gates in the system.

If you, like I have on many occasions, burn up a servo and need to replace it.   Treat the servo as if you have a new install, by not connecting to the gate until you have the open Limit and close Limit set.   All replacement configuration is done through the BLYNK app.

The pattern used to set the gates is the same pattern used to set the outlets.    This applicaiton is written assuming there is only 1 get open at a time and the main gate associate with the outlet on the machine drives the status of the system.   Following the tempalte above, navigate to teh Outlet Config screen on the BLYNK app, and set the switch id to the appropriate outlet.  The switch ID has a MainGate paramenter which corresponds to the PWM position and drives which gate is the main gate associated to the machine. **Remember Switch ID/Volt Sensor/Main Gate combination links the entire system together.**
