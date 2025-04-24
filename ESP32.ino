
#include <WiFi.h>
#include <WebServer.h>

// ==================== CONFIGURACIÓN ====================
WebServer server(80);

enum ModoOperacion { MODO_SUMO, MODO_DIRIGIDO };
ModoOperacion modoActual = MODO_SUMO;

// === Pines del robot ===
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

// Estado del movimiento (modo sumo)
enum EstadoMovimiento { AVANZAR, RETROCEDER, DETENER, GIRAR };
EstadoMovimiento ultimoEstado = DETENER;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);

  // Pines motores
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Pines sensores
  pinMode(sensorTraseroIzq, INPUT);
  pinMode(sensorTraseroDer, INPUT);
  pinMode(sensorDelanteroDer, INPUT);
  pinMode(sensorDelanteroIzq, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // WiFi y servidor
  WiFi.softAP("CarroSumo", "12345678");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Red creada. Conéctate a: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="UTF-8">
        <title>Control de Carro</title>
        <style>
          body { font-family: sans-serif; text-align: center; margin-top: 40px; }
          button { padding: 10px 20px; margin: 10px; font-size: 18px; }
        </style>
      </head>
      <body>
        <h2>Selecciona el modo de operación:</h2>
        <button onclick="location.href='/cambiarModo?modo=sumo'">Modo Sumo</button>
        <button onclick="location.href='/cambiarModo?modo=dirigido'">Modo Dirigido</button>
        <p id="estado">Modo actual: Cargando...</p>

        <script>
          let modo = "";
          fetch('/estado').then(r => r.text()).then(t => {
            document.getElementById('estado').innerText = 'Modo actual: ' + t;
            modo = t;
          });

          document.addEventListener('keydown', (e) => {
            if (modo !== 'DIRIGIDO') return;
            let accion = "";
            if (e.key === 'w') accion = "avanzar";
            if (e.key === 's') accion = "retroceder";
            if (e.key === 'd') accion = "izquierda";
            if (e.key === 'a') accion = "derecha";
            if (accion !== "") fetch('/mover?accion=' + accion);
          });

          document.addEventListener('keyup', (e) => {
            if (modo === 'DIRIGIDO') {
              fetch('/mover?accion=detener');
            }
          });
        </script>
      </body>
      </html>
    )rawliteral");
  });

  server.on("/cambiarModo", HTTP_GET, []() {
    if (server.hasArg("modo")) {
      String modo = server.arg("modo");
      if (modo == "sumo") modoActual = MODO_SUMO;
      else if (modo == "dirigido") modoActual = MODO_DIRIGIDO;
      detener(); // Por seguridad, detener el carro al cambiar de modo
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

// ==================== LOOP ====================
void loop() {
  server.handleClient();

  if (modoActual == MODO_SUMO) {
    ejecutarModoSumo();
  }
}

// ==================== FUNCIONES DE MODO SUMO ====================
void ejecutarModoSumo() {
  int valTI = digitalRead(sensorTraseroIzq);
  int valTD = digitalRead(sensorTraseroDer);
  int valDI = digitalRead(sensorDelanteroIzq);
  int valDD = digitalRead(sensorDelanteroDer);

  if (valTI == HIGH || valTD == HIGH) {
    avanzar();
    ultimoEstado = AVANZAR;
  } 
  else if (valDI == HIGH || valDD == HIGH) {
    retroceder();
    ultimoEstado = RETROCEDER;
  } 
  else {
    long duration;
    float distance;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH, 30000);
    if (duration > 0) {
      distance = duration * 0.034 / 2;
      if (distance < 20.0) {
        avanzar();
        ultimoEstado = AVANZAR;
      } else {
        girar();
        ultimoEstado = GIRAR;
      }
    } else {
      girar();
      ultimoEstado = GIRAR;
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

// ==================== FUNCIONES DE MOVIMIENTO ====================
void avanzar() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  dutyENA = 255; dutyENB = 255;
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}

void retroceder() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  dutyENA = 255; dutyENB = 255;
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}

void girarIzquierda() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  dutyENA = 255; dutyENB = 255;
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}

void girarDerecha() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  dutyENA = 255; dutyENB = 255;
  analogWrite(ENA, 255); analogWrite(ENB, 255);
}

void girar() {
  girarDerecha();
}

void detener() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  dutyENA = 0; dutyENB = 0;
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}