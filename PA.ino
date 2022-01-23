//Semua Library dan pendefenisian//
#include <EEPROM.h>
#include <DallasTemperature.h>
#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Servo.h>
#include <OneWire.h>
#define ONE_WIRE_BUS 14 //D5 
#define FIREBASE_AUTH JD4krO2qCMZDE9FytjkYkh2ddsycxPCba963IQzN
#define countof(a) (sizeof(a) / sizeof(a[0]))

//Semua pemanggilan Class//
FirebaseData firebaseData;
OneWire oneWire(ONE_WIRE_BUS);
ThreeWire myWire(4, 5, 0); // D2, D1, D3 DAT,CLK,RST
RtcDS1302<ThreeWire> Rtc(myWire);
Servo servo;
DallasTemperature sensors(&oneWire);

//Semua Penginisialisasian//
const char* ssid = "ALFITRA";
  const char* password = "qwerty123";
const int analogInPin = A0; 
char datestring[20];  
float suhu;
int sensorValue = 0;
int relayInput = 16; //D0
unsigned long int avgValue; 
float b;
int buf[10],temp;
int cek = 0;
long previousMillis = 0;
unsigned long currentMillis;
float pHVol;
float phValue;
String recData, konfirmasi, recServo;
int pakan1,pakan2,pakan3,pakan4,menit;


void setup() {
   EEPROM.begin(512);
   Serial.begin(9600);
   sensors.begin();
   pinMode(relayInput, OUTPUT);
   servo.attach(2); //D4
   Serial.begin(9600);
    
   Serial.print("compiled: ");
   Serial.print(__DATE__);
   Serial.println(__TIME__);
    
   Rtc.Begin();
   
   RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
   printDateTime(compiled);
   Serial.println();
    
   konekWifi();
   Firebase.begin("https://tugasakhir-2f0d2-default-rtdb.firebaseio.com/", "JD4krO2qCMZDE9FytjkYkh2ddsycxPCba963IQzN");
    
  
   if (!Rtc.IsDateTimeValid())
   {
     // Common Causes:
     //    1) first time you ran and the device wasn't running yet
     //    2) the battery on the device is low or even missing
  
     Serial.println("RTC lost confidence in the DateTime!");
     Rtc.SetDateTime(compiled);
   }
  
   if (Rtc.GetIsWriteProtected())
   {
     Serial.println("RTC was write protected, enabling writing now");
     Rtc.SetIsWriteProtected(false);
   }
  
   if (!Rtc.GetIsRunning())
   {
     Serial.println("RTC was not actively running, starting now");
     Rtc.SetIsRunning(true);
   }
  
   RtcDateTime now = Rtc.GetDateTime();
   if (now < compiled)
   {
     Serial.println("RTC is older than compile time!  (Updating DateTime)");
     Rtc.SetDateTime(compiled);
   }
   else if (now > compiled)
   {
     Serial.println("RTC is newer than compile time. (this is expected)");
   }
   else if (now == compiled)
   {
     Serial.println("RTC is the same as compile time! (not expected but all is fine)");
   }
}


void konekWifi() {
   WiFi.begin(ssid, password);
   //memulai menghubungkan ke wifi router
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print("."); //status saat mengkoneksikan
   }
   Serial.println("Sukses terkoneksi wifi!");
   Serial.println("IP Address:"); //alamat ip lokal
   Serial.println(WiFi.localIP());
}


void loop() {
   for(int i=0;i<10;i++) 
   { 
    buf[i]=analogRead(analogInPin);
   }
   for(int i=0;i<9;i++)
   {
    for(int j=i+1;j<10;j++)
    {
     if(buf[i]>buf[j])
     {
      temp=buf[i];
      buf[i]=buf[j];
      buf[j]=temp;
     }
    }
   }
   currentMillis = millis();
   avgValue=0;
   for(int i=2;i<8;i++)
   avgValue+=buf[i];
   pHVol=(float)avgValue*5.0/1024/6;
   phValue = -3.467   * pHVol + 21.34;
   sensors.requestTemperatures();               
   suhu = sensors.getTempCByIndex(0);
   RtcDateTime now = Rtc.GetDateTime();
   if(recServo=="false"){servo.write(0);}
   senData(10000);
   ReceiveData();
   onOffRelay();
   printDateTime(now);
   if (!now.IsValid())
   {
    // Common Causes:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
   }
}

void printDateTime(const RtcDateTime& dt)
{
   snprintf_P(datestring,
              countof(datestring),
              PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
              dt.Month(),
              dt.Day(),
              dt.Year(),
              dt.Hour(),
              dt.Minute(),
              dt.Second());
   Firebase.setString(firebaseData, "Time", datestring); 
   if(recServo=="true"){
    servo.write(180);
    Serial.println("Servo ON"); 
   }else if(recServo=="false"){
    if((dt.Hour()==pakan1||dt.Hour()==pakan2||dt.Hour()==pakan3||dt.Hour()==pakan4)&&dt.Minute()==menit&&dt.Second()<=5){
      servo.write(180);
      Serial.println("Ikan Sedang Diberi Makan");
    }
   }
}    

void senData(int interval){
  if((unsigned long)(currentMillis - previousMillis) >= interval) {
   Firebase.setInt(firebaseData, "Ph", phValue);
   Firebase.setFloat(firebaseData, "Suhu", suhu);
   previousMillis = millis();
 }
}

void ReceiveData(){
  Firebase.getString(firebaseData, "/Relay");
  recData = firebaseData.stringData();
  Firebase.getString(firebaseData, "/Servo");
  recServo = firebaseData.stringData();
  Firebase.getString(firebaseData, "/Pakan/Konfirmasi/");
  konfirmasi = firebaseData.stringData();
  if(konfirmasi=="true"){
    Firebase.getInt(firebaseData, "/Pakan/Pakan1");
    pakan1 = EEPROM.read(1);
    EEPROM.write(1,firebaseData.intData());
    Firebase.getInt(firebaseData, "/Pakan/Pakan2");
    pakan2 = EEPROM.read(2);
    EEPROM.write(2,firebaseData.intData());
    Firebase.getInt(firebaseData, "/Pakan/Pakan3");
    pakan3 = EEPROM.read(3);
    EEPROM.write(3,firebaseData.intData());
    Firebase.getInt(firebaseData, "/Pakan/Pakan4");
    pakan4 = EEPROM.read(4);
    EEPROM.write(4,firebaseData.intData());
    Firebase.getInt(firebaseData, "/Pakan/Menit");
    menit = EEPROM.read(5);
    EEPROM.write(5,firebaseData.intData());
    }
}

void onOffRelay(){
  if(recData=="true"){                                                         
      Serial.println("Relay ON");                         
      digitalWrite(relayInput, LOW); }
      else{                                                  
        if(phValue<=6.5||phValue>=8.5||suhu<=24||suhu>=30){
         Serial.println("Air Dikuras");
          digitalWrite(relayInput, LOW);
        }else{
          Serial.println("Air Berhenti Dikuras");
          digitalWrite(relayInput, HIGH);
       }                                               
   }      
}
