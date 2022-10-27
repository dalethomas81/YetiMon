# YetiMon
 
YetiMon is an open source add-on for your Goal Zero Yeti X series that monitors the voltage level of the battery and switches to an auxilary charging source such as AC mains once the capacity of the Yeti has depleted past a threshold.  

This is useful in applications where you want to use only solar power but want to automatically switch to another power such in situations where there has not been suficient sun to maintain your Yeti.  

Video describing how YetiMon works: https://youtu.be/-7pzLf8jMJA  
Video showing YetiMon installed: https://youtu.be/F3phBXkWJbo  
Video showing how to calibrate YetiMon: https://youtu.be/rwJIze3Ulzg  

More info to come!  

## Build Pictures
Money Shot  
<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/build/money-shot.jpeg" alt="menu" width="400"/>  

Build  
<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/build/top-down-full-no-cover.jpeg" alt="menu" width="400"/>  

Installed  
<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/build/installed-cover-open.jpeg" alt="menu" width="400"/>  

Display  
<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/build/oled-display-boot.jpeg" alt="menu" width="400"/>  

Functional Diagram  
<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/build/functional-diagram.jpeg" alt="menu" width="400"/>  

Grafana  
<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/build/grafana.png" alt="menu" width="400"/>  

Web app  
<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/build/web-page.png" alt="menu" width="400"/>  

## 3D Printing 

The "chassis" of YetiMon is just the factory cover that came with the Yeti X and is normally removed when you install a Yeti Link. What I did was use this as the chassis for YetiMon and 3D printed a cover that I made (link [here](source/cad/fusion-360)).  

The cover is qutie large and had to be printed vertically on my Ender 5.
<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/3d-printing/chassis-vertical-cura.png" alt="menu" width="400"/>  

Additionally, I 3D printed a case for the oled display. I got this design off of Thingiverse. Hopefully the link still exist at the time of you reading this. If not, let me know and I will design one. [Here](https://www.thingiverse.com/thing:2176764) is the link.  

## Electrical Schematic  
Below is the Fritzing diagram that shows the connections clear as if they were made on a breadboard. The source file can be found [here](source/cad/fritzing).  

<img src="https://github.com/dalethomas81/YetiMon/blob/main/media/drawings/fritzing.png" alt="menu" width="400"/>  

Sources for some of the Fritzing parts:  
https://github.com/squix78/esp8266-fritzing-parts  
https://github.com/adafruit/Fritzing-Library  
https://arduinomodules.info/ky-019-5v-relay-module/  

## Parts List  
Parts:  
| Part | Qty | Description | Link |
| ---- | --- | ----------- | ---- |
| MCU | 1 | NodeMCU ESP8266 Dev Board | |
| Relay | 1 | How power SPDT relay 30A | |
| Display | 1 | OLED Display SSD1306 128x64 | |
| Temp Sensor | 1 | MAX31855 thermocouple amplifier breakout | |
| Thermocouple | 1 | Type K thermoucouple | |
| Power Supply | 1 | 12v to 5v Buck converter with USB connector | |
| R1 | 1 | 100K resistor | |
| R2 | 1 | 10K potentiometer | |
| R3 | 1 | 15K resistor | |
| R4 | 1 | 10K resistor | |

Consumables:  
| Description | Link |
| ----------- | ---- |
| Power pole Connectors | |
| 12awg silicon wire | |
| Solid core wire | |
| Kapton tape | |
| Zip ties | |

Tools:  
1. Drill and bits
2. Soldering iron
3. Crimping tool
4. Wire strippers
5. Various small screwdrivers