

/*

 
 ///////////////////////////////////////////////////////////////////////////////////
 AutoTalk - easy communication framework for systems involved with automation
 version: 0.3
 
 "device" script 
 written by: Andrew Frueh, 2014
 ///////////////////////////////////////////////////////////////////////////////////
 
 For more information about the AutoTalk framework and how you can use it in your project, visit:
 andrewfrueh.com/autoTalk
 
 
 This script is an example of a possible device using AutoTalk to communicate.
 It includes functions for receiving/parsing as well as wrapping/sending AutoTalk commands.
 - This version requires an external solution (currently a Processing script) for handling communication with the web.
 - This version does not support communication with other "devices". It assumes it will only be communicating with a web server.
 
 Note: 
 - This code uses the String object -- requires Arduino version 0019+, but was built and tested in 0022.
 
 ================================
 using this script
 
 This script is set up to controll four variables.
 sensor1 & sensor2 - these are meant for reading the analog pins
 led1 & led2 - these are meant for setting the brightness of an led (PWM)  
 
 
 
 */

#define DEBUG_LOGGING true


void debugLog(String msg, int tab=0){
  String tabString = "  ";
#if DEBUG_LOGGING == true
  if(tab==0){
    Serial.println(msg);
  }
  else if(tab>0){
    for(int i=0; i<tab; i++){
      Serial.print(tabString);
    }
    Serial.println(msg);
  }
#endif
}

/* ==========================================================================================
 stuff you DO need to change
 ========================================================================================*/


/* ==============================
 variable definitions
 ============================*/
// here we define our variables
int sensor1, sensor2, led1, led2;



/* ==============================
 This stuff is for the AutoTalk framework
 ============================*/

// the password used for this network, set to "" if no security enabled
String AutoTalk_netKey = "";



// YOU NEED TO CHANGE ALL THREE ARRAYS BELOW
//  TODO - this could be a class. each object has a property for val, name, type

// here we define references for AutoTalk control (binding)
int *AutoTalk_varVals[] = {
  &sensor1, &sensor2, &led1, &led2
};

// here we define names
String AutoTalk_varNames[] = {
  "sensor1", "sensor2", "led1", "led2"
};

// here we define types
String AutoTalk_varTypes[] = {
  "apin", "apin", "dpin", "dpin"
};




/* ==============================
 define the pins on your Arduino board
 ============================*/
const int sensorPin1 = 0;
const int sensorPin2 = 1;
const int ledPin1 = 10;
const int ledPin2 = 11;



/* ====================================
 you can handle your variables here
 ====================================*/
void updateMyState(){

  // get the vals from my sensors
  sensor1 = analogRead(sensorPin1);
  sensor2 = analogRead(sensorPin2);

  analogWrite(ledPin1, led1);
  analogWrite(ledPin2, led2);

}






/* ==========================================================================================
 stuff you DON'T need to change
 ========================================================================================*/

// the limitation of EEPROM is that you are limited to BYTE i.e. 0-255 for each address
#include <EEPROM.h>
// the address of EEPROM memory to use for my id on the AutoTalk network
#define EEADDRESS_MYID 0 
//#define EEADDRESS_SERVERID 1


// =================================
// you do not need to change these 

// the ID of this specific AutoTalk device
int AutoTalk_deviceId;
// the array size, DO NOT CHANGE THIS
#define AutoTalk_varArrLength (sizeof(AutoTalk_varVals)/sizeof(int *)) 
#define VAR_DELIM_CHAR ","


// ====================================
// timers
// ====================================

//
unsigned long timerCurrentTime;
//
unsigned long timerMSGFast_last;
const int timerMSGFast_rate = 500; 
//
unsigned long timerMSGSlow_last;
const int timerMSGSlow_rate = 5000; 
//
unsigned long timerState_last;
const int timerState_rate = 100; 





// ====================================
// messenging
// ====================================
// >> messenging
//const int timerMSGFast_rate = 100; // clock speed of the messaging system
//const int timerMSGSlow_rate = 5000; // clock speed of the messaging system
// for message parsing
boolean MSGactive = false; 
String messageBuffer;
//boolean tryToJoinNetwork = false;
boolean queuingIsActive = false;
// << messenging




//String mySavedMsg;
//FLASH_STRING(mySavedMsg,"");


// ====================================
// Ethernet
// ====================================

#include <SPI.h>
#include <Ethernet.h>

EthernetClient client;

boolean serverMessageActive = false; 
String serverMessageBuffer;

boolean hostCommunicationActive = false;
boolean hostCommunicationVerified = false; // to make sure you have actually connected to the server

byte mac[] = {  
  0x90, 0xA2, 0xDA, 0x00, 0x27, 0x18 };

char hostAddress[] = "andrewfrueh.com";

String URLRequestString1 = "GET /autoTalk/dashboard/server/?msg=";
String URLRequestString2 = " HTTP/1.1";

String serverResponseData;

boolean ethernetInitialized = false;


/* ==========================================================================================
 required Arduino functions
 ========================================================================================*/

// ====================================
// standard Arduino setup() 
// ====================================
void setup(){

  Serial.begin(9600);
  // time to boot up
  delay(1000);

}


// ====================================
// standard Arduino loop() 
// ====================================
void loop(){

  if( !ethernetInitialized ){
    initEthernet();
  }


  // for all timers
  timerCurrentTime = millis();

  // -------------------------------------------
  // >> update state for sensors, etc.
  // -------------------------------------------

  if ( abs(timerCurrentTime - timerState_last) >= timerState_rate) {
    timerState_last = timerCurrentTime; 
    // 
    updateMyState();
  }

  // -------------------------------------------
  // << update state for sensors, etc.
  // -------------------------------------------




  // -------------------------------------------
  // >> message system timer
  // -------------------------------------------

  if ( abs(timerCurrentTime - timerMSGFast_last) >= timerMSGFast_rate) {
    timerMSGFast_last = timerCurrentTime; 
    // check for messages
    //autoTalk_messageIn();  
    if( ethernetInitialized){
      if( hostCommunicationActive ){
        autoTalk_messageIn();
      } 
      else { 
        sendDataToServer("<join>");
      }
    }
    // 
  }

  if ( abs(timerCurrentTime - timerMSGSlow_last) >= timerMSGSlow_rate) {
    timerMSGSlow_last = timerCurrentTime;    
    //
    // get my queued messages, out only, response is just message stream
    if(queuingIsActive){
      AutoTalk_getQueuedMessages();
    }
  }

  // -------------------------------------------
  // << message system timer
  // -------------------------------------------


}





/* ==========================================================================================
 general functionallity
 ========================================================================================*/


// ====================================
// 
// ====================================
void logThis(String msg){
  Serial.println(msg);
}






// ====================================
// convert from String to int, so the incoming message can be used in math
// ====================================
// taken from -- http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1285085509
// probably needs improvement
int stringToInt(String text){
  char temp[20];
  text.toCharArray(temp, 19);
  int x = atoi(temp);
  if (x == 0 && text != "0") {
    x = -1;
  }
  return x;
}







/* ==========================================================================================
 Ethernet functionallity
 ========================================================================================*/

// ====================================
// close the Ethernet connection
// ====================================  
void serverCloseConnection(){
  debugLog("serverCloseConnection()");
  client.stop();
  client.flush();
  hostCommunicationActive = false;
  serverMessageBuffer = "";
  serverMessageActive = false;        
}


// ====================================
// close the Ethernet connection
// ====================================  
void sendDataToServer(String msgOut){
  debugLog("sendDataToServer()");

  // if you get a connection, report back via serial:
  if (client.connect(hostAddress, 80)) {
    hostCommunicationActive = true;
    hostCommunicationVerified = true; // is never set to false
    debugLog("connected, sending data...",1);
    //String AutoTalk_msg_out;
    //AutoTalk_msg_out = "<join>";
    // Make a HTTP request:
    client.print(URLRequestString1); 
    client.print(msgOut); 
    client.println(URLRequestString2); 

    client.print("Host: ");
    client.println(hostAddress);
    client.println("Connection: close");
    client.println();

  } 
  else {
    hostCommunicationActive = false;
    // if you didn't get a connection to the hostAddress:
    debugLog("connection failed",1);
  }

}


// ====================================
// initialize Ethernet connection
// ====================================  
void initEthernet(){
  debugLog("initEthernet()");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) { // WARNING: call to Ethernet.begin() can lock up the board for a long time
    ethernetInitialized = false;
    debugLog("connection failed",1);
  }
  else{
    debugLog("connection success",1);
    ethernetInitialized = true;
    // give the Ethernet shield a second to initialize:
    delay(1000);
  }
}


/* ==========================================================================================
 AutoTalk functionallity
 ========================================================================================*/


// ====================================
// incoming serial message, read from the buffer looking for < and >
// ====================================
void autoTalk_messageIn(){
  debugLog("autoTalk_messageIn()");
  //int lenTemp = Serial.available();
  int lenTemp = client.available();
  debugLog(String(lenTemp),1);

  if (lenTemp == 0){
    // no response, kill the connection
    serverCloseConnection();        
    debugLog("killing the connection",1);
  }
  else  if (lenTemp > 0) {
    char incomingByte;
    for(int i=0; i<lenTemp; i++){
      // read the incoming byte:
      //incomingByte = Serial.read();
      incomingByte = client.read();
      Serial.print(incomingByte);

      switch(incomingByte){
      case '<': // open tag
        //debugLog("open tag",1);
        //
        MSGactive = true;
        messageBuffer = "";
        break;
      case '>': // close tag
        //debugLog("close tag",1);
        //
        if(MSGactive){
          debugLog("WHERE IS processMessage???",1);
          autoTalk_processMessage(messageBuffer);
          messageBuffer = "";
          MSGactive = false;
          // close communication
          serverCloseConnection();
        }
        break;
      default: // tag contents?
        if(MSGactive){
          messageBuffer += incomingByte;
        }
      }
    }
  }
}





// ====================================
// message has been received, now process it
// ====================================  
// AutoTalk_netKey
void autoTalk_processMessage(String messageIn){
  
  debugLog("autoTalk_processMessage()");
  debugLog(messageIn, 1);

  String messageTemp = messageIn;
  String messageHead = "";
  String messageBody = "";
  String messageAddress = "";
  String messageKey = "";
  //
  int p1=-1, p2=-1, p3=-1;
  p1 = messageTemp.indexOf('/'); 
  if(p1!=-1){
    p2 = messageTemp.indexOf('/', p1+1);
  } 
  if(p2!=-1){
    p3 = messageTemp.indexOf('/', p2+1); 
  }
  String tmp = "p1:";
  tmp += p1;
  tmp += ", p2:";
  tmp += p2;
  tmp += ", p3:";
  tmp += p3;
  debugLog(tmp, 1);


  // message head
  messageHead = messageTemp.substring(0, p1);
  // message body
  messageBody = messageTemp.substring(p1+1,p2);
  // message address (if exist)
  if(p2!=-1){
    messageAddress = messageTemp.substring(p2+1,p3);
  }

  debugLog("===>", 1);
  debugLog(messageHead, 1);
  debugLog(messageBody, 1);
  debugLog("<===", 1);


  // >> parse the address
  String sender = "";
  int offset = 0;
  //int p1, p2;
  String chunk, varName, varVal;
  p1 = messageAddress.indexOf(VAR_DELIM_CHAR); 
  chunk = messageAddress.substring(offset, p1);
  p2 = chunk.indexOf(':'); 
  varName = chunk.substring(0, p2);
  varVal = chunk.substring(p2+1);
  if(varName=="s"){
    sender = varVal;
  }
  chunk = messageAddress.substring(p1+1);
  p2 = chunk.indexOf(':'); 
  varName = chunk.substring(0, p2);
  varVal = chunk.substring(p2+1);
  if(varName=="s"){
    sender = varVal;
  }
  // <<



  // key
  if(p3!=-1){
    messageKey = messageTemp.substring(p3+1);
  }

  // >> AutoTalk security model
  boolean proceed;
  if(AutoTalk_netKey==""){
    debugLog("  SECURITY DISABLED proceeding");
    //  SECURITY DISABLED
    proceed = true;
  }
  else{
    //  SECURITY ENABLED - key is expected
    debugLog("  SECURITY ENABLED");
    if( messageKey == AutoTalk_netKey ){
      proceed = true;
    }
    else{
      proceed = false;
    }
  }
  // << AutoTalk security model

  // special case, NO SECURITY CHECK
  if(messageHead=="greet"){
    // greeting between AutoTalk devices (handshake)
    autoTalk_greetHandler(proceed);
  }
  else if( !proceed ){
    //
    autoTalk_msg("security error, you are not authorized");
  }
  else if(messageHead==""){
    //
    autoTalk_msg("message error, no head");
  }
  else{
    //Serial.println(messageHead);
    // route by message type
    if(messageHead=="get"){
      autoTalk_getHandler(messageBody,sender);
    }
    else if(messageHead=="set"){
      autoTalk_setHandler(messageBody,sender);
    }
    else if(messageHead=="msg"){
      // ?
      /*
      }else if(messageHead=="greet"){
       // greeting between AutoTalk devices (handshake)
       autoTalk_greetHandler(messageBody);
       */
    }
    else if(messageHead=="join.r"){
      // greeting between AutoTalk devices (handshake)
      autoTalk_joinHandler(messageBody);
    }
    else if(messageHead=="_saveid"){
      autoTalk_setMyId(messageBody);
    }
    else if(messageHead=="_getid"){
      Serial.print(autoTalk_getMyId());
    }
  }
}




/* ====================================
 
 ==================================== */
void autoTalk_setMyId(String newId){
  EEPROM.write(EEADDRESS_MYID, byte(stringToInt(newId)));
}

/* ====================================
 
 ==================================== */
int autoTalk_getMyId(){
  int val = EEPROM.read(EEADDRESS_MYID);
  return val;
}






/* ====================================
 
 int AutoTalk_setServerId(String newId){
 EEPROM.write(EEADDRESS_SERVERID, byte(stringToInt(newId)));
 }
 ==================================== */

/* ====================================
 
 int AutoTalk_getServerId(){
 int val = EEPROM.read(EEADDRESS_SERVERID);
 return val;
 }
 ==================================== */




String netKeyMsgString(){
  String messageOut = "";
  if(AutoTalk_netKey!=""){
    messageOut += "/";
    messageOut += AutoTalk_netKey;
  }
  return messageOut;
}


/* ====================================
 
 ==================================== */
void AutoTalk_joinNetwork(){
  debugLog("AutoTalk_joinNetwork()");
  //
  String messageOut = "<join/type:device";
  messageOut += VAR_DELIM_CHAR;
  messageOut += "id:";
  messageOut += autoTalk_getMyId();
  //
  if(netKeyMsgString()!=""){
    messageOut += "/";
    messageOut += netKeyMsgString();
  }
  messageOut += ">";
  Serial.print(messageOut);
}

/* ====================================
 
 ==================================== */
void autoTalk_joinHandler(String msgBody){
  debugLog("autoTalk_joinHandler()");
  //
  int p = msgBody.indexOf(':', 0);
  if(p!=-1){
    autoTalk_setMyId( msgBody.substring(p+1) );
    queuingIsActive = true;
    //tryToJoinNetwork = false;
  }
}





// ====================================
// respond to greeting, let the other device know you are here
// ====================================  
void autoTalk_greetHandler(boolean proceed){
  debugLog("autoTalk_greetHandler()");
  //if( msgBody=="" ){
  Serial.print("<greet.r>");
  //tryToJoinNetwork = true;
  delay(2000);
  AutoTalk_joinNetwork();
  //}
}





/* ====================================
 
 ==================================== */
void AutoTalk_getQueuedMessages(){
  debugLog("AutoTalk_getQueuedMessages()");
  String messageOut = "<queue//s:";
  messageOut += autoTalk_getMyId();
  messageOut += netKeyMsgString();
  messageOut += ">";
  Serial.print(messageOut);
}







/*
// - THIS CAUSES BUGGY BEHAVIOR - UNRELIABLE
 String AutoTalk_returnAllVars(String type){
 Serial.println("AutoTalk_returnAllVars()");
 String output = "";
 for(int i=0; i<AutoTalk_varArrLength; i++){
 Serial.println(i);
 if( type=="" || type==AutoTalk_varTypes[i] ){
 output += AutoTalk_varNames[i];
 output += ":";
 output += *AutoTalk_varVals[i];
 output += ";";
 }
 }
 if( output.endsWith(';') ){ output = output.substring(0, output.length()-1); }
 Serial.println(output);
 return output;
 }
 */



/* ====================================
 
 ==================================== */




// ====================================
// respond to request for data
// ====================================  
void autoTalk_getHandler(String messageBody, String sender){
  debugLog("autoTalk_getHandler()");
  //
  //Serial.print("reportAllLevels: ");



  // initialize the message string
  String messageOut = "";

  // >> create the message
  messageOut += "<get.r/";

  //if(p==-1){
  //if(messageBody.substring(0,p)=="all"){
  if( messageBody.startsWith("all") ){
    //
    int p = messageBody.indexOf(':', 0);
    if(p==-1){
      // there was no val passed via 'all'
      //messageOut += AutoTalk_returnAllVars("");
      for(int i=0; i<AutoTalk_varArrLength; i++){
        messageOut += AutoTalk_varNames[i];
        messageOut += ":";
        messageOut += *AutoTalk_varVals[i];
        messageOut += VAR_DELIM_CHAR;
      }
    }
    else{
      // a val was passed via 'all'
      //messageOut += AutoTalk_returnAllVars(messageBody.substring(p+1));
      for(int i=0; i<AutoTalk_varArrLength; i++){
        if( messageBody.substring(p+1)==AutoTalk_varTypes[i] ){
          messageOut += AutoTalk_varNames[i];
          messageOut += ":";
          messageOut += *AutoTalk_varVals[i];
          messageOut += VAR_DELIM_CHAR;
        }
      }
    }
  }
  else{
    // get the specific var names
    //
    boolean loopActive = true; 
    int p;
    //int splitIndexChunk;
    int splitIndexOffset = 0;
    //String currentChunk;  
    //int varValNum;
    // loop through the variable pair items in the message body
    while(loopActive==true){
      // >> PARSING
      // get the next variable pair
      p = messageBody.indexOf(VAR_DELIM_CHAR, splitIndexOffset);
      //messageOut += "p";
      //messageOut += p;
      if(p==-1){
        loopActive=false;
      } // this is the last one
      //varName = messageBody.substring(splitIndexOffset, splitIndex);
      // << PARSING

      // loop over the name array to find our var
      for(int i=0; i<AutoTalk_varArrLength; i++){
        /*
        messageOut += "~";
         messageOut += messageBody.substring(splitIndexOffset, p);
         messageOut += "`";
         messageOut += AutoTalk_varNames[i];
         */
        if( messageBody.substring(splitIndexOffset, p) == AutoTalk_varNames[i]){
          // get the variable via pointer
          messageOut += AutoTalk_varNames[i];
          messageOut += ":";
          messageOut += *AutoTalk_varVals[i];
          messageOut += VAR_DELIM_CHAR;
        }
      }

      //
      splitIndexOffset = p+1;

    } // END while()
  }// END if(all)

  // last char is ';' chop it off
  if( messageOut.endsWith(VAR_DELIM_CHAR) ){
    messageOut = messageOut.substring( 0, messageOut.length()-1 );
  } 

  if(sender!=""){
    messageOut += "/s:";
    messageOut += autoTalk_getMyId();
    messageOut += VAR_DELIM_CHAR;
    messageOut += "r:";
    messageOut += sender;
  }

  //
  messageOut += netKeyMsgString();
  messageOut += ">"; 
  // << create the message

  // send the message out
  Serial.print(messageOut);

}



// ====================================
// use incoming data to set variables
// ====================================  
void autoTalk_setHandler(String messageBody, String sender){
  debugLog("autoTalk_setHandler()");
  debugLog(messageBody, 1);
  debugLog(sender, 1);
  //
  boolean loopActive = true; 
  int splitIndex;
  int splitIndexChunk;
  int splitIndexOffset = 0;
  String currentChunk, varName, varValue;  
  int varValNum;
  // loop through the variable pair items in the message body
  while(loopActive==true){

    // >> PARSING
    // get the next variable pair
    splitIndex = messageBody.indexOf(VAR_DELIM_CHAR, splitIndexOffset);
    if(splitIndex==-1){
      loopActive=false;
    } // this is the last one
    currentChunk = messageBody.substring(splitIndexOffset, splitIndex);
    splitIndexOffset = splitIndex+1;  
    // split the pair to get the variable name and value
    splitIndexChunk = currentChunk.indexOf(':');
    varName = currentChunk.substring(0, splitIndexChunk);
    varValue = currentChunk.substring(splitIndexChunk+1);
    // << PARSING

    varValNum = stringToInt(varValue);
    // loop over the name array to find our var
    for(int i=0; i<AutoTalk_varArrLength; i++){
      if( varName == AutoTalk_varNames[i]){
        // set the variable via pointer
        *AutoTalk_varVals[i] = varValNum;
      }
    }

  } // END while()

  // initialize the message string
  String messageOut = "";
  // >> create the message
  messageOut += "<set.r/true";

  if(sender!=""){
    messageOut += "/s:";
    messageOut += autoTalk_getMyId();
    messageOut += VAR_DELIM_CHAR;
    messageOut += "r:";
    messageOut += sender;
  }


  messageOut += netKeyMsgString();
  messageOut += ">";
  //
  Serial.print(messageOut);
}






// ====================================
// send a "msg" type message, used for errors, etc.
// ====================================  
void autoTalk_msg(String stringData){
  //Serial.print("reportAllLevels: ");

  // initialize the message string
  String messageOut = "";

  // >> create the message
  messageOut += "<msg/";
  //
  messageOut += stringData;
  //
  messageOut += ">"; 
  // << create the message

  // send the message out
  Serial.print(messageOut);

}











/* ====================================
 // send a "msg" type message, used for errors, etc.
 String* autoTalk_varParser(String varStr){
 String returnVals[] = {"a", "b"};
 
 return returnVals;
 
 }
 ==================================== */













