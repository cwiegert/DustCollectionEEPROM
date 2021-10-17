## WIFI config for the EEPROM version ##

This version will decrypt the [Config File](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/EEPROM_Writer_DustCollector/DustWifi%20v53.cfg) through the Blynk app, and set the wifi connection parameters through the Blynk app.   However the [parameter definition](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/DustWifi%20v53%20--%20keep%20this%20around%20for%20restore.cfg) has the original format of how parameters are set in the structure.    in Setup, the EEPROM app will decrypt and read the wifi config file name found in the main app configuration file.   The last line of the file is the name of the wifi config file.   To initialize the system from scratch, the startWifiFromConfig() will need a hard coded connection string to define how to link the arduino to the Blynk app.  Once that connection string is set, you can use the Encrypt section of the blynk app to reset the parameters in the EEPROM file.    

To start, open the .ino file, search for startWifiFromConfig() select whether you are running local or to the blynk server, and redefine the commented line 639.   The line is currently commented and used for the reference on how to hard code parameters for initilization.   Documentation can be found [here](blynk.io) and can be a bit tricky to figure out.   first and formost, connect the blynk app to "A" blynk server - whether local or the blynk.com server - and connect the blynk app to the server - then generate a key.   That key, will be the auth key in the parameters setting.

[Blynk Application QR Code](https://github.com/cwiegert/DustCollectionEEPROM/blob/main/DustCollection_v60_10_10_2021/BlynkApplication.jpeg)

  

