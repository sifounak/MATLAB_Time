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
	var $invertColors = $('#invertColors');
	var $showWeather = $('#showWeather');
	var $shakeWeather = $('#shakeWeather');
	var $useCelsius = $('#useCelsius');
	var $vibeConnect = $('#vibeConnect');
	var $vibeDisconnect = $('#vibeDisconnect');
	var $reflectBatt = $('#reflectBatt');

	if (localStorage.invertColors) {
		$textColorPicker[0].value = localStorage.textColor;
		$backgroundColorPicker[0].value = localStorage.backgroundColor;
		$invertColors[0].checked = localStorage.invertColors === 'true';
		$showWeather[0].checked = localStorage.showWeather === 'true';
		$shakeWeather[0].checked = localStorage.shakeWeather === 'true';
		$useCelsius[0].checked = localStorage.useCelsius === 'true';
		$vibeConnect[0].checked = localStorage.vibeConnect === 'true';
		$vibeDisconnect[0].checked = localStorage.vibeDisconnect === 'true';
		$reflectBatt[0].checked = localStorage.reflectBatt === 'true';
	}
}

function getAndStoreConfigData() {
	var $textColorPicker = $('#textColorPicker');
	var $backgroundColorPicker = $('#backgroundColorPicker');
	var $invertColors = $('#invertColors');
	var $showWeather = $('#showWeather');
	var $shakeWeather = $('#shakeWeather');
	var $useCelsius = $('#useCelsius');
	var $vibeConnect = $('#vibeConnect');
	var $vibeDisconnect = $('#vibeDisconnect');
	var $reflectBatt = $('#reflectBatt');

	var options = {
		textColor: $textColorPicker.val(),
		backgroundColor: $backgroundColorPicker.val(),
		invertColors: $invertColors[0].checked,
		showWeather: $showWeather[0].checked,
		shakeWeather: $shakeWeather[0].checked,
		useCelsius: $useCelsius[0].checked,
		vibeConnect: $vibeConnect[0].checked,
		vibeDisconnect: $vibeDisconnect[0].checked,
		reflectBatt: $reflectBatt[0].checked
	};

	localStorage.textColor = options.textColor;
	localStorage.backgroundColor = options.backgroundColor;
	localStorage.invertColors = options.invertColors;
	localStorage.showWeather = options.showWeather;
	localStorage.shakeWeather = options.shakeWeather;
	localStorage.useCelsius = options.useCelsius;
	localStorage.vibeConnect = options.vibeConnect;
	localStorage.vibeDisconnect = options.vibeDisconnect;
	localStorage.reflectBatt = options.reflectBatt;

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