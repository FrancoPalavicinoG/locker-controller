#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define RXp2 16
#define TXp2 17
#define LED0 27
#define LED1 26
#define LED2 25
#define LED3 33
#define LED4 23
#define LED5 22


#define BUTTON_1 32
#define BUTTON_2 35
#define BUTTON_3 4
#define SERVO_1 18
#define SERVO_2 19
#define SERVO_3 2

// Label map: 'palm': 1, 'L': 2, 'fist_moved': 3, 'index': 4, 'ok': 5, 'palm_moved': 6

const char* ssid = "wifi-campus";  //"movistar2,4GHZ_8F8FAF"; wifi-campus
const char* password = "uandes2200";  //"Mx39bUhjkcbrJf3R8HGU"; uandes2200
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

int currentGestureLocker1 = 0;
int currentGestureLocker2 = 0;
int currentGestureLocker3 = 0;
int closedPosition = 100;
int openPostion = 0;
bool locker_1 = false;  
bool button1Pressed = false; 
bool locker_2 = false;  
bool button2Pressed = false; 
bool locker_3 = false;  
bool button3Pressed = false; 
bool servo1opened = false;  
bool servo2opened = false; 
bool servo3opened = false; 
int isServo = 0;

String id;

Servo servo;
Servo servo2;
Servo servo3;

String message;

// PIN_1 =  3256
String P1_1 = "3"; // fist_moved       
String P1_2 = "2"; // L         
String P1_3 = "5"; // ok    
String P1_4 = "6"; // palm_moved

// PIN_2 = 4531
String P2_1 = "4"; // index  
String P2_2 = "5"; // ok           
String P2_3 = "3"; // fist_moved
String P2_4 = "1"; // palm

// PIN_3 = 4531
String P3_1 = "4"; // index  
String P3_2 = "5"; // ok           
String P3_3 = "3"; // fist_moved
String P3_4 = "1"; // palm

void setup() {
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  servo.attach(SERVO_1, 500, 2500);
  servo.write(closedPosition); 
  servo2.attach(SERVO_2, 500, 2500);
  servo2.write(closedPosition); 
  servo3.attach(SERVO_3, 500, 2500);
  servo3.write(closedPosition);
  Serial.begin(9600);
  Serial2.begin(115200, SERIAL_8N1, RXp2, TXp2);

  WiFi.begin(ssid, password);
  Serial.println("..............");

  Serial.print("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED)
       {  delay(500);
          Serial.print(".") ;
       }
  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqttPort);
  client.setKeepAlive(60);


  connectMQTT();
  client.setCallback(callback);
}

void loop() {
  // Conexion MQTT
  connectMQTT();  
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  // Control de botones
  if (digitalRead(BUTTON_1) == HIGH && !button1Pressed) {
    button1Pressed = true; 
    locker_1 = !locker_1;  
    blinkLED(LED0);
    delay(300); 
  }

  if (digitalRead(BUTTON_2) == HIGH && !button2Pressed) {
    button2Pressed = true; 
    locker_2 = !locker_2;
    blinkLED(LED1);  
    delay(300); 
  }

  if (digitalRead(BUTTON_3) == HIGH && !button3Pressed) {
    button3Pressed = true; 
    locker_3 = !locker_3;
    blinkLED(LED2);  
    delay(300); 
  }

  if (digitalRead(BUTTON_1) == LOW && button1Pressed) {
    button1Pressed = false;  
    servo1_closer();
    currentGestureLocker1 = 0;
  }

  if (digitalRead(BUTTON_2) == LOW && button2Pressed) {
    button2Pressed = false;  
    servo2_closer();
    currentGestureLocker2 = 0;
  }

  if (digitalRead(BUTTON_3) == LOW && button3Pressed) {
    button3Pressed = false;  
    servo3_closer();
    currentGestureLocker3 = 0;
  }

  // Control del locker 1 y 2 para leer gestos y accionar el servo
  if (locker_1) {
    delay(2000); 
    isServo = 1;
    if (Serial2.available()) {
      String receivedData = Serial2.readString();
      locker_controller(receivedData, P1_1, P1_2, P1_3, P1_4, currentGestureLocker1, isServo);
    }
  } else if (locker_2) {
    delay(2000); 
    isServo = 2;
    if (Serial2.available()) {
      String receivedData = Serial2.readString();
      locker_controller(receivedData, P2_1, P2_2, P2_3, P2_4, currentGestureLocker2, isServo);
    }
  } else if (locker_3){
    delay(2000); 
    isServo = 3;
    if (Serial2.available()) {
      String receivedData = Serial2.readString();
      locker_controller(receivedData, P3_1, P3_2, P3_3, P3_4, currentGestureLocker3, isServo);
    }
  } else {
    Serial.println("Esperando inicio");
    delay(1000); 
    gestureLED(0);
  }
}

void locker_controller(String receivedData, String P1, String P2, String P3, String P4, int &currentGesture, int isServo) {
  
  int parser_1 = receivedData.indexOf(';');

  if (parser_1 >= 0) {
    String detected_gesture_1 = receivedData.substring(0, parser_1);
    String detected_gesture_2 = receivedData.substring(parser_1 + 1);
    int parser_2 = detected_gesture_2.indexOf('.');

    if (parser_2 >= 0) {
      detected_gesture_2 = detected_gesture_2.substring(0, parser_2);
    }

    Serial.println("Detected 1: " + detected_gesture_1 + ", Detected 2: " + detected_gesture_2);

    switch (currentGesture) {
      case 0: 
        if (detected_gesture_1 == P1 || detected_gesture_2 == P1) {
          Serial.println("Primer gesto correcto");
          gestureLED(P1.toInt());
          delay(1500);
          currentGesture++;
        }
        break;
        
      case 1:
        if (detected_gesture_1 == P2 || detected_gesture_2 == P2) {
          Serial.println("Segundo gesto correcto");
          gestureLED(P2.toInt());
          delay(1500);
          currentGesture++;
        }
        break;
        
      case 2:
        if (detected_gesture_1 == P3 || detected_gesture_2 == P3) {
          Serial.println("Tercer gesto correcto");
          gestureLED(P3.toInt());
          delay(1500);
          currentGesture++;
        }
        break;
        
      case 3:
        if (detected_gesture_1 == P4 || detected_gesture_2 == P4) {
          Serial.println("Cuarto gesto correcto");
          gestureLED(P4.toInt());
          delay(1500);
          gestureLED(0);
          if (isServo == 1){
            servo1_opener();  
          } else if (isServo == 2) {
            servo2_opener();  
          } else if (isServo == 3) {
            servo3_opener();  
          }
          delay(2500);
          currentGesture = 0;  
        }
        break;
        
      default:
        break;
    }
  }
}

void servo1_opener() {
  if (!servo1opened) {     
    servo.write(openPostion);     
    delay(1500);          
    servo1opened = true; 
    Serial.print("Open locker 1");
    // publicar en open_locker_g6 el id del casillero
    if (client.connected()) {
      // Publicar el mensaje al tópico
      client.publish("open_locker_g6", "{ \"id\": \"1\" }"); 
      Serial.println("Mensaje enviado al topic: open_locker_g6");
    } else {
      Serial.println("Fallo al publicar, intentando reconectar...");
      connectMQTT();
      client.publish("open_locker_g6", "{ \"id\": \"1\" }");
      Serial.println("Mensaje enviado al topic: open_locker_g6");
    }
  }
}

void servo1_closer() {
  if (servo1opened) {     
    servo.write(closedPosition);     
    delay(1500);          
    servo1opened = false;   
  }
}

void servo2_opener() {
  if (!servo2opened) {     
    servo2.write(openPostion);     
    delay(1500);          
    servo2opened = true;  
    Serial.print("Open locker 2");
    // publicar en open_locker_g6 el id del casillero
    if (client.connected()) {
      // Publicar el mensaje al tópico
      client.publish("open_locker_g6", "{ \"id\": \"2\" }"); 
      Serial.println("Mensaje enviado al topic: open_locker_g6");
    } else {
      Serial.println("Fallo al publicar, intentando reconectar...");
      connectMQTT();
      client.publish("open_locker_g6", "{ \"id\": \"2\" }");
      Serial.println("Mensaje enviado al topic: open_locker_g6");
    } 
  }
}

void servo2_closer() {
  if (servo2opened) {     
    servo2.write(closedPosition);     
    delay(1500);          
    servo2opened = false;   
  }
}

void servo3_opener() {
  if (!servo3opened) {     
    servo3.write(openPostion);     
    delay(1500);          
    servo3opened = true;  
    Serial.print("Open locker 3");
    // publicar en open_locker_g6 el id del casillero
    if (client.connected()) {
      // Publicar el mensaje al tópico
      client.publish("open_locker_g6", "{ \"id\": \"3\" }"); 
      Serial.println("Mensaje enviado al topic: open_locker_g6");
    } else {
      Serial.println("Fallo al publicar, intentando reconectar...");
      connectMQTT();
      client.publish("open_locker_g6", "{ \"id\": \"3\" }");
      Serial.println("Mensaje enviado al topic: open_locker_g6");
    } 
  }
}

void servo3_closer() {
  if (servo3opened) {     
    servo3.write(closedPosition);     
    delay(1500);          
    servo3opened = false;   
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el tema: ");
  Serial.println(topic);

  message = ""; 

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensaje JSON recibido: ");
  Serial.println(message);

  // Procesar el JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("Error al analizar el JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // setear id y pin password
  id = doc["id"].as<String>();
  String pin = doc["password"].as<String>();

  // set password Locker 1
  if (id == "1") {
    if (pin.length() == 4) {
      P1_1 = String(pin[0]); 
      P1_2 = String(pin[1]); 
      P1_3 = String(pin[2]); 
      P1_4 = String(pin[3]); 
    } else {
      Serial.println("Error: El PIN no tiene 4 dígitos");
      return;
    }
    // set password Locker 2
  } else if (id == "2") {
    if (pin.length() == 4) {
      P2_1 = String(pin[0]); 
      P2_2 = String(pin[1]); 
      P2_3 = String(pin[2]); 
      P2_4 = String(pin[3]); 
    } else {
      Serial.println("Error: El PIN no tiene 4 dígitos");
      return;
    }
  } else if (id == "3") {
    if (pin.length() == 4) {
      P3_1 = String(pin[0]); 
      P3_2 = String(pin[1]); 
      P3_3 = String(pin[2]); 
      P3_4 = String(pin[3]); 
    } else {
      Serial.println("Error: El PIN no tiene 4 dígitos");
      return;
    }
  }

  Serial.print("Locker ID: ");
  Serial.println(id);

  if (id == "1") {
    blinkLED(LED0);
    Serial.println("P1_1: " + P1_1);
    gestureLED(P1_1.toInt());
    delay(500);
    Serial.println("P1_2: " + P1_2);
    gestureLED(P1_2.toInt());
    delay(500);
    Serial.println("P1_3: " + P1_3);
    gestureLED(P1_3.toInt());
    delay(500);
    Serial.println("P1_4: " + P1_4);
    gestureLED(P1_4.toInt());
    delay(500);
  } else if (id == "2"){
    blinkLED(LED1);
    Serial.println("P2_1: " + P2_1);
    gestureLED(P2_1.toInt());
    delay(500);
    Serial.println("P2_2: " + P2_2);
    gestureLED(P2_2.toInt());
    delay(500);
    Serial.println("P2_3: " + P2_3);
    gestureLED(P2_3.toInt());
    delay(500);
    Serial.println("P2_4: " + P2_4);
    gestureLED(P2_4.toInt());
    delay(500);
  } else if (id == "3"){
    blinkLED(LED2);
    Serial.println("P3_1: " + P3_1);
    gestureLED(P3_1.toInt());
    delay(500);
    Serial.println("P3_2: " + P3_2);
    gestureLED(P3_2.toInt());
    delay(500);
    Serial.println("P3_3: " + P3_3);
    gestureLED(P3_3.toInt());
    delay(500);
    Serial.println("P3_4: " + P3_4);
    gestureLED(P3_4.toInt());
    delay(500);
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.println("Conectando al broker MQTT...");
    if (client.connect("ArduinoClient")) {
      Serial.println("Conectado al broker MQTT");
      client.subscribe("set_locker_g6"); 
      
    } else {
      Serial.print("Error de conexión, estado: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void gestureLED(int gesture) {
  if (gesture == 0) {
    digitalWrite(LED0, LOW);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
    digitalWrite(LED5, LOW);
  } else if (gesture == 1) {
    digitalWrite(LED0, HIGH);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
    digitalWrite(LED5, LOW);
  } else if (gesture == 2) {
    digitalWrite(LED0, HIGH);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
    digitalWrite(LED5, LOW);
  } else if (gesture == 3) {
    digitalWrite(LED0, HIGH);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
    digitalWrite(LED5, LOW);
  } else if (gesture == 4) {
    digitalWrite(LED0, HIGH);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, LOW);
    digitalWrite(LED5, LOW);
  } else if (gesture == 5) {
    digitalWrite(LED0, HIGH);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
    digitalWrite(LED5, LOW);
  } else if (gesture == 6) {
    digitalWrite(LED0, HIGH);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
    digitalWrite(LED5, HIGH);
  }
}

void blinkLED(int led) {
  unsigned long startTime = millis(); 
  while (millis() - startTime < 2000) { 
    digitalWrite(led, HIGH); 
    delay(250); 
    digitalWrite(led, LOW); 
    delay(250); 
  }
}


