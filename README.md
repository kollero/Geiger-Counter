# Geiger-counter version 4 C rev

Uses wireless charger so it can be made waterproof. PCB is also narrower and components are more tightly packed. New STL files are needed. 

With 2x sbm-20 tubes, 1.3" Oled display (7pins), Single button on/ (hold) off control (with led), wireless charging,
Atmega328p, only exotic component is mcp73871 (lipo charger IC). 

Display shows time the device has been on (up to 999hours), battery charge after 5 seconds running, 5 second mean of radiation in the middle, current high voltage running to the tubes, and mean value of radiation since the device has been turned on. When detection is made the on/off buttons led is flashed and a chirp is generated. 

5v to 400v boost features IPD90R1k2C3 mosfet, runs at ~22kHz (outside human hearing),
1mH (ELL8TP102MB) inductor is barely enough for this frequency (mainly ESR), also used tantalum capacitors have high ESR, PWM is
PID controlled using atmega328p's 16bit counter.

single ~5000mAh lipo cell, 806090 in size.




