/*

///////////////////////////////////////////////////////////////////////////////////
AutoTalk - easy communication framework for systems involved with automation
version: beta 2

Processing-passthrough script 
written by: Andrew Frueh, 2010
///////////////////////////////////////////////////////////////////////////////////

For more information about the AutoTalk framework and how you can use it in your project, visit:
  andrewfrueh.com/autoTalk


This script simply passes the data between Arduino and the web server. There is no parsing done here.

This script could easily be done away with if the "device" (Arduino) could comm. directly with the web server.
 
*/


// ### THE URL WHERE YOUR DASHBOARD IS INSTALLED ###

String MY_URL = "http://andrewfrueh.com/autoTalk/test/";





/* ====================================================================================

                 YOU SHOULD NOT HAVE TO CHANGE ANYTHING BELOW THIS

 ====================================================================================*/



//String MY_URL = "http://andrewfrueh.com/autoTalk/test/";
//String MY_URL = "http://127.0.0.1/AutoTalk_web";


// this is the index of which of your available serial ports the Arduino is connected to
//int serialPortToUse = 2;

// to enable serial comunication
import processing.serial.*;
Serial autoTalkDevicePort;
Boolean serialPortIsActive = false;
Boolean deviceIsConnected = false;


long timerCurrentTime, timerMSGlast;

// >> messenging
int MSGtimerRate = 5000; // clock speed of the messaging system
boolean MSGactive = false;
//char messageBuffer[128];
String messageBuffer;
String messageHead;
String messageBody;
// << messenging

//XMLElement xmlOjbect;
String webServerCommURL = MY_URL + "/server/?msg=";
//
String AutoTalk_netKey = "ratso";



// ==============================  standard Processing stuff  ===================================

// ====================================
// standard Processing setup() 
// ====================================
void setup(){
  // the port is hard-coded, you must determine which port your Arduino uses
  //autoTalkDevicePort = new Serial(this, Serial.list()[serialPortToUse], 9600);
  // don't generate a serialEvent() unless you get a newline character:
  //autoTalkDevicePort.bufferUntil('>');
  logThis("setup()");
  logThis("  MY_URL: "+MY_URL);
}



// ====================================
// standard Processing draw() 
// ====================================
void draw(){
  
    // auto-detect serial port 
    if(!serialPortIsActive){
      setupAutoTalkDevice();
    }
    // END auto-detect serial port   
  
     // deviceIsConnected = true;

  
    // for all timers
    timerCurrentTime = millis();

    // -------------------------------------------
    // >> message system timer
    // -------------------------------------------
      
    if ( abs(timerCurrentTime - timerMSGlast) >= MSGtimerRate) {
      timerMSGlast = timerCurrentTime;
      //
      //tryGreetAgain();
      /*
      if(!deviceIsConnected){
        tryGreetAgain();
      }
      */
    }
      
    // -------------------------------------------
    // << message system timer
    // -------------------------------------------
    
}






// ==============================  general functionallity  ===================================



// ====================================
// simple logger
// ====================================
void logThis(String msg){
  println(msg);
}



// ====================================
// this function catches a serial event when the Arduino board responds
// ====================================
void serialEvent(Serial p) { 
  logThis("serialEvent()");
  // get the ASCII string:
  // String inString = autoTalkDevicePort.readStringUntil('\n');  
  String inString = p.readStringUntil('>');
  
  if (inString != null) {
    // trim off any whitespace:
    //inString = trim(inString);
    //autoTalk_processDeviceMessage(inString);
    logThis(""+inString);
    
    autoTalk_passToServer(inString);
    /*
    if(inString == "<greet.r>"){
      deviceIsConnected = true;
    }else{
       autoTalk_passToServer(inString);
    }
    */
  }
}





// ==============================  AutoTalk functionallity  ===================================


void tryGreetAgain(){
  autoTalkDevicePort.write("<greet>");
}
   
   
// ====================================
// 
// ====================================
void setupAutoTalkDevice(){
  String portNameTemp = findAutoTalkDeviceOnPort();
  if(portNameTemp==""){
    // so something here
  }else{
    logThis("activating serial connection on port "+portNameTemp);
    serialPortIsActive = true;
    //autoTalkDevicePort = new Serial(this, portNameTemp, 9600);
    //autoTalkDevicePort.bufferUntil('>'); // use the close bracket char for buffer
   //autoTalkDevicePort.write("<greet/>");
  }
}

String netKeyMsgString(){
  String messageOut = "";
  if(AutoTalk_netKey!=""){
    messageOut += "/";
    messageOut += AutoTalk_netKey;
  }
  return messageOut;
}



// ====================================
// auto-detect AutoTalk board on serial ports
// ====================================
String findAutoTalkDeviceOnPort(){
  //
  logThis("findAutoTalkDeviceOnPort()");
  //
  String returnVal = "";
  Boolean portFound = false;
  String liveTestSerialPort = "";
  //
  for(int i=0; i < Serial.list().length;i++){
    //
    liveTestSerialPort = Serial.list()[i];
    logThis("trying port "+liveTestSerialPort);
    
    try{
      //Serial arduinoPort = new Serial(this, liveTestSerialPort, 9600);
      autoTalkDevicePort = new Serial(this, liveTestSerialPort, 9600);
      
      autoTalkDevicePort.bufferUntil('\n'); // trick the serialEvent() so it does not fire (AutoTalk will not use '\n')
      delay(2000);
      String msgTemp = "<greet";
      if(netKeyMsgString()!=""){
        msgTemp += "//";
        msgTemp += netKeyMsgString();
      }
      msgTemp += ">";
      autoTalkDevicePort.write(msgTemp);
      //arduinoPort.write("<greet/request:"+i+">");
      //arduinoPort.clear();
      delay(2000);
      
      if(autoTalkDevicePort.available()>0){
        String inString = autoTalkDevicePort.readStringUntil('>'); 
        //logThis("inString: "+inString);
        if( inString.equals("<greet.r>") ){ // apparently you must use string.equals()
          logThis("AutoTalk device found on port "+liveTestSerialPort);
          returnVal = liveTestSerialPort;
          portFound = true;
          autoTalkDevicePort.bufferUntil('>'); // use the close bracket char for buffer
          //break;
        }
      }
     
      //
      if(portFound){
        break;
      }else{
        autoTalkDevicePort.stop();
      }
      
    }catch(NullPointerException e) {
      logThis("exception");
    }
    //delay(5000);
  }
   
  return returnVal;	
}



// ====================================
// pass data from the device up to the server
// ====================================
void autoTalk_passToServer(String message){
  logThis("autoTalk_passToServer()");

  if(message.charAt(0)=='<' && message.charAt(message.length()-1)=='>'){
    
    logThis("  message: "+message);
    String fullURL = webServerCommURL + message;
    //logThis("  fullURL: "+fullURL);
    // make request from the server
    String responseData[] = loadStrings(fullURL);
    //logThis("  responseData.length: "+responseData.length);
    if( responseData!=null && responseData.length>0 && responseData[0]!="" ){
      logThis("  response: "+responseData[0]);
      // the response
      
      autoTalk_passToDevice(responseData[0]);
    }
  }else{
    
    logThis("ERROR - not AutoTalk");
    logThis("  message: "+message);
    
  }
}



// ====================================
// pass data from server down to device
// ====================================
void autoTalk_passToDevice(String message){
  logThis("autoTalk_passToDevice()");
  logThis("    message:"+message);
  logThis("    message.length:"+message.length());
  if( message!="" && message!=null && message.length()>0 ){
    if(message.charAt(0)=='<' && message.charAt(message.length()-1)=='>'){
      //logThis("\n  message: "+message);
      // pass message to Arduino 
      autoTalkDevicePort.write(message); 
    } 
  }
}




