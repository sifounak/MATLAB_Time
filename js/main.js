(function () {
	loadOptions();
	submitHandler();
})();

function submitHandler() {
	var $submitButton = $('#submitButton');

	$submitButton.on('click', function() {
		console.log('Submit');

		var return_to = getQueryParam('return_to', 'pebblejs://close#');
		document.location = return_to + encodeURIComponent(JSON.stringify(getAndStoreConfigData()));
	});
}

function loadOptions() {
	var $textColorPicker = $('#textColorPicker');
	var $backgroundColorPicker = $('#backgroundColorPicker');
	var $rotateLogo = $('#rotateLogo');
	var $reflectBatt = $('#reflectBatt');
	var $showBattPct = $('#showBattPct');
	var $vibeConnect = $('#vibeConnect');
	var $vibeDisconnect = $('#vibeDisconnect');
	var $tempUnits = $('#tempUnits');
	var $showConditions = $('#showConditions');
	
	if(localStorage.backgroundColor) {
		$textColorPicker[0].value = localStorage.textColorPicker;
		$backgroundColorPicker[0].value = localStorage.backgroundColorPicker;
		$rotateLogo[0].value = localStorage.rotateLogo;
		$reflectBatt[0].checked = localStorage.reflectBatt == true;
		$showBattPct[0].checked = localStorage.showBattPct == true;
		$vibeConnect[0].checked = localStorage.vibeConnect == true;
		$vibeDisconnect[0].checked = localStorage.vibeDisconnect == true;
		$tempUnits[0].value = localStorage.tempUnits;
		$showConditions[0].checked = localStorage.showConditions == true;
	}
}

function getAndStoreConfigData() {
	var $textColorPicker = $('#textColorPicker');
	var $backgroundColorPicker = $('#backgroundColorPicker');
	var $rotateLogo = $('#rotateLogo');
	var $reflectBatt = $('#reflectBatt');
	var $showBattPct = $('#showBattPct');
	var $vibeConnect = $('#vibeConnect');
	var $vibeDisconnect = $('#vibeDisconnect');
	var $tempUnits = $('#tempUnits');
	var $showConditions = $('#showConditions');

	var options = {
		backgroundColor: $backgroundColorPicker.val(),
		textColor: $textColorPicker.val(),
		//rotateLogo: $textColorPicker.val(),
		reflectBatt: $reflectBatt[0].checked,
		showBattPct: $showBattPct[0].checked,
		vibeDisconnect: $vibeDisconnect[0].checked,
		vibeConnect: $vibeConnect[0].checked,
		//tempUnits: $tempUnits.val(),
		showConditions: $showConditions[0].checked
	};
	
	localStorage.backgroundColor = options.backgroundColor;
	localStorage.textColor = options.textColor;
	//localStorage.rotateLogo = options.rotateLogo;
	localStorage.reflectBatt = options.reflectBatt;
	localStorage.showBattPct = options.showBattPct;
	localStorage.vibeDisconnect = options.vibeDisconnect;
	localStorage.vibeConnect = options.vibeConnect;
	//localStorage.tempUnits = options.tempUnits;
	localStorage.showConditions = options.showConditions;
	
	console.log('Got options: ' + JSON.stringify(options));
	return options;
}

function getQueryParam(variable, defaultValue) {
  var query = location.search.substring(1);
  var vars = query.split('&');
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split('=');
    if (pair[0] === variable) {
      return decodeURIComponent(pair[1]);
    }
  }
  return defaultValue || false;
}