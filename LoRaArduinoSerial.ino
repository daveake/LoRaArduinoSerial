// LoRa receiver sending packets to serial port
// Intended for use with a bluetooth module for comms to an Android phone, but usable for anything with a serial connection

#include <string.h>
#include <ctype.h>
#include <SPI.h>

/*---------------------------------------------------*\
|                                                     |
|               Arduino Rx - Bluetooth Tx             |
|               Arduino Tx - Bluetooth Rx             |
|               Arduino  8 - RFM DIO0                 |
|               Arduino  9 - RFM DIO5                 |
|               Arduino 10 - RFM NSS                  |
|               Arduino 11 - RFM MOSI                 |
|               Arduino 12 - RFM MISO                 |
|               Arduino 13 - RFM CLK                  |
|                                                     |
\*---------------------------------------------------*/

// RFM98
int _slaveSelectPin = 10; 
String content = "";
char character;
int dio0 = 3;
// int dio5 = 9;
byte currentMode = 0x81;
unsigned long UpdateClientAt=0;
double Frequency=434.448;
int ImplicitOrExplicit;
int ErrorCoding;
int Bandwidth;
int SpreadingFactor;
int LowDataRateOptimize;

#define REG_FIFO                    0x00
#define REG_FIFO_ADDR_PTR           0x0D 
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_RX_NB_BYTES             0x13
#define REG_OPMODE                  0x01
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS               0x12
#define REG_PACKET_SNR              0x19
#define REG_PACKET_RSSI             0x1A
#define REG_RSSI_CURRENT            0x1B
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_PAYLOAD_LENGTH          0x22
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_HOP_PERIOD              0x24
#define REG_FREQ_ERROR              0x28
#define REG_DETECT_OPT              0x31
#define REG_DETECTION_THRESHOLD     0x37

// MODES
// MODES
#define RF96_MODE_RX_CONTINUOUS     0x85
#define RF96_MODE_SLEEP             0x80
#define RF96_MODE_STANDBY           0x81

#define PAYLOAD_LENGTH              80

// Modem Config 1
#define EXPLICIT_MODE               0x00
#define IMPLICIT_MODE               0x01

#define ERROR_CODING_4_5            0x02
#define ERROR_CODING_4_6            0x04
#define ERROR_CODING_4_7            0x06
#define ERROR_CODING_4_8            0x08

#define BANDWIDTH_7K8               0x00
#define BANDWIDTH_10K4              0x10
#define BANDWIDTH_15K6              0x20
#define BANDWIDTH_20K8              0x30
#define BANDWIDTH_31K25             0x40
#define BANDWIDTH_41K7              0x50
#define BANDWIDTH_62K5              0x60
#define BANDWIDTH_125K              0x70
#define BANDWIDTH_250K              0x80
#define BANDWIDTH_500K              0x90

// Modem Config 2

#define SPREADING_6                 0x60
#define SPREADING_7                 0x70
#define SPREADING_8                 0x80
#define SPREADING_9                 0x90
#define SPREADING_10                0xA0
#define SPREADING_11                0xB0
#define SPREADING_12                0xC0

#define CRC_OFF                     0x00
#define CRC_ON                      0x04


// POWER AMPLIFIER CONFIG
#define REG_PA_CONFIG               0x09
#define PA_MAX_BOOST                0x8F
#define PA_LOW_BOOST                0x81
#define PA_MED_BOOST                0x8A
#define PA_MAX_UK                   0x88
#define PA_OFF_BOOST                0x00
#define RFO_MIN                     0x00

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23  // 0010 0011
#define LNA_OFF_GAIN                0x00


char Hex[] = "0123456789ABCDEF";

void SetParametersFromLoRaMode(int LoRaMode)
{
  LowDataRateOptimize = 0;
  
  if (LoRaMode == 7)
  {
    ImplicitOrExplicit = EXPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_5;
    Bandwidth = BANDWIDTH_20K8;
    SpreadingFactor = SPREADING_7;
  }
  else if (LoRaMode == 6)
  {
    ImplicitOrExplicit = IMPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_5;
    Bandwidth = BANDWIDTH_41K7;
    SpreadingFactor = SPREADING_6;
  }
  else if (LoRaMode == 5)
  {
    ImplicitOrExplicit = EXPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_8;
    Bandwidth = BANDWIDTH_41K7;
    SpreadingFactor = SPREADING_11;
  }
  else if (LoRaMode == 4)
  {
    ImplicitOrExplicit = IMPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_5;
    Bandwidth = BANDWIDTH_250K;
    SpreadingFactor = SPREADING_6;
  }
  else if (LoRaMode == 3)
  {
    ImplicitOrExplicit = EXPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_6;
    Bandwidth = BANDWIDTH_250K;
    SpreadingFactor = SPREADING_7;
  }
  else if (LoRaMode == 2)
  {
    ImplicitOrExplicit = EXPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_8;
    Bandwidth = BANDWIDTH_62K5;
    SpreadingFactor = SPREADING_8;

  }
  else if (LoRaMode == 1)
  {
    ImplicitOrExplicit = IMPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_5;
    Bandwidth = BANDWIDTH_20K8;
    SpreadingFactor = SPREADING_6;

  }
  else if (LoRaMode == 0)
  {
    ImplicitOrExplicit = EXPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_8;
    Bandwidth = BANDWIDTH_20K8;
    SpreadingFactor = SPREADING_11;
    LowDataRateOptimize = 0x08;
  }
}

// initialize the library with the numbers of the interface pins

void setup()
{
  Serial.begin(9600); //   57600);

  Serial.println("");
  Serial.println("LoRa USB Receiver V1.2");
  Serial.println("");

  SetParametersFromLoRaMode(1);

  setupRFM98();
}

void loop()
{
  CheckPC();
  
  CheckRx();
  
  UpdateClient();
}

void UpdateClient(void)
{
  if (millis() >= UpdateClientAt)
  {
    int CurrentRSSI;

    if (Frequency > 525)
    {
      CurrentRSSI = readRegister(REG_RSSI_CURRENT) - 157;
    }
    else
    {
      CurrentRSSI = readRegister(REG_RSSI_CURRENT) - 164;
    }
    
    Serial.print("CurrentRSSI=");
    Serial.println(CurrentRSSI);
    
    UpdateClientAt = millis() + 1000;
  }
}

double FrequencyReference(void)
{
  switch (Bandwidth)
  {
    case  BANDWIDTH_7K8:  return 7800;
    case  BANDWIDTH_10K4:   return 10400; 
    case  BANDWIDTH_15K6:   return 15600; 
    case  BANDWIDTH_20K8:   return 20800; 
    case  BANDWIDTH_31K25:  return 31250; 
    case  BANDWIDTH_41K7:   return 41700; 
    case  BANDWIDTH_62K5:   return 62500; 
    case  BANDWIDTH_125K:   return 125000; 
    case  BANDWIDTH_250K:   return 250000; 
    case  BANDWIDTH_500K:   return 500000; 
  }
}


double FrequencyError(void)
{
  int32_t Temp;
  double T;
  
  Temp = (int32_t)readRegister(REG_FREQ_ERROR) & 7;
  Temp <<= 8L;
  Temp += (int32_t)readRegister(REG_FREQ_ERROR+1);
  Temp <<= 8L;
  Temp += (int32_t)readRegister(REG_FREQ_ERROR+2);
  
  if (readRegister(REG_FREQ_ERROR) & 8)
  {
    Temp = Temp - 524288;
  }

  T = (double)Temp;
  T *=  (16777216.0 / 32000000.0);
  T *= (FrequencyReference() / 500000.0);

  return -T;
} 

int receiveMessage(unsigned char *message)
{
  int i, Bytes, x, currentAddr;

  Bytes = 0;

  x = readRegister(REG_IRQ_FLAGS);
  // printf("Message status = %02Xh\n", x);
  
  // clear the rxDone flag
  // writeRegister(REG_IRQ_FLAGS, 0x40); 
  writeRegister(REG_IRQ_FLAGS, 0xFF); 
   
  // check for payload crc issues (0x20 is the bit we are looking for
  if((x & 0x20) == 0x20)
  {
    // printf("CRC Failure %02Xh!!\n", x);
    // reset the crc flags
    writeRegister(REG_IRQ_FLAGS, 0x20); 
  }
  else
  {
    currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR);
    Bytes = readRegister(REG_RX_NB_BYTES);
    // printf ("%d bytes in packet\n", Bytes);

    // printf("RSSI = %d\n", readRegister(REG_RSSI) - 137);
	
    writeRegister(REG_FIFO_ADDR_PTR, currentAddr);   
    // now loop over the fifo getting the data
    for(i = 0; i < Bytes; i++)
    {
      message[i] = (unsigned char)readRegister(REG_FIFO);
    }
    message[Bytes] = '\0';

    // writeRegister(REG_FIFO_ADDR_PTR, 0);  // currentAddr);   
  } 
  
  return Bytes;
}

void ReplyOK(void)
{
  Serial.println('*');
}

void ReplyBad(void)
{
  Serial.println('?');
}

void SetFrequency(char *Line)
{
  double Freq;

  Freq = atof(Line);

  if (Frequency > 0)
  {
    ReplyOK();

    Frequency = Freq;
    
    Serial.print("Frequency=");
    Serial.println(Frequency);

    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}

void SetMode(char *Line)
{
  int Mode;

  Mode = atoi(Line);

  if ((Mode >= 0) && (Mode <= 7))
  {
    ReplyOK();

    SetParametersFromLoRaMode(Mode);
    
    Serial.print("Mode=");
    Serial.println(Mode);

    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}

void SetBandwidth(char *Line)
{
  Serial.print("BW len="); Serial.print(strlen(Line)); Serial.print(" <"); Serial.print(Line); Serial.println(">");
  if (strcmp(Line, "7K8") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_7K8;
    startReceiving();
  }
  else if (strcmp(Line, "10K4") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_10K4;
    startReceiving();
  }
  else if (strcmp(Line, "15K6") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_15K6;
    startReceiving();
  }
  else if (strcmp(Line, "20K8") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_20K8;
    startReceiving();
  }
  else if (strcmp(Line, "31K25") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_31K25;
    startReceiving();
  }
  else if (strcmp(Line, "41K7") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_41K7;
    startReceiving();
  }
  else if (strcmp(Line, "62K5") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_62K5;
    startReceiving();
  }
  else if (strcmp(Line, "125K") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_125K;
    startReceiving();
  }
  else if (strcmp(Line, "250K") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_250K;
    startReceiving();
  }
  else if (strcmp(Line, "500K") == 0)
  {
    ReplyOK();
    Bandwidth = BANDWIDTH_500K;
    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}

void SetErrorCoding(char *Line)
{
  int Coding;

  Coding = atoi(Line);

  if ((Coding >= 5) && (Coding <= 8))
  {
    ReplyOK();
    ErrorCoding = (Coding-4) << 1;
    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}

void SetSpreadingFactor(char *Line)
{
  int Spread;

  Spread = atoi(Line);

  if ((Spread >= 6) && (Spread <= 12))
  {
    ReplyOK();
    SpreadingFactor = Spread << 4;
    startReceiving();
  }
  else
  {
    ReplyBad();
  }
}

void SetImplicit(char *Line)
{
  int Implicit;

  Implicit = atoi(Line);

  ReplyOK();
  
  ImplicitOrExplicit = Implicit ? IMPLICIT_MODE : EXPLICIT_MODE;
  startReceiving();
}

void SetLowOpt(char *Line)
{
  int LowOpt;

  LowOpt = atoi(Line);

  ReplyOK();
  
  ImplicitOrExplicit = LowOpt ? 0x08 : 0;
  
  startReceiving();
}

void ProcessCommand(char *Line)
{
  char Command;

  Command = Line[1];
  Line += 2;
       
  if (Command == 'F')
  {
    SetFrequency(Line);
  }
  else if (Command == 'M')
  {
    SetMode(Line);
  }
  else if (Command == 'B')
  {
    SetBandwidth(Line);
  }
  else if (Command == 'E')
  {
    SetErrorCoding(Line);
  }
  else if (Command == 'S')
  {
    SetSpreadingFactor(Line);
  }
  else if (Command == 'I')
  {
    SetImplicit(Line);
  }
  else if (Command == 'L')
  {
    SetLowOpt(Line);
  }
  else
  {
    ReplyBad();
  }
}

void CheckPC()
{
  static char Line[32];
  static int Length=0;
  char Character;

  while (Serial.available())
  { 
    Character = Serial.read();
    
    if (Character == '~')
    {
      Line[0] = Character;
      Length = 1;
    }
    else if (Length >= sizeof(Line))
    {
      Length = 0;
    }
    else if (Length > 0)
    {
      if (Character == '\r')
      {
        Line[Length] = '\0';
        ProcessCommand(Line);
        Length = 0;
      }
      else
      {
        Line[Length++] = Character;
      }
    }
  }
}

void CheckRx()
{
  if (digitalRead(dio0))
  {
    unsigned char Message[256];
    int Bytes, SNR, RSSI, i;
    long Altitude;
    
    Bytes = receiveMessage(Message);
    
    Serial.print("FreqErr="); Serial.println(FrequencyError()/1000.0);

    SNR = readRegister(REG_PACKET_SNR);
    SNR /= 4;
    
    if (Frequency > 525)
    {
      RSSI = readRegister(REG_PACKET_RSSI) - 157;
    }
    else
    {
      RSSI = readRegister(REG_PACKET_RSSI) - 164;
    }
    
    if (SNR < 0)
    {
      RSSI += SNR;
    }
    
    Serial.print("PacketRSSI="); Serial.println(RSSI);
    Serial.print("PacketSNR="); Serial.println(SNR);
    

    // Serial.print("Packet size = "); Serial.println(Bytes);

    // Telemetry='$$LORA1,108,20:30:39,51.95027,-2.54445,00141,0,0,11*9B74

    if (Message[0] == '$')
    {
      Serial.print("Message=");
      Serial.println((char *)Message);
    }
    else if (Message[0] == '%')
    {
      char *ptr, *ptr2;

      Message[0] = '$';
      
      ptr = (char *)Message;
      do
      {
        if ((ptr2 = strchr(ptr, '\n')) != NULL)
        {
          *ptr2 = '\0';
          Serial.print("Message=");
          Serial.println(ptr);
          ptr = ptr2 + 1;
        }
      } while (ptr2 != NULL);
    }
    else
    {
      Serial.print("Hex=");
      for (i=0; i<Bytes; i++)
      {
        if (Message[i] < 0x10)
        {
          Serial.print("0");
        } 
        Serial.print(Message[i], HEX);
      }
      Serial.println();
    }
  }
}


/////////////////////////////////////
//    Method:   Change the mode
//////////////////////////////////////
void setMode(byte newMode)
{
  if(newMode == currentMode)
    return;  
  
  switch (newMode) 
  {
    case RF96_MODE_RX_CONTINUOUS:
      writeRegister(REG_PA_CONFIG, PA_OFF_BOOST);  // TURN PA OFF FOR RECIEVE??
      writeRegister(REG_LNA, LNA_MAX_GAIN);  // LNA_MAX_GAIN);  // MAX GAIN FOR RECIEVE
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
      
      break;
    case RF96_MODE_SLEEP:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
    case RF96_MODE_STANDBY:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
    default: return;
  } 
  
  if(newMode != RF96_MODE_SLEEP){
    //while(digitalRead(dio5) == 0)
    //{
      // 
    //} 
    delay(10);
  }
   
  return;
}


/////////////////////////////////////
//    Method:   Read Register
//////////////////////////////////////

byte readRegister(byte addr)
{
  select();
  SPI.transfer(addr & 0x7F);
  byte regval = SPI.transfer(0);
  unselect();
  return regval;
}

/////////////////////////////////////
//    Method:   Write Register
//////////////////////////////////////

void writeRegister(byte addr, byte value)
{
  select();
  SPI.transfer(addr | 0x80); // OR address with 10000000 to indicate write enable;
  SPI.transfer(value);
  unselect();
}

/////////////////////////////////////
//    Method:   Select Transceiver
//////////////////////////////////////
void select() 
{
  digitalWrite(_slaveSelectPin, LOW);
}

/////////////////////////////////////
//    Method:   UNSelect Transceiver
//////////////////////////////////////
void unselect() 
{
  digitalWrite(_slaveSelectPin, HIGH);
}

void SetLoRaFrequency()
{
  unsigned long FrequencyValue;
  double Temp;
  
  Temp = Frequency * 7110656 / 434;
  FrequencyValue = (unsigned long)(Temp);

  Serial.print("FrequencyValue is ");
  Serial.println(FrequencyValue);

  writeRegister(0x06, (FrequencyValue >> 16) & 0xFF);    // Set frequency
  writeRegister(0x07, (FrequencyValue >> 8) & 0xFF);
  writeRegister(0x08, FrequencyValue & 0xFF);
}

void SetLoRaParameters()
{
  writeRegister(REG_MODEM_CONFIG, ImplicitOrExplicit | ErrorCoding | Bandwidth);
  writeRegister(REG_MODEM_CONFIG2, SpreadingFactor | CRC_ON);
  writeRegister(REG_MODEM_CONFIG3, 0x04 | LowDataRateOptimize);                  // 0x04: AGC sets LNA gain
  writeRegister(REG_DETECT_OPT, (readRegister(REG_DETECT_OPT) & 0xF8) | ((SpreadingFactor == SPREADING_6) ? 0x05 : 0x03));  // 0x05 For SF6; 0x03 otherwise
  writeRegister(REG_DETECTION_THRESHOLD, (SpreadingFactor == SPREADING_6) ? 0x0C : 0x0A);    // 0x0C for SF6, 0x0A otherwise

}

void startReceiving()
{
  setMode(RF96_MODE_SLEEP);
  writeRegister(REG_OPMODE,0x80);  
  setMode(RF96_MODE_SLEEP);

  SetLoRaFrequency();
  
  SetLoRaParameters();
  
  writeRegister(REG_PAYLOAD_LENGTH, 255);
  writeRegister(REG_RX_NB_BYTES, 255);
  
  writeRegister(REG_FIFO_RX_BASE_AD, 0);
  writeRegister(REG_FIFO_ADDR_PTR, 0);
  
  // Setup Receive Continous Mode
  setMode(RF96_MODE_RX_CONTINUOUS);
}

void setupRFM98(void)
{
  // initialize the pins
  pinMode( _slaveSelectPin, OUTPUT);
  pinMode(dio0, INPUT);
  // pinMode(dio5, INPUT);

  SPI.begin();
  
  startReceiving();
  
  Serial.println("Setup Complete");
}
