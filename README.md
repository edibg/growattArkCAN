# growattArkCAN
ESP32 project for Growatt ARK LiFePo4 battery CAN BMS protocol gateway

Main goal of this project is to create a CAN gateway between the Growatt ARK2.5L LiFePo4 battery and Growatt SPF5000ES inverters.

## Hardware
  MCP2515 dual CAN hat https://www.waveshare.com/2-ch-can-hat.htm. The CAN HAT is connected ot VSPI port on NudeMCU, the remaining ports are defined in the sketch.
  NodeMCU32-S
  
## Software
  -Arduino IDE 2.0.0
  -CRC by  RobTillaart 0.3.1 https://github.com/RobTillaart/CRC (not yet used)
  -MCP_CAN_lib by coryjfowler 1.5.0 https://github.com/coryjfowler/MCP_CAN_lib
  -esp32 by espressif 2.0.5 https://github.com/espressif/arduino-esp32
  
## History  
I have another DIY LiFePo4 battery working in pralell with Growark ARK-2.5L, but I want to keep the digital interface between the ARK and the invertor. Since the ARK sets all the charging parameters, using 500kbit/s CAN interface, when I connected the DIY battery the charge current is set to 25A and shared ~ 20% to the ARK and 80% to the DIY battery. Charging 15kW DIY battery with 25A is quite slow and since only around 20% of the charge current goes to the ARK I can set the total current to 75A, 100A and still be within the ARK specificed limits.

## Growatt BMS Protocol (L51 in SPF5000ES)
ID=0x311 Send by battery and includes all limits, related to the battery charging are set by the battery by broadcasting 8 bytes long CAN message with ID = 0x311. 
- [D0:D1] = max charging voltage * 0.1V - example 56.8V(0x02,0x38)
- [D2:D3] = max charging current * 0.1A - 25A(0x00,0xFA), 50A(0x01,0xF4), 60A(0x02,0x58)
- [D4:D5] = max discharging currne * 0.1A - 25A(0x00,0xFA)
- [D6:D7] = status bits.
  -D7  : 0x?1  - stend by;0x?2  - charging;0x?3  - discharging

ID=0x313 - Send by the battery and include some of current statuses
- [D0:D1] = System voltage * 0.1V
- [D2:D3] = Total current * 0.1A passing trough the ARK BMS. If the value is greater than 0 - charging, if it lower than 0 discharging.
- [D4:D5] = unknown
- [D6] = SOC in %
- [D7] = may be SOH but not confirmed.

CAN Bus wires the middle 2 wires in RJ45 connector - when using standard ethernet cable those are the BLUE (CAN_H) and BLUE-WHITE (CAN_L).

## How it works
Usually the ARK BMS broadcasts messages ever 500mS. To be able to change the limits received by the inverter we need a CAN gateway to separate the BMS CAN bus from the inverter CAN bus. This can be accomplished by the available dual CAN HAT for Raspberry Pi. In my case CAN1 is attached to ARK BMS side, CAN0 to inverter side.

CAN gateway, when receives a message on CAN1 checks if the message is with id=0x311 or 0x313. 
- 0x311 - when this id is received, the last 2 bytes [D6:D7], where the BMS statuses are located, are copied to the array called id311[6:7] and remaining bytes are altern accordingly to the set max currents/votlages. Then send modified message on CAN0.
- 0x313 - when this id is received it is checked the SOC and calculate max charging current and if BMS report actual battery charging current greater than defined in "ark_max_current" set the inverter charging limit to 25A ( default one when only one ARK battery in used). Then send the message wihtout any modification to CAN0.

If any other id is received on CAN1 it is transmited to CAN0. 

On CAN0 the invertor is transmitting only one static message with id=0x301 that is re-translated to CAN1 when received.

## Current Issues
- In some cases the DIY batteries stop charging ( I guess due high cell voltage), but the ARK battery is still charging. That is why there is a logic, when the SOC is increased the invertor charging current is decresed, until reaches 25A. This is done in func. charge_current_limit(). 
- Even the prev. one is some cases the DIY battery is disconnected ( not charging) that leads all the current to goes to the ARK battery. In this case everytime when id 0x313 is received bytes [D2:D3] are checked for overcurrent if such a case is present the charging current is limit to 25A. (to be tested).

## Next Steps
- Currently my DIY battery uses custom BMS with bluetooth LE that I managed to reverce engineer, so I plan to read its parameters and amend the routines to take them in to account.
- To add WiFi capabilities to report to local server. Currently I have a running python server that reads the Growatt Plants data from growatt server and siplay it in a form of a table in web page, but will add the information and from my DIY battery.
- This may be used in case of other type of BMSs are used to make a protocol convertor.
