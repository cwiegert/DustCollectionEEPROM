# DustCollectionEEPROM
**v. 6.0 --** moved all configuration from SD Files to EEPROM, and set Blynk app to be able to configure a new gate from the app. 

## Background ##
Having completed the [Router Lift Automation](https://github.com/cwiegert/RouterLift-v5.6-EEPROM), it was time to move on to another project.   I was tired of running back and forth to the dust collector, and opening/closing all the gates to a machine.   I found Bob's automation on his youtube channel [I like to Make stuff](https://www.youtube.com/watch?v=D1JWH425o7c) took his code, started the concept and modified it to make the shop as configurable as possible.

## The Code ##
Starting with the model similar to Bob's I just wanted to get the servo's, outlets and blast gates working.    Instead of hardcoding all the gate configurations, I decided to build on the configuration model used in the router lift project.   With 4 different configuration concepts, there are 4 different sections to the DustGatesConfiguration 53.cfg file.   

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

the baseline configuration below is for Gate 0, plugged into PWM slot 0, and is the gate on the Left Y of the collector, wiht a gate open value of 480, close value of 375, is the 1st gate in the system and will be the only gate opened all other gates should be closed.

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

