/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "application.h"
#line 1 "c:/Users/ryan.trisnojoyo/Documents/Particle/projects/Cricket/src/Cricket.ino"
/*  Changelog
1.1.1- Change pin type from INPUT_PULLDOWN to INPUT to try and block invalid trips
1.1.2- Added timesync with each status message


*/
void setup();
void responseHandler(const char *event, const char *data);
void parseString(String strVars);
bool setParameter(String param, String value);
void statusMessage();
void responseMessage();
void debugMessage(String message);
void loop();
void initSyncTime();
bool initConnection();
void disconnectConnection();
void calculateUltraSonicData();
int calculateFloatData();
#line 7 "c:/Users/ryan.trisnojoyo/Documents/Particle/projects/Cricket/src/Cricket.ino"
String Version = "1.1.2";


int led1 = D0; // Instead of writing D0 over and over again, we'll write led1
// You'll need to wire an LED to this one to see it blink.

int led2 = D7; // Instead of writing D7 over and over again, we'll write led2
// This one is the little blue LED on your board. On the Photon it is next to D7, and on the Core it is next to the USB jack.

const int buttonPin = D4;

int sendInterval = 4;        // number of samples collected to transmit
int  logInterval = 2;        // Set the log interval in minutes 
int statusInterval = 1440;   //  Status message interval in minutes (default to 1 day)
int transmitMode = 0 ;       // 0- send logged data at regular send interval, 1= send at logInterval if tripCount > 0
long int lastStatusMessage = 0;
long int prevTime;
long int currentTime;
long int sleepTime;
      
int samplesLogged = 0;
String payload;
String totalPayload;
bool booting = true;
pin_t wakeUpPins[1] = {D4};
int sendAttempts = 1;   // counter to indicate number of send attempt failures 
int timeout = 10000;   // length of time to wait for cellular connection (milliseconds)
bool debug = false;

int pinRelay = A0;
int pinMassaSensor = A1;
int pinFloatSensor= D13;

int massaValue = 0;
int floatValue = 0;

PRODUCT_ID(10618);
PRODUCT_VERSION(1);


SYSTEM_MODE (MANUAL);

void setup()
{ 
  Serial.begin(9600);
  pinMode(pinRelay,OUTPUT);
  pinMode(pinMassaSensor, INPUT);
  pinMode(pinFloatSensor, INPUT_PULLUP);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(pinRelay,LOW); 

  initSyncTime();
  prevTime = Time.now();         
  //Particle.subscribe(System.deviceID() + "/hook-response/Status", responseHandler, MY_DEVICES);
  Serial.println("Starting");
}
void responseHandler(const char *event, const char *data) {
  parseString(data);
   responseMessage();
}
       
void parseString(String strVars)
{

  strVars = strVars.replace("\"","");
  String parameter = ""; 
  String value = "";
  String inChar = "";
  int i = 0;
  bool readingParam = true;

  while (inChar != ".")
  {
  inChar = strVars.charAt(i++);
  if (inChar != ":" and inChar != ",")
    {
    if (readingParam)
      parameter += inChar;
    else
      value += inChar;    
    }
  else
    {
      if (inChar == ":")
      {
        readingParam = false;
      }
      else
      {
        if (setParameter(parameter, value) == false)
          return;
        parameter = "";
        value = ""; 
        readingParam = true;
      }
    }
  }

  setParameter(parameter,value.replace(".",""));
}
bool setParameter(String param, String value)
{
 
if (param == "si")
  {
      Serial.println("Setting sendInterval to: " + value);
      sendInterval = value.toInt();
  }
  else if (param == "li")
  {
      Serial.println("Setting logInterval to: " + value);
      logInterval = value.toInt();
  }
  else if (param == "sm")
  {
      Serial.println("Setting statusInterval to: " + value);
      statusInterval = value.toInt();
  }
  else if (param == "tm")
  {
      Serial.println("Setting transmitMode to: " + value);
      transmitMode = value.toInt();
  }
  else if (param == "to")
  {
      Serial.println("Setting timeout to: " + value);
      timeout = value.toInt();
      if (timeout < 10000)  // Ensure that an invalid timeout parameter does not disable radio... minimum timeout is 10 seconds
        timeout = 10000;
  }
  else if (param == "db")
  {
      Serial.println("Setting debug to: " + value);
      if(value == "true")
        debug = true;
  }
  else
  {
      Serial.println("Unknown parameter- " + param + ":" + value);
  }
  
  return true;
}

/****  Every version of code should contain a daily status message  ****/
void statusMessage()
{
  if (initConnection())
  {
        Particle.syncTime();
    delay(2000);
    CellularSignal sig = Cellular.RSSI();
    int rssi = sig.rssi;
    Serial.println("RSSI: " + (String)rssi);

    String message =  String(Time.now()) + ",Status,";
    message += "li:" + (String)logInterval + ",si:" + (String)sendInterval + ",sm:" + (String)statusInterval + ",tm:" + (String)transmitMode;
    message += ",to:" + (String)timeout + ",ver:" + Version ;
    message +=  ", RSSI: " + (String)rssi;
    Particle.publish("Status", message, PRIVATE);

    disconnectConnection();
  }
  lastStatusMessage = Time.now() + 60;
}

/****  validate response to status by sending back new parameters  ****/
void responseMessage()
{
  if (initConnection())
  {
    String message =  String(Time.now()) + ",Reply,";
    message += "li:" + (String)logInterval + ",si:" + (String)sendInterval + ",sm:" + (String)statusInterval + ",tm:" + (String)transmitMode;
    message += ",to:" + (String)timeout;
    Particle.publish("Counter", message, PRIVATE);
    disconnectConnection();
  }
}

void debugMessage(String message)
{
  if (debug) {
    if (initConnection())
    {

      Particle.publish("Counter", message, PRIVATE);
      disconnectConnection();
    }
  }
}

void loop()
{
  currentTime = Time.now();
  String wakeupFrom = "RTC";
  SleepResult result = System.sleepResult();
  boolean sendingStatus = false;

  booting = false;

  /******* Check if status message is due  *****/
  if (lastStatusMessage + (60 * statusInterval) < Time.now() )
  {
    statusMessage();
    sendingStatus = true;
  }

  /*********   Take sample *************/
  if (currentTime - prevTime >= (logInterval * 60)) 
  {
    calculateUltraSonicData();
    payload += "," + (String)massaValue;
    samplesLogged++;
    prevTime = currentTime;

    if (((samplesLogged >= (sendInterval * sendAttempts)) || (transmitMode == 1)) && !sendingStatus)
    {
      if (initConnection())
      {
        totalPayload = String(Time.now()-(60*logInterval*(samplesLogged-1)));
        totalPayload += ",1043,";
        totalPayload += String(logInterval);
        totalPayload += payload;
        Particle.publish("Counter", totalPayload, PRIVATE);
        Serial.println("totalPayload: " + (String)totalPayload);
        payload = "";
        samplesLogged = 0;
        disconnectConnection();
      }
      else
      {
        sendAttempts++;
      }
    }    
  }

  /********  Good night!  ************/
      
     if (wakeupFrom == "PIN")
     {       // Back to sleep for remainder of normal log interval
      int sleepRemainder = (logInterval * 60) - (Time.now() - sleepTime);
      debugMessage("Sleep from Pin Wake: " + (String)sleepRemainder);
      System.sleep(wakeUpPins,1,FALLING, sleepRemainder);
     }
     
    else
    {
      sleepTime = Time.now();
      debugMessage("Normal sleep");
      System.sleep(wakeUpPins,1,FALLING,logInterval * 60);
    }
    
}
void initSyncTime()
{
  initConnection();
  Particle.syncTime();
}
bool initConnection()
{
  bool retVal = false;
  Cellular.on();
  Cellular.connect();
  waitFor(Cellular.ready,timeout);
  Particle.connect();
  waitFor(Particle.connected,timeout);
  if (Cellular.ready())
    retVal = true;
  return retVal;
}
void disconnectConnection()
{
  Particle.disconnect();
  waitUntil(Particle.disconnected);
  Cellular.off();
}

void calculateUltraSonicData()
{
  digitalWrite(pinRelay,HIGH);
  delay(5000); 
  massaValue = analogRead(pinMassaSensor);
  delay(1000);
  digitalWrite(pinRelay,LOW); 
}

int calculateFloatData(){
  floatValue = digitalRead(pinFloatSensor);
  return floatValue;
}