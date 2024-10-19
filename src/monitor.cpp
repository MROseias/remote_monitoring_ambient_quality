// Import required libraries 
#include <Arduino.h> 
#include <ESP8266WiFi.h> 
#include <Hash.h> 
#include <ESPAsyncTCP.h> 
#include <ESPAsyncWebServer.h> 
#include <Adafruit_Sensor.h> 
#include <DHT.h> 


  
// Replace with your network credentials 
const char* ssid = "CAIRES 2G"; 
const char* password = getenv("WIFI-SENHA"); 

#define DHTPIN 5     // Digital pin D1 connected to the DHT sensor 
 
// Uncomment the type of sensor in use: 

// #define DHTTYPE    DHT11     // DHT 11 
#define DHTTYPE    DHT22     // DHT 22 (AM2302) 
//#define DHTTYPE    DHT21     // DHT 21 (AM2301) 

DHT dht(DHTPIN, DHTTYPE); 

// current temperature & humidity, updated in loop() 
float t = 0.0; 
float h = 0.0; 

// Create AsyncWebServer object on port 80 

AsyncWebServer server(80); 

// Generally, you should use "unsigned long" for variables that hold time 
// The value will quickly become too large for an int to store 

unsigned long previousMillis = 0;    // will store last time DHT was updated 

// Updates DHT readings every 10 seconds 
const long interval = 10000;   
const char index_html[] PROGMEM = R"rawliteral( 

<!DOCTYPE HTML> 
<html lang="pt-BR"> 
<head> 
  <meta charset="UTF-8"> 
  <meta name="viewport" content="width=device-width, initial-scale=1"> 
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous"> 
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script> 
  <style> 
    html { 
      font-family: Arial; 
      display: inline-block; 
      margin: 0px auto; 
      text-align: center; 
    } 
    h2 { font-size: 3.0rem; } 
    p { font-size: 2.0rem; } /* Tamanho de fonte ajustado para a linha de temperatura e umidade */ 
    .units { font-size: 1.2rem; } 
    .dht-labels { 
      font-size: 1.5rem; 
      vertical-align: middle; 
      padding-bottom: 15px; 
    } 
    .creator-name { 
      font-size: 1.0rem; /* Tamanho reduzido dos nomes */ 
    } 
    .project-creator { 
      font-size: 1.2rem; /* Tamanho reduzido da frase "Criadores do Projeto" */ 
    } 
    canvas { 
      max-width: 600px; 
      margin: 20px auto; 
    } 
  </style> 

</head> 
<body> 
  <h2>ESP8266 DHT Server</h2> 
    <p> 
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>  
    <span class="dht-labels">Temperature:</span>  
    <span id="temperature">%TEMPERATURE%</span> 
    <sup class="units">°C</sup> 
    &nbsp; &nbsp; <!-- Espaçamento entre temperatura e umidade --> 
    <i class="fas fa-tint" style="color:#00add6;"></i>  
    <span class="dht-labels">Humidity:</span> 
    <span id="humidity">%HUMIDITY%</span> 
    <sup class="units">%</sup> 
  </p> 
  <!-- Gráfico de linha --> 
  <canvas id="myChart" width="1100" height="450"></canvas> 
  <p class="project-creator"> 
    Criadores do Projeto:<br> 
    <span class="creator-name"><strong>Oséias Henrique</strong></span>,<br> 
    <span class="creator-name"><strong>Antônio Guillherme</strong></span>,<br> 
    <span class="creator-name"><strong>Vitor Oliveira</strong></span> 
  </p> 

  <script> 
    const ctx = document.getElementById('myChart').getContext('2d'); 
    const myChart = new Chart(ctx, { 
      type: 'line', 
      data: { 
        labels: [], // Labels do eixo x (tempo) 
        datasets: [ 
          { 
            label: 'Temperatura (°C)', 
            data: [], 
            borderColor: 'rgba(255, 99, 132, 1)', 
            backgroundColor: 'rgba(255, 99, 132, 0.2)', 
            borderWidth: 1 
          }, 

          { 
            label: 'Umidade (%)', 
            data: [], 
            borderColor: 'rgba(54, 162, 235, 1)', 
            backgroundColor: 'rgba(54, 162, 235, 0.2)', 
            borderWidth: 1 
          } 
        ] 
      }, 
      options: { 
        scales: { 
          y: { 
            beginAtZero: true 
          } 
        } 
      } 
    }); 

    function updateChart(temperature, humidity) { 
      const currentTime = new Date().toLocaleTimeString(); 
      if (myChart.data.labels.length >= 12) { // Limita a 12 pontos (5 minutos) 
        myChart.data.labels.shift(); 
        myChart.data.datasets[0].data.shift(); 
        myChart.data.datasets[1].data.shift(); 
      } 
      myChart.data.labels.push(currentTime); 
      myChart.data.datasets[0].data.push(temperature); 
      myChart.data.datasets[1].data.push(humidity); 
      myChart.update(); 
    } 

    setInterval(function () { 
      var xhttp = new XMLHttpRequest(); 
      xhttp.onreadystatechange = function() { 
        if (this.readyState == 4 && this.status == 200) { 
          const temperature = parseFloat(this.responseText); 
          document.getElementById("temperature").innerHTML = temperature; 
          updateChart(temperature, document.getElementById("humidity").innerHTML); 
        } 
      }; 
      xhttp.open("GET", "/temperature", true); 
      xhttp.send(); 
    }, 60000); // Atualização a cada 1 minuto 

    setInterval(function () { 
      var xhttp = new XMLHttpRequest(); 
      xhttp.onreadystatechange = function() { 
        if (this.readyState == 4 && this.status == 200) { 
          const humidity = parseFloat(this.responseText); 
          document.getElementById("humidity").innerHTML = humidity; 
          updateChart(document.getElementById("temperature").innerHTML, humidity); 
        } 

      }; 

      xhttp.open("GET", "/humidity", true); 
      xhttp.send(); 
    }, 60000); // Atualização a cada 1 minuto 
  </script> 
</body> 
</html> 
)rawliteral"; 

  
// Replaces placeholder with DHT values 
String processor(const String& var){ 
  //Serial.println(var); 
  if(var == "TEMPERATURE"){ 
    return String(t); 
  } 
  else if(var == "HUMIDITY"){ 
    return String(h); 
  } 
  return String(); 
} 

  
void setup(){ 
  // Serial port for debugging purposes 
  Serial.begin(115200); 
  dht.begin();

  // Connect to Wi-Fi 
  WiFi.begin(ssid, password); 
  Serial.println("Connecting to WiFi"); 

  while (WiFi.status() != WL_CONNECTED) { 
    delay(1000); 
    Serial.println("."); 
  } 

  // Print ESP8266 Local IP Address 
  Serial.println(WiFi.localIP()); 

  // Route for root / web page 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send_P(200, "text/html", index_html, processor); 
  }); 

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send_P(200, "text/plain", String(t).c_str()); 

  }); 
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send_P(200, "text/plain", String(h).c_str()); 

  }); 
  // Start server 
  server.begin(); 

} 

  

void loop(){   
  unsigned long currentMillis = millis(); 
  if (currentMillis - previousMillis >= interval) { 
    // save the last time you updated the DHT values 
    previousMillis = currentMillis; 
    // Read temperature as Celsius (the default) 
    float newT = dht.readTemperature(); 
    // Read temperature as Fahrenheit (isFahrenheit = true) 
    //float newT = dht.readTemperature(true); 
    // if temperature read failed, don't change t value 
    if (isnan(newT)) { 
      Serial.println("Failed to read from DHT sensor!"); 
    } 
    else { 
      t = newT; 
      Serial.println(t); 
    } 
    // Read Humidity 
    float newH = dht.readHumidity(); 
    // if humidity read failed, don't change h value  
    if (isnan(newH)) { 
      Serial.println("Failed to read from DHT sensor!"); 
    } 
    else { 
      h = newH; 
      Serial.println(h); 
    } 

  } 

} 