
#IoT Button
=====================
Setup for physical button to hit an api and perform an action over wifi.

Arduino Files
-------------
The arduino file were constructed using the [www.inotool.org](Ino Command Line Tool) and the actual .ino file exists inside src. Specific arduino settings including board-model and baud-rate set in ino.ini file.

Libraries
--------
Using the [Adafruit CC3000 Library](https://github.com/adafruit/Adafruit_CC3000_Library) for wireless internet communication and the CC3000 Shield.

Quick Start
----------
* Install InoTool `pip install ino`
* Build arduino file `ino build`
__If build fails due to the Robot Control library remove that library from Arduino.app/Contents/Resources/Java/libraries__
* Upload to arduino `ino upload`
* To monitor serial comm `ino serial`
__Baud rate set to 115200 in ino.ini file__

