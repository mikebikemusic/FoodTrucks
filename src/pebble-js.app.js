// Food Trucks

var activeTrucks;
var index;
var saveCity = "boston";
var unitTest = true;

function log(msg) {
	console.log(msg);
}

function addOpenTruck(name, location) {
	activeTrucks[activeTrucks.length] = 
		{
		'index': index,
		'title' : name,
		'subtitle' : location
		};
	index++;
}

function sendToPebble(which) {
	var msg;
	msg =  (which < activeTrucks.length) ? activeTrucks[which] : { 'count': which };
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
		addOpenTruck (city + " " + i, "location " + i);
	}
	sendToPebble(activeTrucks.length);
}

function lookupCity(city) {
	log ("lookupCity " + city);
	activeTrucks = [];
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
