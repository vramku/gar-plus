#include <chibi.h>
#include <chibiUsrCfg.h>

#include <SPI.h>
#include <Adafruit_RA8875.h>
#include <Adafruit_mfGFX.h>
#include <fonts.h>



// Define Reciver ID
#define ME 1234  
//TODO: need someway to reduce this to 16 bits or 65535 max

//Define the Screen Pins
// Connect SCLK to UNO Digital #13 (Hardware SPI clock)
// Connect MISO to UNO Digital #12 (Hardware SPI MISO)
// Connect MOSI to UNO Digital #11 (Hardware SPI MOSI)
#define RA8875_INT 3
#define RA8875_CS 10
#define RA8875_RESET 9

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);
uint16_t tx, ty;

// Define the radio rcv
#define CHANNEL 3
#define BPSK_MODE 3
#define CHB_RATE_250KBPS 0
#define FREAKDUINO_LONG_RANGE 1

//Define AES Encription 
uint8_t AESKEY = 329093092;

const int linelength = 16;
const int maxlines = 2;

// Define PIR sensor
int calibrationTime = 30;        
long unsigned int lowIn;         
long unsigned int pause = 5000;  
boolean lockLow = true;
boolean takeLowTime;  
int pirPin = 4;    

void setup()
{
  
  Serial.begin(9600);
  
  // Init Screen Display
  if (!tft.begin(RA8875_800x480)) {
    Serial.println("RA8875 Not Found!");
    while (1);
  }

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);
  tft.fillScreen(RA8875_BLACK);

  tft.textMode();
  tft.setCursor(20,20);
  tft.setTextColor(RA8875_YELLOW);
  PrintOnDisplay("MTA GAR+ 2.0");

  // PIR calibration
  for(int i = 0; i < calibrationTime; i++){
    Serial.print(".");
    delay(1000);
  }
  Serial.println("calibration completed");

  delay(50);

    
  // Init the chibi wireless stack
  chibiInit();
  chibiSetChannel(CHANNEL);
  chibiAesInit(&AESKEY);
  chibiSetMode(BPSK_MODE);
  chibiSetDataRate(CHB_RATE_250KBPS);
  //chibiHighGainModeEnable();
  
  chibiSetShortAddr(ME);

  Serial.println("Ready!");
}

void loop(){
  MotionDetector();
  
   //constantly receive
  if (chibiDataRcvd() == true)
  {
     byte rssi = chibiGetRSSI();
     Serial.print("RSSI = "); 
     Serial.println(rssi, HEX);
      
     byte buff[256];
     chibiGetData(buff);
      
     char data[256];
     strncpy(data, (char*) buff, 256);
    
     String temp = "";
    
     char* line = strtok(data, "\n");
     while(line != 0){
      int type = 0;//0 is minutes, 1 is stops
      char* separator = strchr(line, '@');//@ is minute symbol, # is stops away symbol
      if(separator == 0){
        separator = strchr(line, '#');
        type = 1;
      }
      
      if(separator != 0){
        //parse the information
        *separator = 0;
        Serial.print("Route: ");
        Serial.print(line);
        
        ++separator;
        int distance = atoi(separator);
        
        temp = temp + line + ": " + distance;
        
        if(type == 0){
          Serial.print(" Minutes Away: ");
          temp = temp + "min\n";
        }else{
          Serial.print(" Stops Away: ");
          temp = temp + "stops\n";
        }
        
        Serial.print(distance);
        Serial.println();
        
      }
      
      line = strtok(0, "\n");
    }
    
    String out = temp.substring(0, 16) + "-" + String(rssi, DEC) + "db up:" + (millis()/1000);
    PrintOnDisplay(out);
  }
}

void PrintOnDisplay(String data){
    tft.setCursor(20,20);
    tft.setFont(COMICS_8);
    tft.setTextSize(10);
    tft.print(data);
}

void MotionDetector(){
  
     if(digitalRead(pirPin) == HIGH){
       if(lockLow){  
         lockLow = false;            
         Serial.print("Motion detected at ");
         Serial.print(millis()/1000);
         Serial.println(" sec"); 
         Serial.print("Screen On");
         tft.displayOn(true);
         delay(50);
         }         
         takeLowTime = true;
       }

     if(digitalRead(pirPin) == LOW){       
        if(takeLowTime){
        lowIn = millis();          
        takeLowTime = false;       
        }
       if(!lockLow && millis() - lowIn > pause){  
           lockLow = true;                        
           Serial.print("motion ended at ");      //output
           Serial.print((millis() - pause)/1000);
           Serial.println(" sec");
           Serial.print("Screen OFF");
           tft.displayOn(false);
           delay(50);
           }
       }
}
