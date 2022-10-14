# growattArkCAN
ESP32 project for Growatt ARK LiFePo4 battery CAN BMS protocol gateway

Main goal of this project is to create a CAN gateway between the Growatt ARK2.5L LiFePo4 battery and Growatt SPF5000ES inverters.

## Hardware
  MCP2515 dual CAN hat https://www.waveshare.com/2-ch-can-hat.htm. The CAN HAT is connected ot VSPI port on NudeMCU, the remaining ports are defined in the sketch.
  NodeMCU32-S
  
## Software
  Arduino IDE 2.0.0
  CRC by  RobTillaart 0.3.1 https://github.com/RobTillaart/CRC (not yet used)
  MCP_CAN_lib by coryjfowler 1.5.0 https://github.com/coryjfowler/MCP_CAN_lib
  esp32 by espressif 2.0.5 https://github.com/espressif/arduino-esp32
  
## History  
I have another DIY LiFePo4 battery working in pralell with Growark ARK-2.5L, but I want to keep the digital interface between the ARK and the invertor. Since the ARK sets all the charging parameters, using 500kbit/s CAN interface, when I connected the DIY battery the charge current is set to 25A and shared ~ 20% to the ARK and 80% to the DIY battery. Charging 15kW DIY battery with 25A is quite slow and since only around 20% of the charge current goes to the ARK I can set the total current to 75A, 100A and still be within the ARK specificed limits.

All limits, related to the battery charging are set by the battery by broadcasting 8 bytes long CAN message with ID = 0x311. 
  [D0:D1] = max charging voltage * 0.1V - example 56.8V(0x02,0x38)
  [D2:D3] = max charging current * 0.1A - 25A(0x00,0xFA), 50A(0x01,0xF4), 60A(0x02,0x58)
  [D4:D5] = max discharging currne * 0.1A - 25A(0x00,0xFA)
  [D6:D7] = status bits.
    D7  : 0x?1  - stend by
        : 0x?2  - charging
        : 0x?3  - discharging
  
