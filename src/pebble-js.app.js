// Food Trucks

var MAX_MENU_ITEMS = 100;
var FIRST_DETAIL = 1000;
var activeTrucks;
var activeTruckDetails;
var index;
var now;
var offset;
var saveCity = "boston";
var unitTest = true;

function log(msg) {
	//console.log(msg);
}

function addOpenTruck(name, location, endtime, details) {
	var len = activeTrucks.length;
	if (len >= MAX_MENU_ITEMS)
		return;
	activeTrucks[len] = 
		{
		'index' : index,
		'title' : name,
		'subtitle' : location
		};
	activeTruckDetails[len] = 
		{
		'endtime' : endtime - offset,
		'details' : details
		};
	index++;
}

function sendToPebble(which) {
	var msg;
	var activeTruckData = activeTrucks;
	var i = which;
	if (which >= FIRST_DETAIL) {
		activeTruckData = activeTruckDetails;
		i = which - FIRST_DETAIL;
	}
	msg =  (i < activeTrucks.length) ? activeTruckData[i] : { 'count': i };
	log("sending " + which + " of " + activeTrucks.length + ": " + JSON.stringify(msg));
	Pebble.sendAppMessage(msg);
}

function requestSceduleFor(city) {
	log ("scheduleRequest " + city);
	// where the real lookup will go
}

function testRequestSceduleFor(city) {
	log ("testRequestSceduleFor " + city);
	for (var i = 0; i < 30; i++) {
		addOpenTruck (city + " " + i, "location " + i, now + i * 600, "Details " + i + "\nLine 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6\nLine 7\nLine 8\nLine 9\nLine 10\nLine 11");
	}
	sendToPebble(activeTrucks.length);
}

function lookupCity(city) {
	log ("lookupCity " + city);
	now = Math.floor((new Date()).getTime() / 1000);
	offset = (new Date()).getTimezoneOffset() * 60;
	activeTrucks = [];
	activeTruckDetails = [];
	index = 0;
	if (unitTest) {
		testRequestSceduleFor(city);
	} else {
		requestSceduleFor(city);
	}
}

Pebble.addEventListener("ready", function() {
	lookupCity(saveCity);
});

// Set callback for appmessage events
Pebble.addEventListener("appmessage", function(e) {
	log("message =" + JSON.stringify(e.payload));
	sendToPebble(e.payload.index);
});
