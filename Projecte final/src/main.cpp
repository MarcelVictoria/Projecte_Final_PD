#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#include <WiFi.h>
#include "time.h"
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <Servo.h>


//DEFINICIÓ PINS

    //LEDS
#define LED_GREEN 2
#define LED_RED 32

  // pins SD i RFID
#define SS_PIN_RFID 5
#define CS_PIN_SD 15
#define RST_PIN_RFID 17
/*
SCK RFID I SCK SD pin18
MOSI RFID I MOSI SD pin23
MISO RFID I MISO SD pin19 (amb resistència 1000ohms)
*/




//OBJETOS

  //Creamos el objeto para el MFRC522
MFRC522 mfrc522(SS_PIN_RFID, RST_PIN_RFID);

  //Creamos el objeto AsyncWebServer  en port 80
AsyncWebServer server(80);

  //Creamos un Event Source en /events
AsyncEventSource events("/events");

  //Archivo SD
File myFile;

 //Tupla usuari
typedef struct
  {
    String name, id;
  } Usuari;

Usuari *usuaris = new Usuari[10]; // Crear un conjunto de usuarios

  //Servo
Servo myservo;

  //Pulsador
struct Button 
{
  const uint8_t PIN; //pin asignado
  uint32_t numberKeyPresses; //num. veces apretado
  bool unpressed, pressed; //apretado?
};

Button button1 = {21, 0, false, false}; //Incio: pin 21, 0 veces apretado, no apretado

  



//DECLARACIÓN VARIABLES  

  //Wifi ssid, contraseña
const char* ssid       = "iPhone de Victoria";
const char* password   = "patata01";

  //Servidor, variables de Hora_Data
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

  //Contadors
int contador_usuarios=1;
int contador_entradas=0;

  //Variables webserver
String UID;
String acceso;
String nom;
String hora_data;
bool entrada;

  //BARRERA(Servo)
boolean tancada=true, oberta=false;





//ACCIONES i FUNCIONES

  //PULSADOR
void IRAM_ATTR isr() //accion isr
{
  button1.numberKeyPresses += 1; //aumenta en 1
  button1.unpressed = true; //apretado true
}

  //SERVO
    //Abrir
void abrirbarreraservo()
{
    delay(1000);
    myservo.write(10);
    delay(50);
    myservo.write(15);
    delay(50);
    myservo.write(20);
    delay(50);
    myservo.write(25);
    delay(50);
    myservo.write(30);
    delay(50);
    myservo.write(35);
    delay(50);
    myservo.write(40);
    delay(50);
    myservo.write(45);
    delay(50);
    myservo.write(50);
    delay(50);
    myservo.write(55);
    delay(50);
    myservo.write(60);
    delay(50);
    myservo.write(65);
    delay(50);
    myservo.write(70);
    delay(50);
    myservo.write(75);
    delay(50);
    myservo.write(80);
    delay(50);
    myservo.write(85);
    delay(50);
    myservo.write(90);
    delay(1500);
}

    //Cerrar
void cerrarbarreraservo()
{
      delay(1000);
    myservo.write(80);
    delay(50);
    myservo.write(75);
    delay(50);
    myservo.write(70);
    delay(50);
    myservo.write(65);
    delay(50);
    myservo.write(60);
    delay(50);
    myservo.write(55);
    delay(50);
    myservo.write(50);
    delay(50);
    myservo.write(45);
    delay(50);
    myservo.write(40);
    delay(50);
    myservo.write(35);
    delay(50);
    myservo.write(30);
    delay(50);
    myservo.write(25);
    delay(50);
    myservo.write(20);
    delay(50);
    myservo.write(15);
    delay(50);
    myservo.write(10);
    delay(50);
    myservo.write(5);
    delay(50);
    myservo.write(0);
    delay(1500);
}

  //TARGETA SD
void mostrarUsuari(Usuari usuario, int numero) 
{
  Serial.print("Usuari");
  Serial.print(numero);
  Serial.print(": ");
  Serial.print(" ");
  Serial.print("Nom");
  Serial.print(" ");
  Serial.print(usuario.name);
  Serial.print(" ");
  Serial.print("id");
  Serial.print(" ");
  Serial.println(usuario.id);
}

    //Obtenir un usuario de l'arxiu a la SD
Usuari obtenerDatosUsuari(String linea) 
{
  int indiceEspacio = linea.indexOf(' ');
  int indiceComa = linea.indexOf(',');
  
  String nombre = linea.substring(0, indiceEspacio);
  String id = linea.substring(indiceEspacio + 1, indiceComa);
  
  Usuari usuario;
  usuario.name = nombre;
  usuario.id = id;
  
  return usuario;
}

  //LECTOR RFID
String obtenerUID() 
{
  String UID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
    UID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    UID.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  return UID;
}

    // Comprobar si existe el UID en la lista de usuarios
bool ExisteUID(String uid, int total) 
{
  for (int i = 0; i < total - 1; i++) 
  {
    if (usuaris[i].id.equals(uid)) {
      return true;
    }
  }
  return false;
}

    //Obtener el nombre del usuario si existe UID
String obtenerNombreUsuario(String UID) 
{
  for (int i = 0; i < contador_usuarios - 1; i++) 
  {
    if (usuaris[i].id == UID) 
    {
      return usuaris[i].name;
    }
  }
  return "";
}



  //DATA I HORA
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void getDataHora()
{
    hora_data="";
    struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
    char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);
  char timeSecond[3];
  strftime(timeSecond,3, "%S", &timeinfo);
  char timeDay[3];
  strftime(timeDay,3, "%d", &timeinfo);
  char timeMonth[3];
  strftime(timeMonth,3, "%m", &timeinfo);
  char timeYear[5];
  strftime(timeYear,5, "%Y", &timeinfo);
  hora_data+=timeHour;
  hora_data+=+":";
  hora_data+=timeMinute;
  hora_data+=":";
  hora_data+=timeSecond;
  hora_data+=" ";
  hora_data+=timeDay;
  hora_data+="/";
  hora_data+=timeMonth;
  hora_data+="/";
  hora_data+=timeYear;
}



  //SERVIDOR WEB

    //Obtener Datos Usuario de lector RFID para la web
void getUsuariReading()
{
  UID = obtenerUID();
  entrada = ExisteUID(UID, contador_usuarios);
  if (entrada == true) acceso = "Permitido";
  else acceso = "Denegado";
  nom = entrada ? obtenerNombreUsuario(UID) : "Desconocido";

  getDataHora();

    //Mostrar per pantalla les variables
    Serial.print("UID: ");
    Serial.println(UID);
    Serial.print("Acceso: ");
    Serial.println(acceso);
    Serial.print("Usuario: ");
    Serial.println(nom);
    Serial.print("Hora: ");
    Serial.println(hora_data);
}  

    //Variables actuales tomadas por servidorweb
String processor(const String& var)
{
  getUsuariReading();
  if (var == "USUARI"){
    return nom;
  }
  else if(var == "UID"){
    return UID;
  }
  else if(var == "HORA_DATA"){
    return hora_data;
  }
    else if(var == "ACCESO"){
    return acceso;
  }
  return String();
}
    //Código html servidorweb
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p { font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #50B8B4; color: white; font-size: 1rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
    .reading { font-size: 1.4rem; }
    table { width: 100%; border-collapse: collapse; }
    th, td { padding: 8px; border-bottom: 1px solid #ddd; }
    .acceso-permitido { background-color: green; color: white; }
    .acceso-denegado { background-color: red; color: white; }
  </style>
</head>
<body>
  <div class="topnav">
    <h1> SERVIDOR PARKING</h1>
  </div>
  <div class="content">
    <table>
      <thead>
        <tr>
          <th>NOM</th>
          <th>UID</th>
          <th>HORA_DATA</th>
          <th>ACCESO</th>
        </tr>
      </thead>
      <tbody id="data-table">
      </tbody>
    </table>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 var row = document.createElement("tr");
 source.addEventListener('nom', function(e) {
  console.log("nom", e.data);
  var cell = document.createElement("td");
  cell.innerHTML = e.data;
  row.appendChild(cell);
 }, false);
 
 source.addEventListener('UID', function(e) {
  console.log("UID", e.data);
  var cell = document.createElement("td");
  cell.innerHTML = e.data;
  row.appendChild(cell);
 }, false);

 source.addEventListener('hora_data', function(e) {
  console.log("hora_data", e.data);
  var cell = document.createElement("td");
  cell.innerHTML = e.data;
  row.appendChild(cell);
}, false);

source.addEventListener('acceso', function(e) {
  console.log("acceso", e.data);
  var cell = document.createElement("td");
  cell.innerHTML = e.data;
  
  // Verificar el valor de acceso y aplicar la clase correspondiente
  if (e.data === "Permitido") {
    cell.classList.add("acceso-permitido");
  } else if (e.data === "Denegado") {
    cell.classList.add("acceso-denegado");
  }
  
  row.appendChild(cell);
  
  // Verificar si se han agregado todas las celdas
  if (row.childElementCount === 4) {
    // Agregar la fila completa a la tabla
    document.getElementById("data-table").appendChild(row);
    
    // Crear una nueva fila para los siguientes datos
    row = document.createElement("tr");
  }
}, false);

} // Cierre del if (!!window.EventSource)

</script>
</body>
</html>)rawliteral";

  //WIFI
void initWiFi() 
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) 
    {
        Serial.print('.');
        delay(500);
    }
    Serial.println("CONNECTED");
    Serial.println(WiFi.localIP());//Mostrar IP 
    
}

void setup()
{
  Serial.begin(115200);

  Serial.println("Incialitzant sistema...");

  //Pins

    //Leds
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

    //Servo
  myservo.attach(16);

    //Pulsador
  pinMode(button1.PIN, INPUT_PULLUP); //asignamos pin botón

  Serial.println("Pins assignats amb exit");

  //Connexió al WiFi

  Serial.println("WIFI:");  
  initWiFi();

  delay(1000);


  // Inicializar el lector RFID

  Serial.print("Iniciando SPI ...");
  SPI.begin();

  Serial.print("Iniciando mfrc522 ...");
  mfrc522.PCD_Init();

  delay(1000);


  //Sincronitzar Hora i Data Rellotge

  Serial.println("Obtenint hora...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;

  Serial.println("Sincronitzant rellotge...");
  while(!getLocalTime(&timeinfo)) delay(1000);

  Serial.println("Rellotge sincronitzat");
  printLocalTime();

  delay (1000);


  //Incialitzar la targeta SD

  Serial.print("Iniciando SD ...");
  if (!SD.begin(15)) 
  {
    Serial.println("No se pudo inicializar");
    return;
  }

  Serial.println("Inicialitzacio exitosa");

  delay(1000);

  //Lectura arxiu.txt

  Serial.printf("Llegint: %s\n", "/usuaris.txt");

  myFile = SD.open("/usuaris.txt");
  
  if (myFile) 
  {
    Serial.println("archivo.txt:");
    while(myFile.available())
    {
      //Llegir fins (,)
      String linea = myFile.readStringUntil(',');
      Usuari usuario = obtenerDatosUsuari(linea);
      usuaris[contador_usuarios - 1] = usuario;
      contador_usuarios++;
    }
    for (int i = 0; i < contador_usuarios - 1; i++) 
    {
      mostrarUsuari(usuaris[i], i + 1);
    }

    myFile.close(); //cerramos el archivo

    delay(1000);
    
  } 
  else 
  {
    Serial.println("Error al abrir el archivo");
  }

  // Handle Web Server

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, processor);
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
  });

  //Events Servidor Web

    Serial.println("Preparando Servidor Async...");
    events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
  Serial.println("Servidor listo");


  //Interrupción pulsador

  attachInterrupt(button1.PIN, isr, RISING); //interrumpcion en pin 18, funcion isr, high a low

  //Leds

    //Encendemos las dos leds al mismo tiempo

    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, HIGH); 
    delay(2000);

    //Apagamos led verde -> programa preparado

    digitalWrite(LED_GREEN, LOW);
    delay(2000);

    Serial.println("Sistema Listo");
    Serial.println("Pasa la Tarjeta");
}

void loop()
{

  // Verificar si se detecta una tarjeta RFID
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) 
  {
    getUsuariReading();

    // Send Events to the Web Client with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(String(nom).c_str(),"nom",millis());
    events.send(String(UID).c_str(),"UID",millis());
    events.send(String(hora_data).c_str(),"hora_data",millis());
    events.send(String(acceso).c_str(),"acceso",millis());

    if (entrada) //acceso permitido
    {
      delay(2000);
      if (tancada == true)//si la puerta esta cerrada
      {
        //cambio a luz verde
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_RED, LOW);
        Serial.println("Llum verda");
        delay(1000);

        //abrir barrera
        abrirbarreraservo();
        Serial.println("Barrera oberta");
        delay(3000); //esperem a que estigui oberta

        Serial.println("El cotxe surt de la plataforma");

        while (!button1.unpressed)
        {
          //Esperamos a que el cotxe salga de la plataforma
          Serial.println(".");
          delay(500);
        };

        if (button1.unpressed)//el cotxe ha salido de la plataforma    
        {
          Serial.println ("Vehiculo fuera de la plataforma");
          Serial.printf("Button 1 has been pressed %u times\n", button1.numberKeyPresses);
      
          //asignamem variables booleanes perque pugui tancarse barrera
          oberta = true;
          tancada = false;

          //finalizamos interrupcion asignada en pin pulsador
          detachInterrupt(button1.PIN);
          delay(1000);
        }
      }

      if (oberta == true)//si la puerta esta abierta
      {
        //cambio luz a roja
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH);
        Serial.println("Llum vermella");
        delay(1000);

        //cerrar barrera
        cerrarbarreraservo();
        Serial.println("Barrera cerrada");
        delay(3000);
    
        //asignamem variables booleanes perque pugui obrirse la barrera
        tancada = true;
        oberta = false;
      }
        //interrumpcion en pin 18, funcion isr, high a low
        attachInterrupt(button1.PIN, isr, RISING);

        //asignem variable desapretar el botó com a false
        button1.unpressed = false; 
    }

    mfrc522.PICC_HaltA(); // Detener la comunicación con la tarjeta RFID
    Serial.println("Pasa la Tarjeta");
  }

  delay(5000);
}