<?php

/*
///////////////////////////////////////////////////////////////////////////////////
AutoTalk - easy communication framework for systems involved with automation
version: beta 2

"server" script
written by: Andrew Frueh, 2011
///////////////////////////////////////////////////////////////////////////////////

For more information about the AutoTalk framework and how you can use it in your project, visit:
  andrewfrueh.com

--------------------------------------


########
TODO


 
 
The Framework:

 - if address is not specified then assumed:
 	a. that the message is intended for server
 	b. you are just connecting two devices together, so there is no need for id
 - if device in a network recieves <join>, then that device becomes a proxy for the newly connected device
 	-> this means that in response to <join>, the device sends a message (<newDevice> maybe?) to the server - which replies with a new device id, that the proxy device gives back to the new device via <join.r>
 	
 	
 	
Steven says:
 - messages should be able to be sent to everyone
 - attribute for whether a message is queued (so meesages intende ONLY for devices that are currently connected)
 	
*/





//
$GLOBALS['log'] = "";
$GLOBALS['logType'] = "file";// can be "echo", "file", or "none"
$GLOBALS['deviceData'] = "";

//
$GLOBALS['AutoTalk_netKey'] = "";

// file names
define("DATA_PATH", "data/");
define("DEVICE_DATA_FILE", DATA_PATH."nodeList.xml");
define("MESSAGE_FILE_PREFIX", DATA_PATH."msgBuffer_");
define("MESSAGE_FILE_SUFFIX", ".txt");

define("NETWORK_NODE_NAME", "node");
	
define("VAR_DELIM_CHAR", ",");


StartUp();



//
function StartUp(){

	logThis("StartUp()");
	
	// init variables
	$GLOBALS['deviceData'] = loadDeviceSettings();
	$msgHead = "";
	$msgBody = "";
	$msgAddress = array ( "s"=>"","r"=>"" );
	$msgKey = "";
	
	// get AutoTalk message
	$msg = "";
	// 'msg'
	// if there is POST data use that, else use GET data	
	if( array_key_exists('msg', $_POST) ){
		$msg = $_POST['msg'];
	}else if( array_key_exists('msg', $_GET) ) {
		$msg = $_GET['msg'];
	}
	logThis("    msg: ".$msg);
	
	$msgTemp = trim($msg, "<>");
	$msgArr = explode("/", $msgTemp);
	$msgHead = $msgArr[0];
	
	// validation
	if(count($msgArr)>1){
		$msgBody = $msgArr[1];
	} 
	if(count($msgArr)>2){
		$msgAddress = AutoTalk_varsToObj($msgArr[2]);
		if( !array_key_exists('r', $msgAddress) ){
			$msgAddress["r"] = "";
		}
		if( !array_key_exists('s', $msgAddress) ){
			$msgAddress["s"] = "";
		}
	} 
	if(count($msgArr)>3){
		$msgKey = $msgArr[3];
	}
	
	// >> AutoTalk security model
	$proceed;
	if( getNetKey()== "" ){
		//  SECURITY DISABLED
		$proceed = true;
	}else{
	    //  SECURITY ENABLED - key is expected
	    if( $msgKey == getNetKey() ){
	      $proceed = true;
	    }else{
	      $proceed = false;
	    }
	}
	// << AutoTalk security model
	
	
	/*
	logThis("    msgHead: ".$msgHead);
	logThis("    msgBody: ".$msgBody);
	logThis("    msgAddress: ".$msgAddress);
	*/
	
	
	if($proceed){
		//
		if( $msgAddress["r"]!="" && $msgAddress["r"]!="0" && $msgAddress["r"]!="undefined"  ){
		//if( array_key_exists('r', $msgAddress) ){
			// has recipient, buffer message
			logThis("    saveMessage");
			// route the message to the appropriate inbox
			AutoTalk_saveMessage( $msgAddress["r"], $msg );
			AutoTalk_messageResponse("<queue.r/true" . netKeyMsgString2() . ">");
		}else{
			// no recipient, so message is intended for me
			if( $msgHead == "join" ){
				logThis("    join");
				// message intended for the AutoTalk server
				if( $msgBody != "" ){
					$msgBodyVars = AutoTalk_varsToObj($msgBody);
					// message intended for the AutoTalk server
					AutoTalk_joinHandler($msgBodyVars);
				}
			}else if( $msgHead == "get" ){
				logThis("    get");
				$msgBodyArr = explode(":",$msgBody);
				if( $msgBodyArr[0] == "nodes" ){
					$deviceType = "";
					if(count($msgBodyArr)>1){
						// return devices
						$deviceType = $msgBodyArr[1];
					}
					AutoTalk_returnDeviceList($deviceType,$msgAddress);
				}else{
					// save message to inbox
					AutoTalk_saveMessage( $msgAddress["r"], $msg );
				}
				
			}else if( $msgHead == "queue" ){
				logThis("    queue");
				// ## currently, "type" does not matter
				AutoTalk_loadMessages($msgAddress["s"]);
				/*
				if( $msgBody == "" ){
					// only recent
					AutoTalk_loadMessages($msgAddress["s"],"new");
				}else if( $msgBody == "all" ){
					// all messages
					AutoTalk_loadMessages($msgAddress["s"],"all");
				}
				*/
			}
		}
	}else{ // END if(proceed)
		AutoTalk_error($msgHead,"noKey");
	}
	
	//
	logOut();
	
}


/* ============================================
	   
============================================ */	
function AutoTalk_error($head,$errorType){
	echo "<" . $head . ".r/error:" . $errorType . ">";
}




/* ------------------------------------------------------------------------
                            GENERAL FUNCTIONALITY
------------------------------------------------------------------------ */



/* ============================================
	    
============================================ */	
function logThis($msg){
	$GLOBALS['log'] .= $msg . "\n";
}



/* ============================================
	    
============================================ */	
function logOut(){
	
	$GLOBALS['log'] = "==========================\n" . $GLOBALS['log'] . "\n\n";
	
	if( $GLOBALS['logType']!="none" ){
		if($GLOBALS['logType']=="echo"){
			echo $GLOBALS['log'];
		}
		if($GLOBALS['logType']=="file"){
			file_put_contents("LOG.txt", $GLOBALS['log'], FILE_APPEND);
		}
	}
}



/* ============================================
	   
============================================ */	
function saveDeviceSettings($xmlObj){
	// validate
	checkDataDir();
	file_put_contents(DEVICE_DATA_FILE, $xmlObj->asXml());
}



/* ============================================
	   
============================================ */	
function checkDataDir(){
	if(!is_dir(DATA_PATH)) { mkdir(DATA_PATH);}
}







/* ------------------------------------------------------------------------
                            AUTOTALK FUNCTIONALITY
------------------------------------------------------------------------ */


function getNetKey(){
	return $GLOBALS['AutoTalk_netKey'];
}


function netKeyMsgString(){
  $messageOut = "";
  if(getNetKey()!=""){
    $messageOut .= "/";
    $messageOut .= getNetKey();
  }
  return $messageOut;
}

function netKeyMsgString2(){
  $messageOut = "";
  if(getNetKey()!=""){
    $messageOut .= "//";
    $messageOut .= getNetKey();
  }
  return $messageOut;
}


/* ============================================
	   
============================================ */	
function AutoTalk_varsToObj($varString){
	$varArray = Array();
	$arrTemp = explode(VAR_DELIM_CHAR, $varString);// split by ;
	foreach($arrTemp as $v){
		$varTemp = explode(":", $v);
		//logThis($k.":".$v);
		if( array_key_exists(1, $varTemp) ){
			$varArray[$varTemp[0]] = $varTemp[1];
		}
	}
	return $varArray;
}




/* ============================================
	   
============================================ */	
function loadDeviceSettings(){
	//
	logThis("loadDeviceSettings()");
	//
	if( file_exists(DEVICE_DATA_FILE) ){
		logThis("    loading file");
		// load data
		//file_get_contents(DEVICE_DATA_FILE);
		$xmlObj = simplexml_load_file(DEVICE_DATA_FILE);
		logThis("    xmlObj: ".$xmlObj);
		//logThis("    DEVICE_DATA_FILE: ".file_get_contents(DEVICE_DATA_FILE) );
	}else{
		logThis("    creating new file");
		// create default obj
		$xmlObj = new SimpleXMLElement("<".NETWORK_NODE_NAME."s/>");
		$node = $xmlObj->addChild(NETWORK_NODE_NAME);
		$node->addAttribute("id","0");
		$node->addAttribute("type","server");
		// and create the file
		saveDeviceSettings($xmlObj);
	}
	//logThis("    success: ".$success);
	return $xmlObj;
}





/* ============================================
	   
============================================ */	
function AutoTalk_joinHandler($msgBodyVars){
	//
	$msgOut = "<join.r/id:" . createNewNetId($msgBodyVars) . netKeyMsgString2() . ">";	
	//
	AutoTalk_messageResponse($msgOut);
}


				




/* ============================================
	   
============================================ */	
function createNewNetId($vars){
	logThis("createNewNetId()");
	$xmlObj = $GLOBALS['deviceData'];
	logThis("    xmlObj: ".$xmlObj);
	//
	if( array_key_exists('id', $vars) ){
		$inputId = $vars["id"];
	} else {
		$inputId = "";
	}
	if( array_key_exists('type', $vars) ){
		$inputType = $vars["type"];
	} else {
		$inputType = "";
	}
	logThis("    inputId: ".$inputId);
	logThis("    inputType: ".$inputType);
	
	//$highestDeviceId = -1;
	$newDeviceId = -1;
	$nodeExistsAlready = false;
	
	//if( $xmlObj->count() == 0){ // need PHP 5.3
	if( count($xmlObj->children()) == 0){ // only needs PHP 5.1
		logThis("    making a new 0 node");
		// this is the first device
		$newDeviceId = "0";
	}else{
		logThis("    looping through nodes");
		// there is at least one other device
		foreach ($xmlObj as $xmlNode) {
		//for($i=0; $i<$xmlObj->count(); $i++){
			logThis("    xmlNode[id]: ".$xmlNode["id"]);
			if( $inputType==$xmlNode["type"] ){
				if( $inputId==$xmlNode["id"] || $inputType=="dashboard" ){
					logThis("      ** use this id");
					// if the ids match OR kludge if I am the dashboard, this is my node
					$newDeviceId = $xmlNode["id"];
					$nodeExistsAlready = true;
					break;
				}
			}
			//
			$thisId = $xmlNode["id"]; // ### ???
			//logThis("    thisId: ".$thisId);
			$newDeviceId = max($thisId+1,$newDeviceId);
			//$newDeviceId = $highestDeviceId;
		}
	}
	
	if(!$nodeExistsAlready){
		$node = $xmlObj->addChild(NETWORK_NODE_NAME);
		$node->addAttribute("id",$newDeviceId);
		$node->addAttribute("type",$inputType);
		//logThis("    newDeviceId: ".$newDeviceId);
		saveDeviceSettings($xmlObj);
	}
	logThis("      return: " . $newDeviceId);
	return $newDeviceId;
	
}




/* ============================================
	   
============================================ */	
function AutoTalk_returnDeviceList($deviceType,$msgAddress){
	logThis("AutoTalk_returnDeviceList()");
	//
	$xmlObj = loadDeviceSettings();
	//
	$msgOut = "<get.r/";
	foreach ($xmlObj as $xmlNode) {
		$id = $xmlNode["id"];
		$type = $xmlNode["type"];
		if( $deviceType=="" || $deviceType==$type ){
			$msgOut .= $id.":".$type.VAR_DELIM_CHAR;
		}
	}
	$msgOut = rtrim($msgOut,VAR_DELIM_CHAR);
	
	// address
	$msgOut .= addressMsgString($msgAddress['s'], $msgAddress['r']);
	// netKey
	$msgOut .= netKeyMsgString();
	// close
	$msgOut .= ">";
	
	AutoTalk_messageResponse($msgOut);
}




/* ============================================
	   
============================================ */	
function addressMsgString($s,$r){
	$msgOut = "";
	if( $s!="" && $r!="" ){
		$msgOut .= "/s:" . $r . VAR_DELIM_CHAR . "r:" . $s;
	}else{
		$msgOut .= "/";
	}
	return $msgOut;
}






/* ============================================
	   
============================================ */	
function AutoTalk_saveMessage( $id, $msg ){
	logThis("AutoTalk_saveMessage()");
	if( $id!="" && $msg!="" ){
		logThis("    msg:".$msg);
		checkDataDir();
		//
		$fileData = "";
		if(file_exists(MESSAGE_FILE_PREFIX.$id.MESSAGE_FILE_SUFFIX)){
			$fileData = file_get_contents( MESSAGE_FILE_PREFIX.$id.MESSAGE_FILE_SUFFIX );
		}
		
		// encode the message
		$msg = encodeMessage($msg);
		//
		$proceed = true;
		// split by '\n'
		$fileDataLines = explode("\n", $fileData);
		// check each line
		foreach( $fileDataLines as $line ){
			if( $line == $msg ){
				$proceed = false;
			}
		}
		
		if( $proceed ){
			$fileNameTemp = MESSAGE_FILE_PREFIX.$id.MESSAGE_FILE_SUFFIX;
			file_put_contents( $fileNameTemp, $msg."\n", FILE_APPEND );
		}
	}
}





/* ============================================
	   
============================================ */	
function AutoTalk_loadMessages( $id ){
	logThis("AutoTalk_loadMessages(): ");
	//
	checkDataDir();
	//
	$fileNameTemp = MESSAGE_FILE_PREFIX.$id.MESSAGE_FILE_SUFFIX;
	logThis("    fileNameTemp: ".$fileNameTemp);
	if(file_exists($fileNameTemp)){
		// load the queue data
		$fileData = file_get_contents( $fileNameTemp );
		// empty the buffer file
		file_put_contents( $fileNameTemp, "" );  
		// split by '\n'
		$fileDataLines = explode("\n", $fileData);
		$msgReturn = "";
		// check each line
		foreach( $fileDataLines as $line ){
			if( $line != "" ){
				// decode, and append to the message stream
				$msgReturn .= decodeMessage($line);
			}
		}
		logThis("    msgReturn: ".$msgReturn);
		// return the queue
		AutoTalk_messageResponse($msgReturn);
	}
	
}



/* ============================================
	   
============================================ */	
function AutoTalk_msg($msg){
	echo "<msg/".$msg.">";
}



/* ============================================
	   
============================================ */	
function AutoTalk_messageResponse($msg){
	echo $msg;
}



/* ============================================

============================================ */	
function encodeMessage( $msg ){
	if(getNetKey()!=""){
		$converter = new Encryption(getNetKey());
		$msg = $converter->encode($msg);
	}
	return $msg;
}


/* ============================================
	   
============================================ */	
function decodeMessage( $msg ){
	if(getNetKey()!=""){
		$converter = new Encryption(getNetKey());
		$msg = $converter->decode($msg);
	}
	return $msg;
}



// ===========================================================================
// taken from:
// http://stackoverflow.com/questions/1289061/best-way-to-use-php-to-encrypt-and-decrypt

class Encryption {
    //var $skey = "yourSecretKey"; // you can change it

    var $skey = "";
    
    function __construct($skey) {
    }
    
    public  function safe_b64encode($string) {
        $data = base64_encode($string);
        $data = str_replace(array('+','/','='),array('-','_',''),$data);
        return $data;
    }

    public function safe_b64decode($string) {
        $data = str_replace(array('-','_'),array('+','/'),$string);
        $mod4 = strlen($data) % 4;
        if ($mod4) {
            $data .= substr('====', $mod4);
        }
        return base64_decode($data);
    }

    public  function encode($value){ 
        if(!$value){return false;}
        $text = $value;
        $iv_size = mcrypt_get_iv_size(MCRYPT_RIJNDAEL_256, MCRYPT_MODE_ECB);
        $iv = mcrypt_create_iv($iv_size, MCRYPT_RAND);
        $crypttext = mcrypt_encrypt(MCRYPT_RIJNDAEL_256, $this->skey, $text, MCRYPT_MODE_ECB, $iv);
        return trim($this->safe_b64encode($crypttext)); 
    }

    public function decode($value){
        if(!$value){return false;}
        $crypttext = $this->safe_b64decode($value); 
        $iv_size = mcrypt_get_iv_size(MCRYPT_RIJNDAEL_256, MCRYPT_MODE_ECB);
        $iv = mcrypt_create_iv($iv_size, MCRYPT_RAND);
        $decrypttext = mcrypt_decrypt(MCRYPT_RIJNDAEL_256, $this->skey, $crypttext, MCRYPT_MODE_ECB, $iv);
        return trim($decrypttext);
    }
}





?>