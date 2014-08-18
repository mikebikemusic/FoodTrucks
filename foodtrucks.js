var saveCity = "boston";
var version = "foodtrucks.v1";

function log(msg) {
	//console.log(msg);
}
function getSavedSettings() {

	if (localStorage[version + ".city"] !== undefined) {
		saveCity = localStorage[version + ".city"];
        $("#selectCity").val([]);
		$("#selectCity option[value='" + saveCity + "']").attr('selected', 'selected');
        $("#selectCity").selectmenu('refresh');
	}
}

$().ready(function(){
	log("Ready ");
	getSavedSettings();

	$("#selectCity").blur (function (event){
		saveCity = $("#selectCity").val();
	});

	$("#b-submit").click(function() {
		log("Submit");
		var options = { 'city': saveCity };
		localStorage[version + ".city"] = saveCity;

		var location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(options));
		log("Warping to: " + location);
		document.location = location;
	});

	$("#b-cancel").click(function() {
		log("Cancel");
		document.location = "pebblejs://close";
	});
});
