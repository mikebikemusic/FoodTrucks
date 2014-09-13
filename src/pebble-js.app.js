/*  Copyright (C) 2014 Mike Pompa

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.  
*/

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
var unitTest = false;

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
