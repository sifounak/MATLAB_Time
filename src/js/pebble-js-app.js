var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude + '&APPID=2874bea34ea1f91820fa07af69939eea';
  
  console.log("Lat is " + pos.coords.latitude);
  console.log("Lon is " + pos.coords.longitude);

  // Send request to forecast.io
  xhrRequest(url, 'GET', 
    function(responseText) {
      console.log("Parsing JSON");
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      console.log(JSON.parse(responseText));

      var temperature = Math.round(((json.main.temp - 273.15) * 1.8) + 32);
      console.log("Temperature in Fahrenheit is " + temperature);
      
      var temperaturec = Math.round(json.main.temp - 273.15);
      console.log("Temperature in Celsius is " + temperaturec);

      // Conditions
      var conditions = json.weather[0].main;      
      console.log("Conditions are " + conditions);
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_TEMPERATURE_IN_C": temperaturec,
        "KEY_CONDITIONS": conditions,
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS Ready! Getting weather.');

  getWeather();
});

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received! Updating weather.');

    getWeather();
  }                     
);

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://turnervink.github.io/square-config/';

  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));

  console.log('Configuration page returned: ' + JSON.stringify(configData));

  if (configData.textColor) {
    Pebble.sendAppMessage({
      textColor: parseInt(configData.textColor, 16),
      backgroundColor: parseInt(configData.backgroundColor, 16),
      invertColors: configData.invertColors ? 1 : 0,
      showWeather: configData.showWeather ? 1 : 0,
      shakeWeather: configData.shakeWeather ? 1 : 0,
      useCelsius: configData.useCelsius ? 1 : 0,
      vibeDisconnect: configData.vibeDisconnect ? 1 : 0,
      vibeConnect: configData.vibeConnect ? 1 : 0,
      reflectBatt: configData.reflectBatt ? 1 : 0
    }, function() {
      console.log('Send successful!');
    }, function() {
      console.log('Send failed!');
    });
  }
});