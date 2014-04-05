

/*

///////////////////////////////////////////////////////////////////////////////////
AutoTalk - easy communication framework for systems involved with automation
version: beta 2

"dashboard" script 
written by: Andrew Frueh, 2011
///////////////////////////////////////////////////////////////////////////////////

For more information about the AutoTalk framework and how you can use it in your project, visit:
  andrewfrueh.com
	
  
  
  
  TODO:
  
  - in AutoTalk_getNodeList(), messages need to be checked for key like they are in incomingDeviceDataHandler()
  	*** the key check should be generalized, a function that returns T/F
  	*** make a processMessage() function so ALL incomming messages are handled the same (example in "_common/generalFunctionTest.html")
  
  - add combo box to select "nodes of type..." (displays only those types)
  	- see setupNodeByTypeSelector()
  	
  - ADD a checkbox for "auto update" - this will send <get/all> to the device on a timer (5 sec)
  - message queue light (alla WorkLife) on each node in the node selector. Server returns state of queue (T/F) with the standard node list (new attribute?)
  
  - a function that converts a full AT message into an object, msgObj['head'] = "get", msgObj['body']['sensor1'] = "456", etc.
  	** be sure to include validation, to set the expected vars to "" - like sender/receiver
  - in nodeSelected(), you need to be checking on node type. If it is "device" then display
  
  	
  
  
*/



// init variables

// the password used for this network, set to "" if no security enabled
window.AutoTalk_netKey = "";



/* ====================================
	
 ==================================== */
$(document).ready(function() {
   startUp();
});



/* ====================================
	
 ==================================== */
function startUp(){
	console.log("startUp()");
	console.log( "    netKey: "+getNetKey() );
	//
	window.inboxTimerRate = 5000;
	window.nodeDataObjects = new Object();
	window.currentSelectedNodeType;
	setCurrentNode(-1);
	
	//
	$('#dashboardNetworkError').hide();
	$('#dashboardForm').hide();
	$('#dashboardNetKey').hide();
	$('#dashboardNetKeyError').hide();
	
	// the netKey field
	$('#netKeyInput').change( function() {
		//alert( this.value );
		setNetKey(this.value);
		AutoTalk_joinNetwork();
	});
	
	//
	$('#messageOutput').keydown(function(event) {
		if (event.keyCode == '13') {
			sendMessageOut();
		}
	})
	
	setupNodeByTypeSelector();
	
	//				
	AutoTalk_joinNetwork();
	// start timer
	inboxAttendant();
	
	
	
}




/* ====================================
	
 ==================================== */
function setupNodeByTypeSelector(){
	console.log("AutoTalk_joinNetwork()");
	
	// 
	$('#selectNodeByType')
		.append("<option>all</option>")
		.append("<option>device</option>")
		.append("<option>dashboard</option>")
		.append("<option>server</option>")
		.change( nodeByTypeWasSelected )
		.attr('selectedIndex', 1)
	;
	
	setSelectedNodeType("device");
	rebuildNodeSelector();
	
}
/* ====================================
	
 ==================================== */
function nodeByTypeWasSelected(event){
	//alert(event.target.selectedIndex + ", " + event.target.value);
	
	if(event.target.value!=""){
		//TODO a filter was chosen, use it
		setSelectedNodeType(event.target.value);
		rebuildNodeSelector();
	}
}


			

/* ====================================
	
 ==================================== */
function AutoTalk_joinNetwork(){
	console.log("AutoTalk_joinNetwork()");
	// 
	// message
	var messageData = "<join/type:dashboard" + netKeyMsgString2() + ">";
						
	var postObj = $.post("server/", { "msg":messageData }, function(returnData){
		logMessageTraffic(returnData);
		// 
		var msgTopArr = AutoTalk_msgParseTop(returnData);
		var msgVarArr = AutoTalk_varsToObj(msgTopArr[1]);
		// save the returned id for later
		if( msgTopArr[0]=="join.r" && msgTopArr[1]!="error:noKey" ){
			// now we can show the dashboard
			view_dashboardDefault();
			//
			setMyDashboardId(msgVarArr["id"]);
			// and then get the list of nodes
			AutoTalk_getNodeList();
		}else{
			//
			view_netKeyLockout();
		}
		
	});
	
	postObj.success(function() { 
		// it worked; 
	})
	postObj.error(function() { 
		// error
		view_networkFailure();
	})
}




/* ====================================
	
 ==================================== */
function inboxAttendant(){
	console.log("inboxAttendant()");
	
	if(getMyId()!=""){
		// rebuild the device selector
		AutoTalk_getNodeList();
		// check my inbox
		var messageData = "<queue//s:" + getMyId() + netKeyMsgString() + ">";
		console.log("    messageData: "+messageData);
							
		$.post("server/", { "msg":messageData }, function(returnData){
			console.log("    inboxAttendant.returnData: "+returnData);
			
			if(returnData!=""){
				processMessageStream(returnData);
			}
			
		});
	}
	// re-start the timer
	var t = setTimeout("inboxAttendant()",window.inboxTimerRate);
	
}




/* ====================================
	
 ==================================== */
function processMessageStream(msgStream){
	console.log("processMessageStream()");
	console.log("    msgStream: "+msgStream);
	
	var msgStreamArr = msgStream.split("><");
	console.log("    msgStreamArr.length: "+msgStreamArr.length);
	if(msgStreamArr.length>1) {
		// there are multiple messages, need to loop
		logMessageTraffic("MessageStream: " + msgStream);
	}else{
		// there is only one message
		logMessageTraffic(msgStream);
		// 
		var msgTopArr = AutoTalk_msgParseTop(msgStream);
		var msgVarArr = AutoTalk_varsToObj(msgTopArr[1]);
		//
		if( msgStream!="" ){
			incomingDeviceDataHandler(msgStream);
		}
	}				
}




/* ====================================
	
 ==================================== */
function incomingDeviceDataHandler(returnData){
	console.log("incomingDeviceDataHandler()");
	var msgArr = AutoTalk_msgParseTop(returnData);
	var msgHead = msgArr[0];
	var msgBody = msgArr[1];
	var msgAddress = Array();
	console.log("    msgBody: "+msgBody);
	if(msgArr.length>2){
		msgAddress = AutoTalk_varsToObj(msgArr[2]);
		console.log("    sender: "+msgAddress['s']);
	}
	var msgKey = "";
	if(msgArr.length>3){
		msgKey = msgArr[3];
		console.log("    key: "+msgKey);
	}
	
	// >> AutoTalk security model
	var proceed;
	if( getNetKey()== "" ){
		//  SECURITY DISABLED
		proceed = true;
	}else{
	    //  SECURITY ENABLED - key is expected
	    if( msgKey == getNetKey() ){
	      proceed = true;
	    }else{
	      proceed = false;
	    }
	}
	// << AutoTalk security model
	
	
	//TODO move this to a generalized message handler
	if(msgHead=="set.r"){
		proceed = false;
	}

	if( msgBody=="error:noKey" ){
		//
		view_netKeyLockout();
	}else{
		if( msgBody!=undefined && proceed ){
			// we can show the dashboard
			view_dashboardDefault();
			var msgBodyObj = AutoTalk_varsToObj(msgBody);
			//
			setDeviceData(msgAddress['s'],msgBodyObj);
			//
			rebuildDeviceGrid();
		}
	}

					
}




/* ====================================
	
 ==================================== */
function logMessageTraffic(msg){
	console.log("logMessageTraffic()");
	console.log("    msg: "+msg);
	if( msg!="" && msg!=undefined ){
		$('#messageLoggerDisplay').append( msg.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;") + "<br/>" );
		$('#messageLoggerDisplay').scrollTo("max");
	}
}			



/* ====================================
	
 ==================================== */
function rebuildDeviceGrid(){
	console.log("rebuildDeviceGrid()");
	// rebuild the table
	$("#deviceGrid").empty();
	$("#deviceGrid").append(
		$("<thead><tr><td>Name</td><td>value</td></tr></thead>")
	);
	var tableBody = $("#deviceGrid").append(
		$("<tbody></tbody>")
	);
	
	
	
	var msgBodyObj = getDeviceData(getCurrentNode());
	console.log("msgBodyObj: "+msgBodyObj);
	if(msgBodyObj!=undefined){
		for(var v in msgBodyObj){
			var varName = v;
			var varValue = msgBodyObj[v];
			//console.log(varName, ":", varValue);
			deviceGrid_addRow(tableBody,varName,varValue);
		}
	}
}
		



/* ====================================
	
 ==================================== */
function deviceGrid_addRow(tableBody,varName,varValue){
	console.log("deviceGrid_addRow()");
	tableBody.append(
		$("<tr></tr>")
			.append( $("<td class='gridCell_name'>"+varName+"</td>") )
			.append( $("<td class='gridCell_value'></td>") 
				.append( 
					$("<input value='"+varValue+"'></input>")
					.attr("id",varName)
					.change(function() {
						if ( this.value != '' ) {
							setCurrentDeviceVar(this.id,this.value);
							rebuildDeviceGrid();
						}
					})
					.mouseover( function() {
						$(this).removeClass("deviceVarMouseOut").addClass('deviceVarMouseOver');
					})
					.mouseout( function() {
						$(this).removeClass("deviceVarMouseOver").addClass('deviceVarMouseOut');
					})
				)
			) // END value cell
	);// END row
}
/*
.keydown(function(event) {
	if (event.keyCode == '13') {
		setCurrentDeviceVar(this.id,this.value);
		rebuildDeviceGrid();
	}
})
*/

/* ====================================
	
 ==================================== */
function getNetKey(){
	return window.AutoTalk_netKey;
}
/* ====================================
	
 ==================================== */
function setNetKey(newKey){
	if( newKey!=undefined ){
		window.AutoTalk_netKey = newKey;
	}
}

/* ====================================
	
 ==================================== */
function netKeyMsgString(){
	if(getNetKey()!=""){
		return "/"+getNetKey();
	}else{
		return "";
	}
}
function netKeyMsgString2(){
	if(getNetKey()!=""){
		return "//"+getNetKey();
	}else{
		return "";
	}
}

		


/* ====================================
	
 ==================================== */
function getServerId(){
	if(window.serverId==undefined){
		setServerId("");
	}
	return window.serverId;
}
/* ====================================
	
 ==================================== */
function setServerId(newId){
	window.serverId = newId;
}



/* ====================================
	
 ==================================== */
function setMyDashboardId(newId){
	//alert("setMyDashboardId(): "+newId);
	window.myId = newId;
}


/* ====================================
	
 ==================================== */
function getMyId(){
	//alert("setMyDashboardId(): "+newId);
	if(window.myId==undefined){
		setMyDashboardId("");
	}
	return window.myId;
}







/* ====================================
	
 ==================================== */
function setCurrentNode(nodeId){
	window.currentSelectedNode = nodeId;
}

/* ====================================
	
 ==================================== */
function getCurrentNode(){
	return window.currentSelectedNode;
}


/* ====================================
	
 ==================================== */
function setNodeList(data){
	window.currentNodeList = data;
}

/* ====================================
	
 ==================================== */
function getNodeList(){
	return window.currentNodeList;
}


/* ====================================
	
 ==================================== */
function setSelectedNodeType(newType){
	window.currentSelectedNodeType = newType;
}

/* ====================================
	
 ==================================== */
function getSelectedNodeType(){
	return window.currentSelectedNodeType;
}









/* ====================================
	
 ==================================== */
function setDeviceData(nodeId,dataObj){
	window.nodeDataObjects[nodeId] = dataObj;
}

/* ====================================
	
 ==================================== */
function getDeviceData(nodeId){
	return window.nodeDataObjects[nodeId];
}


			


/* ====================================
	
 ==================================== */
function setCurrentDeviceVar(varName,varVal){
	sendVarOutToNode(varName,varVal);
	window.nodeDataObjects[getCurrentNode()][varName] = varVal;
}

/* ====================================
	
 ==================================== */
function getCurrentDeviceVar(varName){
	return window.nodeDataObjects[getCurrentNode()][varName];
}




/* ====================================
	
 ==================================== */
function sendVarOutToNode(varName,varVal){
	// message
	var messageData = "<set/" + varName + ":" + varVal + "/s:" + getMyId() + VAR_DELIM_CHAR() + "r:" + getCurrentNode() + netKeyMsgString() + ">";									
	logMessageTraffic(messageData);
	$.post("server/", { "msg":messageData }, function(returnData){
		//alert(returnData);
		logMessageTraffic(returnData);
	});
}
			









		





/* ====================================
	
 ==================================== */
function view_dashboardDefault(){
	$('#dashboardForm').show();
	$('#dashboardNetKey').show();
	$('#dashboardNetKeyError').hide();
}


/* ====================================
	
 ==================================== */
function view_netKeyLockout(){
	$('#dashboardForm').hide();
	$('#dashboardNetKey').show();
	$('#dashboardNetKeyError').show();
}


/* ====================================
	
 ==================================== */
function view_networkFailure(){
	$('#dashboardNetworkError').show();
	$('#dashboardNetKey').hide();
	$('#dashboardNetKeyError').hide();
}


/* ====================================
	
 ==================================== */
function sendMessageOut(){
	//alert("sendMessageOut()");
	
	// message
	var messageData = $('#messageOutput').val();
	clearMessageOut();
	logMessageTraffic(messageData);
	// AJAX							
	$.post("server/", { "msg":messageData }, function(returnData){
		// display the return data
		//$('#messageLoggerDisplay').text(returnData);
		logMessageTraffic(returnData);
	});
}




			
/* ====================================
	
 ==================================== */
function clearMessageOut(){
	//alert("clearMessageOut(): "+$('#messageOutput').val());
	//$('#messageOutput').empty();
	$('#messageOutput').val("");
}









/* ====================================
	
 ==================================== */
function rebuildNodeSelector(){
	console.log("rebuildNodeSelector()");
	$('#deviceSelector').empty();
	
	// populate the device combobox
	var msgBodyArr = getNodeList();
	
	
	for(var v in msgBodyArr){
		var deviceItemText = msgBodyArr[v];
		var deviceItemId = v;
		
		if(deviceItemText=="server"){
			//kludge, where should this go?
			setServerId(deviceItemId);
		}
		
		//if( msgBodyArr[v]!="server" && msgBodyArr[v]!="dashboard" ){
		//$('#deviceSelector').append(
		
		console.log("    getSelectedNodeType():"+getSelectedNodeType());
		console.log("    deviceItemText:"+deviceItemText);
		// filter by type
		if( getSelectedNodeType()=="all" || getSelectedNodeType()==deviceItemText ){
			var ref = $("<div></div>")
		        .attr("class","listItem")
		        .attr("id",v)
		        .html( msgBodyArr[v] + " (id: " + v + ")" )
				.click(function() {
					nodeSelected(this.id);
				})
				.mouseover( function() {
					$(this).removeClass("listItemMouseOut").addClass('listItemMouseOver');
				})
				.mouseout( function() {
					if(getCurrentNode()==this.id){
						$(this).removeClass("listItemMouseOut").addClass('listItemSelected');
					}else{
						$(this).removeClass("listItemMouseOver").addClass('listItemMouseOut');
					}
				})
				.appendTo($('#deviceSelector'))
		    //);
			if(getCurrentNode()==v){
				ref.addClass('listItemSelected');
			}
		}
		
	}
}





/* ====================================
	
 ==================================== */
function nodeSelected(nodeIndex){
	
	// store the selected node for later
	setCurrentNode(nodeIndex);
	rebuildNodeSelector();
	rebuildDeviceGrid();				
	
	// do <get/all> from the selected node and display the vars in a grid
	// as long as the selected device is not 0 (server) or my id
	
	
	//TODO - I should really just be checking node type, if the node is "device" then send <get>
	if( nodeIndex!=getMyId() && nodeIndex!=0 ){
	
	
		// message
		var messageData = "<get/all/s:" + getMyId() + VAR_DELIM_CHAR() + "r:" + nodeIndex + netKeyMsgString() + ">";
		//$('#messageOutput').text( messageData );
		logMessageTraffic(messageData);				
		$.post("server/", { "msg":messageData }, function(returnData){
			// 
			logMessageTraffic(returnData);
			
			//$('#messageLoggerDisplay').text(returnData);
			if(returnData=="<queue.r/true>"){
				// message has been queued, so now wait for data to come back from device
				//TODO ### add functionallity
			}
			
			
			/*
			//alert("returnData: " + returnData);
			// and write that to a text field
			
			var msgArr = AutoTalk_msgParseTop(returnData);
			var msgHead = msgArr[0];
			var msgBody = msgArr[1];
			var msgBodyArr = AutoTalk_varsToObj(msgBody);
			
			// use the object to display the grid
			*/				
		});
	
	}
	
}





/* ====================================
	
 ==================================== */
function AutoTalk_getNodeList(){
	
	console.log("AutoTalk_getNodeList()");
	// message
	var messageData = "<get/nodes/s:" + getMyId() + VAR_DELIM_CHAR() + "r:" + getServerId() + netKeyMsgString() + ">";
	//logMessageTraffic(messageData);
						
	$.post("server/", { "msg":messageData }, function(returnData){
		console.log("AutoTalk_getNodeList.returnData: "+returnData);
		//logMessageTraffic(returnData);
		// should return "success" if received
		
		// and write that to a text field
		var msgArr = AutoTalk_msgParseTop(returnData);
		var msgHead = msgArr[0];
		var msgBody = msgArr[1];
		
		//TODO, needs to do "proceed" check as in other places
		if( msgBody=="error:noKey" ){
			//
			view_netKeyLockout();
		}else{
			if( msgBody!="" ){
				// we can show the dashboard
				view_dashboardDefault();
				var msgBodyArr = AutoTalk_varsToObj(msgBody);
				setNodeList(msgBodyArr);
				rebuildNodeSelector();
			}
		}
							
	});
}






/* ====================================
	
 ==================================== */
function AutoTalk_msgParseTop(msgRaw){
	msgRaw = msgRaw.replace("<","");
	msgRaw = msgRaw.replace(">","");
	var arrTemp = msgRaw.split("/");
	return arrTemp;
}





/* ====================================
	
 ==================================== */
function AutoTalk_varsToObj(varString){
	var varArray = Array();
	if( varString!="" && varString!=undefined ){
		var arrTemp = varString.split(VAR_DELIM_CHAR());// split by ;
		for(var v in arrTemp){
			var varTemp = arrTemp[v].split(":");
			varArray[varTemp[0]] = varTemp[1];
		}
	}
	return varArray;
}


function VAR_DELIM_CHAR(){
	return ",";
}