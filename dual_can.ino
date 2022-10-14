// Original code from : 
//    Demo: Dual CAN-BUS Shields, Data Pass-through
//    Written by: Cory J. Fowler
//    January 31st 2014
//    This examples the ability of this library to support more than one MCP2515 based CAN interface.
//    https://lastminuteengineers.com/handling-esp32-gpio-interrupts-tutorial/
// 
// Last edited 13.OCT.22 by
// 
/* //millis() example
int period = 1000;
unsigned long time_now = 0;
void loop() {
    if(millis() - time_now > period){
        time_now = millis();
        //do something}}*/
#include <mcp_can.h>
#include <SPI.h>

#define INT_0  16   //CAN0 Interrupt pin
#define INT_1  17   //CAN1 Interrupt pin

//initial charging limit is 25A(0x00,0xFA), 50A(0x01,0xF4), 60A(0x02,0x58), 65A(0x02,0x8A), 70A(0x02,0xBC), 75A(0x02,0xEE)
#define id311_d2  0x02
#define id311_d3  0x58
//initial charging voltages is 56.8V(0x02,0x38), 56.9V(0x02,0x39)
#define id311_d0  0x02
#define id311_d1  0x39
//max charging currents
#define max_current 60          //max charging current in A delivered from the inverters
#define ark_max_current 20      //max charging current in A for the ARK battery, read by its BMS in id=0x313[2,3]

unsigned long rxId;
byte len;
byte rxBuf[8];

byte id311[] = {id311_d0,id311_d1,id311_d2,id311_d3,0x01,0xF4,0x00,0x61};

MCP_CAN CAN0(22);     // CAN0 interface usins CS on digital pin 10
MCP_CAN CAN1(21);     // CAN1 interface using CS on digital pin 9

void setup()
{
  Serial.begin(115200);
  pinMode(INT_0, INPUT_PULLUP);
  pinMode(INT_1, INPUT_PULLUP);
  
  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK)   //Change the frequency of the MCP2515 crystal used
  {    
    Serial.print("CAN0: Init OK!\r\n");
    CAN0.setMode(MCP_NORMAL);
  } 
  else Serial.print("CAN0: Init Fail!!!\r\n");
  
  if(CAN1.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK)
  {
    Serial.print("CAN1: Init OK!\r\n");
    CAN1.setMode(MCP_NORMAL);
  } 
  else Serial.print("CAN1: Init Fail!!!\r\n");
  
  SPI.setClockDivider(SPI_CLOCK_DIV2);         // Set SPI to run at 8MHz (16MHz / 2 = 8 MHz)
}

unsigned int charge_current_limit(byte soc)
{
  // Calculate invertors charge limit based of current SOC of ARK battery.
  // When the SOC increase the max charging current decreses. 
  // This is made with the idea if some point the DIY battery cut outs (for example due overvoltage of some cells)
  // to not force charge the ARK battery with overcurrent. Usually this can happend when the SOC% of batteries is
  // above 80%.
  unsigned int current_limit;
  
  // if SOC>100 or SOC<20%, keep the charging current to 25A
  if(soc>100 || soc<20)
  {
    current_limit = 25;
  }
  else
  {
    current_limit=25+(100-soc);
    if (current_limit>max_current)
    {
      current_limit = max_current;
    }
  }
  return current_limit * 10;
}

void loop(){
  //when Bluetooth and Wifi is running to try with interrupt driven type
  if(!digitalRead(INT_0))
  {                                           // If pin 2 is low, read CAN0 receive buffer
    CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    CAN1.sendMsgBuf(rxId, 0, len, rxBuf);     // Send CAN MSG to the ARK without any changes
  }

  if(!digitalRead(INT_1))
  {                                             // If INT_1 is low, read CAN1 receive buffer - there is MSG from ARK
    CAN1.readMsgBuf(&rxId, &len, rxBuf);        // Read the MSG send by ARK battery
    if(rxId == 0x311)                           // If MSG is with id 0x311 - contains charging voltage and current limits
    {
      id311[6]=rxBuf[6];                        // Copy the status bytes from the ARK to be send to Inverter
      id311[7]=rxBuf[7];
      CAN0.sendMsgBuf(rxId, 0, len, id311);    // Send the modified MSG to the invertor (CAN0)
    }
    else if(rxId == 0x313)                     // SOC and actual battery pack current are available in id=0x313
    {
      unsigned int current_limit = charge_current_limit(rxBuf[6]); //charging current set based on SOC of ARK battery
      
      // If DIY battery is not charging to avoid overchage currnet to the ARK battery check what is actual current. 
      // if it above than the set value in ark_max_current, limit it to 25A
      if((rxBuf[2]&0x80) == 0x00)   //when charging the value is positive, when discharging the value is negative. Note: it is other way araound when reading trough the Inverter API
      {
        unsigned int BMS_current =  rxBuf[2] * 0x100 + rxBuf[3];
        if(BMS_current > (ark_max_current*10) )
        {
          current_limit = 250;
        }
      }
      id311[2] = current_limit / 0x100;
      id311[3] = current_limit % 0x100;
      CAN0.sendMsgBuf(rxId, 0, len, rxBuf);       // Send id 0x313 MSG to inverters - to monitor SOC
    }
    else
    {
      CAN0.sendMsgBuf(rxId, 0, len, rxBuf);
    }
  }

}
