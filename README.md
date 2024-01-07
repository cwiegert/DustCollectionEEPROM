# DustCollectionEEPROM
**v. 6.0 --** moved all configuration from SD Files to EEPROM, and set Blynk app to be able to configure a new gate from the app.  Also reset the setup to have simple functions to read the sections from EEPROM.    Wrote the EEPROM initilization application which uses the old config file to populate EEPROM.    The code must be run with Blynk v1.0, as the new Blynk 2.0 app will require a rewrite and re-coding the connectivity.  There will be a new branch with v.6.1 to do the new connectivity

## Background ##
Having completed the [Router Lift Automation](https://github.com/cwiegert/RouterLift-v5.6-EEPROM), it was time to move on to another project.   I was tired of running back and forth to the dust collector, and opening/closing all the gates to a machine.   I found Bob's automation on his youtube channel [I like to Make stuff](https://www.youtube.com/watch?v=D1JWH425o7c) took his code, started the concept and modified it to make the shop as configurable as possible.

## The Code ##
Starting with the model similar to Bob's I just wanted to get the servo's, outlets and blast gates working.    Instead of hardcoding all the gate configurations, I decided to build on the configuration model used in the router lift project.   With 4 different configuration concepts, there are 4 different sections to the DustGatesConfiguration 53.cfg file.   

Even though there are a number of things going on on the code, it's really very simple.    The loop function will check to see if the system is in automated or manual mode.   If automated, a for/Next loop will cycle through the # of outlets and assess for a -1. (if outlet # is -1, the tool has been taken off line).  If the outlet is active, check for a voltage change.   If the tool is on, turn it off, if off - turn the tool on.   Voltage change is defined by baseline, or running voltage.  In the check for voltage, millis () is used to sample the voltage for a number of milliseconds to account for debouncing.   If there was sensed current, check to see if the dust collector is on.   If so - run the gate map, open and close the gates to the gate associated with the outlet.  That's it... 

on setup- the configuration is read from EEPROM, 
```        readConfigEEPROM();     
        readGatesEEPROM ();     
        readOutletsEEPROM();    
        readWIFIConfig();     
```        

the PWM is intialized
```
        pwm.begin();
        pwm.setPWMFreq(60);  // Default is 1000mS
        pinMode ( dust.dustCollectionRelayPin, OUTPUT);
```
wifi is started, the outlets are 0'd for baseline voltage, the Blynk app is intialized to ensure the names and values of the gates are loaded, the dust collector is turned off, and all the gates in the system are closed
```     startWifiFromConfig ('^^', '^', 1);
        resetVoltageSwitches();
        setBlynkControls();
        turnOffDustCollection();
        closeAllGates (true);
```
Also, to really take advantage of this system, There is a Blynk app I have created wich you can import and get your auth code from the system.   The [Blynk QR Code](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/BlynkApplication.jpeg) is available for your use.   Simply follow the [Blynk Sharing instructions](http://docs.blynk.cc/#sharing) and all the controls will link up with the event handlers defined in the main .ino file.

### System Config ###
As noted in the header section of the header section of the file, there is a specific character delimeter separating each config param.   The items in this section are used to set global variables that give overall system parameters which are used in loops and baseline counting.    

**servos** --> Number of servos, to define where they are connected to the pwm board.   The number of servos should be equivalent to the number of gates.   
**tools** --> Number of physical tools connected to the system.   This will be used in the main loop to monitor the outlets to detect change in current.  This value will determine how many elements will be in the outlet config section array.   If you add an outlet, you have to increment this #, if you take an outlet off the system, this # should be decremented

**Gates** --> Number of gates in the system.   Because there may be gates controlling trunks, and not direct to the tools, the number of servos may be different than the number of tools.   Determines how many lines to read from the GateConfig setting section.   if a gate is added to the system, this parameter has to be incremented.   If a gate taken out, this number must be decremented.

**Dust Collector Delay** --> number of milliseconds to delay before turning off the dust collector.   The delay is there to allow for residual sawdust to be cleaned from the pipes after a machine is turned off.  

**Dust Collector Power** --> The pin which controls the relay, which controls the heavy duty relay to power the dust collector. 

**Manual Gate** --> Pin used by the Blynk app to set the system to Manaual control instead of automated monitoring.   If this Pin is set to TRUE, the main loop will not automatically monitor the outlets.   The manual mode is used to set the open/close limits of the servo.

**Sensitivity** --> sets the monitoring sensitivity of the outlets.   The larger the #, the less sensitive the system will be.   This is especially useful with multi-speed motors (bandsaw) and noisy household voltage.

**debounce** --> not currently used.   Was going to use it in the checkForVoltageChange() but have not implemented

**DEBUG** --> when set to TRUE, debug messages will appear in both the Blynk app and serial monitor.   Used as a mechanism to turn on debugging without having to recompile/reload the code.   Useful when monitoring voltage and when doing configuration of the gate's open/close limits

The baseline system configuration below has 13 servos, 9 tools, 13 gattes, 2000 millisecond delay, has the dust collector power driven by Pin 11, has the Blynk app manual button connected to Pin 12, has a voltage sensitivity of 100 mV, does a 200 microsecond debounce and is set in DEBIG mode.

servos | tools | Gates | Dust collector delay | Dust Collector power | Manual gate | sensitivity | debounce | DEBUG
********************************************************************************/

�13�9�13�2000�11�12�100�200�1�

### Gate Config section ###
This section will set the real workhorse section of the code.   Once a voltage change is detected, the system will change the state of the main gate associated with that outlet.   The link between the outlet and the gate is defined in the outlet section below.   The most important piece of this config is the gate switch map, as it determines what state to set every gate in the sytem.   The idea being, with trunks along the way to the primary tool, the system will open and close the gates to leave only the most efficient path to the active tool.   By setting every gate in the system, a reletively small dust collector will run very efficently as only 1 tool's gate will be open at a time.   The premise is built on the fact you won't leave a tool running when unattended, and only a single outlet switch will be active at any 1 time.

**Parsing Token** --> throw away market to the new section.   When written with the [SD card version](https://github.com/cwiegert/DustCollection) there had to be a way to break the sections of the file.   This new token tells the reader it's reached a new config section of the file.

**gate #** --> As it says, this is the gate index used in the loops when opening and closing gates

**pwmSlot** --> Port # of the PWM shield used to control the servos.   

**gate #** --> As it says, this is the gate index used in the loops when opening and closing gates

**Name** --> Used to diplay the gate name in the Blynk controlling app.   Used on the main screen and configuration screens "gate" dropdown box

**Open** --> Setting to define the open position of the servo.   these are set in the baseline config, but will need to be modified/monitored to not peg the servo's in an unreachable state.   If the open poisiton is beyond the stop point of the gate, the servo will continuously try to set to the end point and burn up the windings.  (8 burned up servo's already.... and counting)

**Close** --> Setting to define the close position of the servo

**gate Switch map** --> Used to define the state of every gate in the system.   if '1', the gate will be opened, if '0' the gate will be closed.   This array defines the path to the tool gate, and is the configuration for the runGateMap(int) function.   

the baseline configuration below is for Gate 0, plugged into PWM slot 0, and is the gate on the Left Y of the collector, with a gate open value of 480, close value of 375, is the 1st gate in the system and will be the only gate opened all other gates should be closed.

parsing token | gate # | pwmSlot | Name | open | close | gate switch map

^^
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

^^

ޮ�0�0�Collector Left�480�375�1,0,0,0,0,0,0,0,0,0,0,0,0�

### Outlet Config section ###
Set up like the gate config, there is a line here for each outlet in the system.   The tools parameter from the system section, will define how many outlet structures will be populated, and the counter for the main loop.

**switch ID** --> Index # of the switch.   This determines which structure to load the remaining config parameters for each outlet

**Name** --> which tool is the outlet controlling.   Used in the Blynk config and monitoring application and used in the Serial monitor debugging.  Otherwise, not used

**analog Pin** --> the arduino PIN the outlet is connected to.   The checkForVoltageChange() function will do the read off the analog pin, and assess for voltage change

**volt baseline** --> the baseline reading of the voltage through an outlet at steady state.  Used to measure a "0" value 

**main Gate** --> links to a gate config line in the gate section.   The outlet/gate combiniation will drive the gate map section to open/close gates for a direct route to the machine

**Voltage Supplied** --> what is the amp threashold a to calculate amperage

**voltDiff** --> change in voltage that will signle the switch state on the machine has changed.   If on, there will be a drop in voltage, if off, there will be an increase in voltage.   

The baseline configuration for the below outlet shows the Miter Saw is plugged into outlet index 0, is plugged into analog Pin 3,  with a baseline noise of 9.5 volts, is driving gate index 3, has 5 volts supplied the voltage sensor and has to see 100 volt difference before signaling to change state.

^^   switch Id | name | analog pin | volt baseline | main Gate | Voltage supplied | voltDiff

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

^^

ޮ�0�Miter Saw�3�9.50�3�5.0�100�

### Wifi Config Section ###
Because there was no serial monitoring and ability to configure the gates open/close parameter without re-uploading the app, I used the [Blynk](https://blynk.io) system to connect to the arduino through wifi solution.   With that, there was a need to set the connection parameters without having to recompile/upload a system.   This section was modified in this version to use WIFI and no longer read the encrypted wifi file.   Have a look in the [EEPROM](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/EEPROM_Writer_DustCollector/README.md) to get the config of the parameters for connecting to wifi and linking the Blynk app to the arduino.

## The Setup - system and hardware ##

All created with an Arduino Mega 2560 - I needed the memory for all the variables and runtime arrays.   Plugged to that is an SD Card shield and a EPS8266 WIFI shield.   Configuring each of those shield are well documented and I won't spend time here.    

Similar to the setup in Bob's original design, using 1/2" baltic birch plywood, I creates a servo holder for for each of the aluminum blast gates, and cut/milled aluminum swing arm to mount to the servo horn.   The swing arm attaches to the blast gate flange with a simple #8 machine screw, and mounts to the horn with 3MM allen set screws.   It's important to have the gate installed correctly, to ensure they don't stick.  The tightening knob on the gate, should be on the flow side opposite the dust collection.   In otherwords, the knob should tighten in the direction the air is flowing back to the dust collector.   The gates are designed to be installed that way, and you will have less trouble with the gate sticking - which in turn will burn up your servo's.

Here is an example of the [prototyping](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/Gates%20prototype.MOV) work and a demonstration of the bracket, servo, arm and .... unfortulately,.... the gates sticking.   

Each of the [servo's](https://www.amazon.com/gp/product/B07VJG5QTJ/ref=ppx_yo_dt_b_asin_title_o05_s00?ie=UTF8&psc=1) are mounted with the [hanger](https://www.amazon.com/gp/product/B07Q2VP8P4/ref=ppx_yo_dt_b_asin_title_o05_s00?ie=UTF8&psc=1) and  will connect to a pin set on the [PWM Contoller](https://www.amazon.com/gp/product/B01D1D0CX2/ref=ppx_yo_dt_b_asin_title_o02_s01?ie=UTF8&psc=1).   ![image](https://github.com/cwiegert/DustCollectionEEPROM/assets/33184701/37dfbf4e-9c02-49f6-9474-0c7f14701a1d)
Obviously, wire is needed to be run from the contoller to the servers, and for that I used 4 wire thermostat wire with [3 pin connectors](https://www.amazon.com/gp/product/B07ZHB4BBY/ref=ppx_yo_dt_b_asin_title_o07_s00?ie=UTF8&psc=1) wrapped with shrink tubing.   Be sure to lable the voltage and ground side, as once the shrink tubing is on, you can't see the wire colors.

Each of the [voltage sensors](https://www.amazon.com/gp/product/B07SPRL8DL/ref=ppx_yo_dt_b_asin_title_o08_s00?ie=UTF8&psc=1) are connected to the system with the same 3 pin connectors.![image](https://github.com/cwiegert/DustCollectionEEPROM/assets/33184701/4b19b3b5-0f25-4f7e-8712-b7f5ebb5c55c)
and, similar to Bob, I wired them in to an outlet connected to each machine.   That outlet has a male plug and cord attached that plugs into the wall, and the machine is plugged into the current sensing outlet.    I wanted to make sure I could move the sensor with the machine and not have to modify my in wall shop outlets.  Again, I used the 3 wire theromostat wire to connect the outlet to the analog pins and 5 volts power shource.

Each of the [servo's](https://www.amazon.com/gp/product/B08KDW4757/ref=ppx_yo_dt_b_search_asin_image?ie=UTF8&th=1) are mounted with the [hanger](https://www.amazon.com/gp/product/B07Q2VP8P4/ref=ppx_yo_dt_b_asin_title_o05_s00?ie=UTF8&psc=1) and  will connect to a pin set on the [PWM Contoller](https://www.amazon.com/gp/product/B01D1D0CX2/ref=ppx_yo_dt_b_asin_title_o02_s01?ie=UTF8&psc=1).   Obviously, wire is needed to be run from the contoller to the servers, and for that I used 4 wire thermostat wire with [3 pin connectors](https://www.amazon.com/gp/product/B07ZHB4BBY/ref=ppx_yo_dt_b_asin_title_o07_s00?ie=UTF8&psc=1) wrapped with shrink tubing.   Be sure to lable the voltage and ground side, as once the shrink tubing is on, you can't see the wire colors.

To control the dust collector, I used a heavy duty 30 amp, 110V [relay](https://www.amazon.com/gp/product/B01MCWO35P/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1) with the [thermo grease](https://www.amazon.com/gp/product/B07NV27PDX/ref=ppx_yo_dt_b_asin_title_o00_s01?ie=UTF8&psc=1) and heat sink to control the switch on the dust collector.   If you have a Jet Canister style like I did, the switch is wired as a 110v/240v switch.   That means, you have to wire the H switch relay to account for the 2 power "on" wires in teh dust collector.  For liability reasons, you will need to look that up yourself, as I am not a licensed electician and should not give you that advice.    However, the relay has a 12V switch side, that is connected to a 12V relay, which is powered through the dust collector PIN from the config file.   When the PIN is set to HIGH - 12v are sent through the relay, to the heavy duty relay, and the current flows to the dust collector and vice versa.   whan the PIN is LOW, the dust collector is off.

For the mounting and distribution of power, see my [router](https://github.com/cwiegert/RouterLift-v5.6-EEPROM) write up, as I used the same boxes, power distribution and rail mounted power sources as there.   1x 5V, 1x 12V and the rails to mount them to the wall.

