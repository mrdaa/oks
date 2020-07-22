#include <WiFi.h>
#include <DHT.h>
#include <SocketIoClient.h>


#define DHTTYPE DHT11
#define dhtPin 39
#define relay1 D3
#define trigPin 32
#define echoPin 33
#define TdsSensorPin 36
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point

String id = "1471984882";

int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,temperature = 25;

long duration ;
float distance;
const int max_hight = 14;

char SocketServer[] = "192.168.43.47";
int port = 4000;

char ssid[] = "B";
char pass[] = "punyaucup11299";

SocketIoClient socket;
DHT dht(dhtPin, DHTTYPE);


void konek(const char * payload, size_t length) {
  socket.emit("new user", "\"P1471984882\"");
    Serial.println("berhasil connect");
}

void setup()
{
    Serial.begin(115200);
    pinMode(TdsSensorPin,INPUT);
    pinMode(trigPin,OUTPUT);
    pinMode(echoPin, INPUT);

    Serial.begin(115200);
    dht.begin();
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED){
      delay(500);
  }

  socket.begin(SocketServer, port);
  socket.on("connect",konek);
  // socket.emit("new user", "\"P1471984882\"");
  //  Blynk.begin(auth,ssid,pass);

  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  
}

void loop(){

  socket.loop();  
//  socket.on("rhum",test);

  float t = dht.readTemperature();
  char temp[10];
  String tempAsString;
    // perform conversion
  dtostrf(t,1,2,temp);
    // create string object
  tempAsString = String(temp);
  String inputTemp;
  inputTemp = t;
  String dataTemp = "{\"_id\":\"" + id + "\",\"_val\":\""+ tempAsString +"\"}";
  socket.emit("temp", dataTemp.c_str());
  
  // Kirim data humidity 
  float h = dht.readHumidity();
  char hum[10];
  String humAsString;
    // perform conversion
  dtostrf(h,1,2,hum);
    // create string object
  humAsString = String(hum);
  String inputHum;
  inputHum = h;
  String dataHum = "{\"_id\":\"" + id + "\",\"_val\":\"" + humAsString + "\"}";
  socket.emit("hum", dataHum.c_str());
//  socket.on("rtemp",event);

  // sensor ultrasound ketinggian air
  digitalWrite(trigPin, LOW); // Added this line
  delayMicroseconds(2);       // Added this line
  digitalWrite(trigPin, HIGH);
  //  delayMicroseconds(1000); - Removed this line
  delayMicroseconds(10); // Added this line
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;
  distance = max_hight - distance;
  distance = (distance / max_hight) * 100;

  char wl[10];
  String wlAsString;
    // perform conversion
  dtostrf(distance,1,2,wl);
    // create string object
  wlAsString = String(wl);
  String inputWl;
  inputWl = distance;
  String dataWl = "{\"_id\":\"" + id + "\",\"_val\":\"" + inputWl + "\"}";
  socket.emit("wl", dataWl.c_str());
    
   static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");

      char tds[10];
      String tdsAsString;
        // perform conversion
      dtostrf(tdsValue,1,2,tds);
        // create string object
      tdsAsString = String(tds);
      String inputTds;
      inputTds = tdsValue;
      String dataTds = "{\"_id\":\"" + id + "\",\"_val\":\"" + inputTds + "\"}";
      socket.emit("tds", dataTds.c_str());
          
   }
}
int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
      for (i = 0; i < iFilterLen - j - 1; i++) 
          {
        if (bTab[i] > bTab[i + 1]) 
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}
