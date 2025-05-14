# CO2
Measure CO2 concentration indoors with the TTGO microcontroller and S8 sensor
This project was developed with the project: https://letsair.org/
 The resources can be found here:https://nousaerons.fr/makersco2/
 Programs for testing are available on this github
 1- ESP32 devmodule : https://github.com/Hana205/CO2/blob/main/esp32RGB.ino
 2-esp32 devmodule with Bluetooth code and app
 https://github.com/Hana205/CO2/blob/main/ESP32_S8_BLUETOOTH.ino
 https://github.com/Hana205/CO2/blob/main/ESP32_S8_BLUETOOTH.apk
 3-TTGo vesion with TFT dispaly
 https://github.com/Hana205/CO2/blob/main/ttgo.ino
 
 for the wiring go to https://github.com/Hana205/CO2/blob/main/co2meter_nousaerons.pdf
 
install Arduino version 1.8.18 here : https://www.arduino.cc/en/software/OldSoftwareReleases
install ESP32: add this line to the preferences and Additional Boards management URLs : https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json *Open Boards Manager from Tools > Board menu and install ESP32 : select 2.0.6 version for TTGO version platform (and don't forget to select your ESP32 board from Tools > Board menu after installation).
If your esp32 is not recognized please install the driver : https://github.com/Hana205/Ledcontrol/blob/main/CP210x_Universal_Windows_Driver.zip
 download the zip file : decompress it 2 : go to computer peripherals: go to USB and port com: right click on the port with an error and indicate the place of CP210x_Universal_Windows_Driver

 install TFT library for display with TTGO
 https://github.com/Xinyuan-LilyGO/TTGO-T-Display/tree/master/TFT_eSPI
 TTGo version contribution https://co2.rinolfi.ch/ 
 
