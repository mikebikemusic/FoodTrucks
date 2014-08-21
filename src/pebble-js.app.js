// Food Trucks

var MAX_MENU_ITEMS = 100;
var FIRST_DETAIL = 1000;
var activeTrucks;
var activeTruckDetails;
var index;
var now;
var offset;
var version = "foodtrucks.v1";
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
		'details' : details.substring(0, 2000), // Limit to what we can send
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

function scanJSON(json) {
	var theTrucks = json.vendors;
	for (var key1 in theTrucks) {
		var theTruck = theTrucks[key1];
		if (theTruck.open.length > 0) {
			var opens = theTruck.open;
			for (var key2 in opens) {
				var open = opens[key2];
				if (open.start < now && open.end > now) {
					var details = "";
					if (theTruck.phone !== null)
						details += theTruck.phone + "\n";
					if (theTruck.description_short !== null)
						details += theTruck.description_short + "\n";
					if (theTruck.email !== null)
						details += theTruck.email + "\n";
					if (theTruck.description !== null)
						details += theTruck.description + "\n";
					if (theTruck.url !== null)
						details += theTruck.url + "\n";
					if (theTruck.twitter !== null)
						details += "@" + theTruck.twitter;
					addOpenTruck(theTruck.name, open.display, open.end, details);
				}
			}
		}
	}
	sendToPebble(activeTrucks.length);
}

function requestSceduleFor(city) {
	log ("scheduleRequest " + city);
	var request = new XMLHttpRequest();
	request.open('GET', "http://data.streetfoodapp.com/1.1/schedule/" + city + "/", true);
	request.setRequestHeader('User-Agent', 'PEBBLEMIKE');
	request.onreadystatechange = function(){
		log(request.readyState + ", " + request.status);
		if(request.readyState == 4 && request.status == 200){
			scanJSON(JSON.parse(request.responseText));
		}
	};
	request.send();
}

function testRequestSceduleFor(city) {
	log ("testRequestSceduleFor " + city);
	var i;
	switch (city) {
		case "boston": // typical count
			for (i = 0; i < 30; i++) {
				addOpenTruck (city + " " + i, "location " + i, now + i * 600, "Details " + i + 
					"\nLine 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6\nLine 7\nLine 8\nLine 9\nLine 10\nLine 11");
			}
			break;
		case "calgary": // count = 0
			break;
		case "edmonton": // count = 1 and long details
			addOpenTruck (city, 
				"location ", now + 600, "Details " + 
					"\nLine 1 is not so long." + 
					"\nLine 2 is much longer than one before it but not as long as the one to follow." + 
					"\nLine 3 is much longer than one before it but not as long as the one to follow because it repeats itself once: Line 3 is much longer than one before it but not as long as the one to follow because it repeats itself." + 
					"\nLine 4 is much longer than one before it but not as long as the one to follow because it repeats itself twice: Line 4 is much longer than one before it but not as long as the one to follow because it repeats itself once more: Line 4 is much longer than one before it but not as long as the one to follow because it repeats itself." +  
					"\nLine 5 is slightly smaller than the one before it because it repeats itself twice: This line is slightly smaller than the one before it because it repeats itself once more: This line is slightly smaller than the one before it because it repeats itself." +
					"\nLine 6 is about the same as the one before it because it repeats itself twice: This line is about the same as the one before it because it repeats itself once more: This line is about the same as the one before it because it repeats itself." + 
					"\nLine 7 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 8 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					//"\nLine 9 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					//"\nLine 10 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 11 is different than the one before it because it is the last one.");
			break;
		case "halifax": // count = MAX_MENU_ITEMS and long truck name
			for (i = 0; i < MAX_MENU_ITEMS; i++) {
				addOpenTruck (city + " "  + i + " with a very long truck name that needs to be truncated", 
					"location " + i, now + i * 600, "Details " + i + 
					"\nLine 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6\nLine 7\nLine 8\nLine 9\nLine 10\nLine 11");
			}
			break;
		case "ottawa": // small count and long location
			for (i = 0; i < 3 + 1; i++) {
				addOpenTruck (city + " " + i, 
					"location " + i + " is so long you might never find this truck because you will get so lost and hungry you will die of starvation before you get there.", 
					now + i * 600, "Details " + i + 
					"\nLine 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6\nLine 7\nLine 8\nLine 9\nLine 10\nLine 11");
			}
			break;
		case "tallahassee": // count above MAX_MENU_ITEMS
			for (i = 0; i < MAX_MENU_ITEMS + 1; i++) {
				addOpenTruck (city + " " + i, 
					"location " + i, now + i * 600, "Details " + i + 
					"\nLine 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6\nLine 7\nLine 8\nLine 9\nLine 10\nLine 11");
			}
			break;
		case "toronto": // count way above MAX_MENU_ITEMS
			for (i = 0; i < MAX_MENU_ITEMS + 50 + 1; i++) {
				addOpenTruck (city + " " + i, 
					"location " + i, now + i * 600, "Details " + i + 
					"\nLine 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6\nLine 7\nLine 8\nLine 9\nLine 10\nLine 11");
			}
			break;
		case "vancouver": // count = 3 and longer details
			addOpenTruck (city + " 1", 
				"location 1", now + 600, "Details " + 
					"\nLine 1 is not so long." + 
					"\nLine 2 is much longer than one before it but not as long as the one to follow." + 
					"\nLine 3 is much longer than one before it but not as long as the one to follow because it repeats itself once: Line 3 is much longer than one before it but not as long as the one to follow because it repeats itself." + 
					"\nLine 4 is much longer than one before it but not as long as the one to follow because it repeats itself twice: Line 4 is much longer than one before it but not as long as the one to follow because it repeats itself once more: Line 4 is much longer than one before it but not as long as the one to follow because it repeats itself." +  
					"\nLine 5 is slightly smaller than the one before it because it repeats itself twice: This line is slightly smaller than the one before it because it repeats itself once more: This line is slightly smaller than the one before it because it repeats itself." +
					"\nLine 6 is about the same as the one before it because it repeats itself twice: This line is about the same as the one before it because it repeats itself once more: This line is about the same as the one before it because it repeats itself." + 
					"\nLine 7 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 8 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					//"\nLine 9 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					//"\nLine 10 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 11 is different than the one before it because it is the last one.");
			addOpenTruck (city + " 2", 
				"location 2", now + 600, "Details " + 
					"\nLine 1 is not so long." + 
					"\nLine 2 is much longer than one before it but not as long as the one to follow." + 
					"\nLine 3 is much longer than one before it but not as long as the one to follow because it repeats itself once: Line 3 is much longer than one before it but not as long as the one to follow because it repeats itself." + 
					"\nLine 4 is much longer than one before it but not as long as the one to follow because it repeats itself twice: Line 4 is much longer than one before it but not as long as the one to follow because it repeats itself once more: Line 4 is much longer than one before it but not as long as the one to follow because it repeats itself." +  
					"\nLine 5 is slightly smaller than the one before it because it repeats itself twice: This line is slightly smaller than the one before it because it repeats itself once more: This line is slightly smaller than the one before it because it repeats itself." +
					"\nLine 6 is about the same as the one before it because it repeats itself twice: This line is about the same as the one before it because it repeats itself once more: This line is about the same as the one before it because it repeats itself." + 
					"\nLine 7 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 8 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 9 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					//"\nLine 10 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 11 is different than the one before it because it is the last one.");
			addOpenTruck (city + " 3", 
				"location 3", now + 600, "Details " + 
					"\nLine 1 is not so long." + 
					"\nLine 2 is much longer than one before it but not as long as the one to follow." + 
					"\nLine 3 is much longer than one before it but not as long as the one to follow because it repeats itself once: Line 3 is much longer than one before it but not as long as the one to follow because it repeats itself." + 
					"\nLine 4 is much longer than one before it but not as long as the one to follow because it repeats itself twice: Line 4 is much longer than one before it but not as long as the one to follow because it repeats itself once more: Line 4 is much longer than one before it but not as long as the one to follow because it repeats itself." +  
					"\nLine 5 is slightly smaller than the one before it because it repeats itself twice: This line is slightly smaller than the one before it because it repeats itself once more: This line is slightly smaller than the one before it because it repeats itself." +
					"\nLine 6 is about the same as the one before it because it repeats itself twice: This line is about the same as the one before it because it repeats itself once more: This line is about the same as the one before it because it repeats itself." + 
					"\nLine 7 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 8 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 9 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 10 is exactly the same as the one that follows it because it repeats itself twice: This line is exactly the same as the one that follows it because it repeats itself once more: This line is exactly the same as the one that follows it because it repeats itself." + 
					"\nLine 11 is different than the one before it because it is the last one.");
			break;
		default:
			break;
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
	if (localStorage[version + ".city"] !== null) {
		saveCity = localStorage[version + ".city"];
	}
	lookupCity(saveCity);
});

Pebble.addEventListener("showConfiguration", function() {
	var url = 'http://pebblemike.com/foodtrucks/';
	log("showing " + url);
	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
	log("configuration closed");
	// webview closed
	if (e.response !== undefined && e.response !== "") {
		var options = JSON.parse(decodeURIComponent(e.response));
		log("Options = " + JSON.stringify(options));
		localStorage[version + ".city"] = options.city;
		lookupCity(options.city);
	}
});

// Set callback for appmessage events
Pebble.addEventListener("appmessage", function(e) {
	log("message =" + JSON.stringify(e.payload));
	sendToPebble(e.payload.index);
});
