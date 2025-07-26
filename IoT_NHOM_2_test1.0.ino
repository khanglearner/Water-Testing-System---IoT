/*
21130049 - Dương Minh Khang (NT)
22130219 - Vũ Thiên Vinh
22130078 - Trần Duy Khang
22130199 - Nguyễn Ngô Thùy Trinh
*/


#include <SimpleTBDevice.h>
#include <ESP8266WiFi.h>
#include "DHTesp.h"  
#include <TM1637Display.h>          
DHTesp dht;    

// Tên và mật khẩu wifi.
#define SSID "SV_DPEE" //"Wifinha 2.4G"//
#define PASSWORD "svvldt38300595"//"341@972K564H#L13" ////"khtn@phonghoc"

// Địa chỉ máy chủ thingsboard
#define SERVER_ADDR "demo.thingsboard.io" //ví dụ example.com:8080
// Access token của device
#define ACCESS_TOKEN "GLuuQbrBUAiwdlgobrqw"  

//================================
#define CLK D3
#define DIO D4

#define RELAY1 D5
#define RELAY2 D6
#define RELAY3 D7

#define TRIG_PIN D1
#define ECHO_PIN D2
#define TIME_OUT 5000
//===============================
#define TdsSensorPin A0
#define VREF 3.3            
#define SCOUNT 30 
TM1637Display display(CLK, DIO);  // Initialize the TM1637 display

//Khai báo biến
int duration, distanceCm;

int analogBuffer[SCOUNT];     
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 29;      
unsigned long TimeMark, TimeMark1;


TBDevice device(ACCESS_TOKEN);


//========================================================
void setup() {
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  pinMode(TdsSensorPin,INPUT);

  Serial.begin(115200);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Bắt đầu kết nối wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Đã kết nối wifi.");
  
  WiFiClient client;
  device.begin(SERVER_ADDR, client);
  
  TimeMark = millis();
  display.setBrightness(7); // Set display brightness (0-7)
  
}
//========================================================

void loop(){
  Serial.println("Begin");
//===================ĐẶT TÊN BIẾN========================
  int thr_distance = device.read("thr1").asInt();
  int thr_tds  = device.read("ref_tdsValue").asInt();
  bool motor_1 = device.read("motor1").asBoolean();
  bool motor_2 = device.read("motor2").asBoolean();
  bool motor_3 = device.read("motor3").asBoolean();
  bool m = device.read("mode").asBoolean();

  TimeMark1 = millis();

//==========================MODE AUTO=================================
  if(m == true){
    Serial.println("MODE Auto");         
  // action_1: Bơm nước vào
  while(distanceCm > thr_distance){
    Ultra_Sensor_Value();
    motor(1);
    }
    motor(0);
  }
  //action_2: Kiểm định nước và phân loại nước
    delay(1000);
    TDS_Sensor_Value();  
    delay(1000);

  if(tdsValue > thr_tds){       //xả nước bẩn
    while(distanceCm < device.read("max").asInt()){
      Ultra_Sensor_Value();
      motor(2);
    }
      motor(0);
  }
  else {
    while(distanceCm < device.read("max").asInt()){
        Ultra_Sensor_Value();
        motor(3);
        }
    motor(0);
        }


//=====================MODE MANUAL============================
  else{
    Serial.println("mode manual");
    TDS_Sensor_Value();
    Ultra_Sensor_Value();

  if(millis() - TimeMark1 >= 200){  
      Serial.println("Hi!");     
  if(motor_1 == true) {                     //Bơm nước
    digitalWrite(RELAY1, LOW);
    Ultra_Sensor_Value();
    if(distanceCm < 12) {digitalWrite(RELAY1, HIGH); } //Tự giới hạn ngưỡng bơm để tránh tràn nước.
  }
  else {
    digitalWrite(RELAY1, HIGH);
    }
  
  if(motor_2) {digitalWrite(RELAY2, LOW);}
  else {
    digitalWrite(RELAY2, HIGH);
    }

  if(motor_3) {digitalWrite(RELAY3, HIGH);}
  else {
    digitalWrite(RELAY3, LOW);
  }
    TimeMark1 = millis(); // Cập nhật thời gian.
    }
  }
}



//=======================================================================
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

//======================HÀM ĐỌC GIÁ TRỊ CẢM BIẾN TDS===============================================
void TDS_Sensor_Value(){
  int s = millis();
  while(millis() - s <= 1000){
     static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0;
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      float compensationVoltage=averageVoltage/compensationCoefficient;
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      
      Serial.println("TDS Value:");
      Serial.println(tdsValue, 0);
      
      }
    }
  }
  if(millis() - TimeMark >= 1000){
    device.write("TDS", tdsValue);
    display.showNumberDec((int)tdsValue); // Hiển thị giá trị TDS (ppm) làm tròn
    TimeMark = millis();
  }
}

//==================HÀM ĐỌC GIÁ TRỊ CẢM BIẾN SIÊU THANH===========================================
void Ultra_Sensor_Value(){        
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN,LOW);
  duration = pulseIn(ECHO_PIN, HIGH, TIME_OUT);
  distanceCm = 0.034*(duration/2);
  device.write("distance", distanceCm);
}
//=============================================================

void motor(int a){
  switch(a) {
  case 0: 
    digitalWrite(RELAY1, HIGH);  //OFF
    digitalWrite(RELAY2, HIGH);  //OFF
    digitalWrite(RELAY3, LOW);   //OFF
    break;

  case 1:
    digitalWrite(RELAY1, LOW);   //ON
    digitalWrite(RELAY2, HIGH);  //OFF
    digitalWrite(RELAY3, LOW);   //OFF
    break;

  case 2:
    digitalWrite(RELAY1, HIGH);  //OFF
    digitalWrite(RELAY2, LOW);   //ON
    digitalWrite(RELAY3, LOW);  //OFF
    break;

  case 3:
    digitalWrite(RELAY1, HIGH);  //OFF
    digitalWrite(RELAY2, HIGH);  //OFF
    digitalWrite(RELAY3, HIGH);  //ON
    break;
  }
  time(10);
}

void time(int t){
  int b = millis();
  while(millis() - b <= t){}
}