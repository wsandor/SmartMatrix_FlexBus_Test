# SmartMatrix_FlexBus_Test
I made this test project to check whether Kinetis's FlexBus could be used for output with SmartMatrix on Teensy 3.6. 

This is a simple test project (for PlatformIO. with small changes it can be used with Arduino IDE as well) based on the SmartMatrix library. 

My aim was to check whether the FlexBus can be used to output data to LED panels. (In some way I think it is similar to the I2S bus on ESP32, so maybe parts of that work could be used wit FlexBus on Teensy also) Theoreticaly FlexBus can output 32 bits paralel - which would mean 5pcs HUB75 ports. Unfortunatly on the Teensy 3.6 not all FlexBus pins are available, so I use it in 16 bit mode, which means 2pcs HUB75 ports. The FlexBus interface generates the write clock for itself, so it is not needed to send every frame twice - less time, less memory. The schematic I used for the test can be found in the TeensyRGB_FlexBus_Test_sch.pdf file.
The project worked, so the FlexBus can be used to send out data paralell on 2pcs HUB75 ports. However this is not a finished project, the code is not 'clean', there could be errors also (I am still a beginner in the ARM/Arduino/C++ world), but I hope this work could be useful for others to get ideas from it.
Beside the FlexBus there are some minor improvements in the library (supporting larger fonts, supporting smaller bit depths, some new module mappings.
