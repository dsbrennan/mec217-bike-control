# MEC 217 E-Bike Control
This repository contains the example [Arduino](https://www.arduino.cc/) code for the University of Sheffield MEC 217 module. The premise of the control portion of the module is to build an e-bike control system. When a user is pedalling the bike, the control system will power up the motor to a predefined maximum speed and maintain this speed whilst the user is pedalling. If the user stops pedalling the system must smoothly power down the motor.

## System parameters
The system will only activate powering up the motor once it has detected that the bike is being pedalled by a user. The system must only power up the motor to make the wheel of the bike turn at the predefined maximum speed. The wheel must not exceed this predefined maximum speed. Once the wheel has reached the predefined maximum speed, it must be maintained at this speed as long as the user is still pedalling the bike. If at any point, the user stops pedalling the bike, the control system must smoothly power down the motor to a stop.

## Sensor theory (how to)
The idea behind how the included sensors can achieve the system described above is outlined below.

If we place a magnet on the crank of the pedal and then place a hall effect sensor on the frame of the bike, we can detect everytime that the pedal (crank) passes the bike frame. Therefore, if we keep a time record of every instance of the hall effect sensor getting activated (an event), we can say that if two of these events happened within a predefined time frame, that someone is pedalling the bike. Conversely, if we haven't received one of these events within a predefined time frame, we can say that the user has stopped pedalling the bike.

The same hall effect principle can be applied to determine the wheel speed of the bike. If we place a magnet on the spoke of the wheel and then place a hall effect sensor on the frame of the bike. We could use this to calculate the wheel speed of the bike to determine if we need more or less power to the ESC.

## Download
To use this code, simply head over to the [release section](https://github.com/dsbrennan/mec217-bike-control/releases) and download the latest zip file. Once the zip file is extracted on your computer, you should be able to open this up in the [Arduino IDE](https://www.arduino.cc/en/software).

## Pins
The Arduino needs to communicate with certain inputs/outputs on numbered `pins`. For the purpose of this module, this combination of input/output to `pin` number have been fixed on the bike rig and cannot be changed. You must ensure that any code you upload onto the Arduino connected to the bike rig uses the numbered `pins` highlighted below.

| System | Input/Output | Purpose | Pin Number |
| --- | --- | --- | --- |
| Electronic speed controller (ESC) | Output | Control the speed of the motor. | 9 |
| LED | Output | Show a visual sign that the system has completed any startup tasks and is ready to be used. | 5 |
| LED | Output | Show a visual sign that the hall effect sensor connected to the pedal crank has been activated. | 4 |
| LED | Output | Show a visual sign that the system is telling the motor to power up. | 6 |
| Hall effect sensor | Input | Detects movement on the crank - the magnet attached to pedal has physically passed by the hall effect sensor attached to the bike frame, thus detecting the bike is being pedalled. | 2 |
| Hall effect sensor | Input | Detects movement on the wheel - the magnet attached to the wheel has physically passed by the hall effect sensor on the frame, thus providing the ability to calculate the speed that the wheel is travelling. | 3 |


## Status LEDS
There are three LEDS that show the status of the control system. Any modifications you do to the code, must use the status LED's to indicate the current status of the system:
- Green (Pin 5) - Status: indicated that the system has completed its initialisation and is ready to be used
- Yellow (Pin 4) - Crank activity: indicates that the crank connected to the pedal has travelled passed the sensor
- Red (Pin 6) - Motor activity: indicates that the motor is currently being powered