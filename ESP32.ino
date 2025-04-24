#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

enum ModoOperacion { MODO_SUMO, MODO_DIRIGIDO };
ModoOperacion modoActual = MODO_SUMO;

// Motores
const int IN1 = 22;
const int IN2 = 23;
const int IN3 = 2;
const int IN4 = 4;
const int ENA = 5;
const int ENB = 18;

// Sensores IR
const int sensorTraseroIzq = 21;
const int sensorTraseroDer = 13;
const int sensorDelanteroDer = 19;
const int sensorDelanteroIzq = 12;

// Sensor ultrasónico
const int trigPin = 14;
const int echoPin = 26;

// PWM manual (modo sumo)
const int periodoPWM = 1000;
int dutyENA = 255;
int dutyENB = 255;

enum EstadoMovimiento { AVANZAR, RETROCEDER, DETENER, GIRAR };
EstadoMovimiento ultimoEstado = DETENER;

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  pinMode(sensorTraseroIzq, INPUT);
  pinMode(sensorTraseroDer, INPUT);
  pinMode(sensorDelanteroDer, INPUT);
  pinMode(sensorDelanteroIzq, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  WiFi.softAP("CarroSumo", "12345678");
  Serial.print("Red creada. Conéctate a: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", paginaHTML);
  });

  server.on("/cambiarModo", HTTP_GET, []() {
    if (server.hasArg("modo")) {
      String modo = server.arg("modo");
      if (modo == "sumo") modoActual = MODO_SUMO;
      else if (modo == "dirigido") modoActual = MODO_DIRIGIDO;
      detener();
      Serial.println("Modo actual: " + String(modoActual == MODO_SUMO ? "SUMO" : "DIRIGIDO"));
      server.sendHeader("Location", "/", true);
      server.send(302, "Redireccionando...");
    } else {
      server.send(400, "Modo no especificado");
    }
  });

  server.on("/estado", HTTP_GET, []() {
    server.send(200, "text/plain", modoActual == MODO_SUMO ? "SUMO" : "DIRIGIDO");
  });

  server.on("/mover", HTTP_GET, []() {
    if (modoActual != MODO_DIRIGIDO) {
      server.send(403, "text/plain", "Modo no permitido");
      return;
    }

    if (!server.hasArg("accion")) {
      server.send(400, "text/plain", "Acción no especificada");
      return;
    }

    String accion = server.arg("accion");
    Serial.println("Acción recibida: " + accion);
    if (accion == "avanzar") avanzar();
    else if (accion == "retroceder") retroceder();
    else if (accion == "izquierda") girarIzquierda();
    else if (accion == "derecha") girarDerecha();
    else if (accion == "detener") detener();
    else {
      server.send(400, "text/plain", "Acción inválida");
      return;
    }

    server.send(200, "text/plain", "Acción: " + accion);
  });

  server.begin();
}

void loop() {
  server.handleClient();
  if (modoActual == MODO_SUMO) {
    ejecutarModoSumo();
  }
}

void ejecutarModoSumo() {
  int valTI = digitalRead(sensorTraseroIzq);
  int valTD = digitalRead(sensorTraseroDer);
  int valDI = digitalRead(sensorDelanteroIzq);
  int valDD = digitalRead(sensorDelanteroDer);

  if (valTI == HIGH || valTD == HIGH) {
    avanzar(); ultimoEstado = AVANZAR;
  } else if (valDI == HIGH || valDD == HIGH) {
    retroceder(); ultimoEstado = RETROCEDER;
  } else {
    long duration;
    float distance;

    digitalWrite(trigPin, LOW); delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH, 30000);
    if (duration > 0) {
      distance = duration * 0.034 / 2;
      if (distance < 20.0) {
        avanzar(); ultimoEstado = AVANZAR;
      } else {
        girar(); ultimoEstado = GIRAR;
      }
    } else {
      girar(); ultimoEstado = GIRAR;
    }
  }

  int tHighENA = map(dutyENA, 0, 255, 0, periodoPWM);
  int tHighENB = map(dutyENB, 0, 255, 0, periodoPWM);

  digitalWrite(ENA, HIGH); delayMicroseconds(tHighENA);
  digitalWrite(ENA, LOW);  delayMicroseconds(periodoPWM - tHighENA);
  digitalWrite(ENB, HIGH); delayMicroseconds(tHighENB);
  digitalWrite(ENB, LOW);  delayMicroseconds(periodoPWM - tHighENB);

  delay(5);
}

void avanzar() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  dutyENA = dutyENB = 255;
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}

void retroceder() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  dutyENA = dutyENB = 255;
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}

void girarIzquierda() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  dutyENA = dutyENB = 255;
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}

void girarDerecha() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  dutyENA = dutyENB = 255;
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}

void girar() {
  girarDerecha();
}

void detener() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  dutyENA = dutyENB = 0;
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}