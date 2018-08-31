# Geiger-counter

With 2x sbm-20 tubes, 1.3" Oled display (7pins), Single button on/off control (with led), wireless charging,
Atmega328p, only exotic component is mcp73871 (lipo charger IC). 

5v to 400v boost features IPD90R1k2C3 mosfet, runs at ~22kHz (outside human hearing),
1mH (ELL8TP102MB) inductor is barely enough for this frequency (mainly ESR), also used tantalum capacitors have high ESR,
PID controlled using atmega328p's 16bit counter.

single ~3000mAh lipo cell, 705065 in size.




