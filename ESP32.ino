#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

// Definimos los pines de los motores
const int IN1 = 16;  // Motor izquierdo
const int IN2 = 17;
const int IN3 = 18;  // Motor derecho
const int IN4 = 19;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("CarritoSumo");  // Nombre del Bluetooth
  Serial.println("El dispositivo está listo para emparejarse");

  // Configuramos los pines como salidas
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Nos aseguramos de que el carro esté detenido al inicio
  detenerCarro();
}

void loop() {
  if (SerialBT.available()) {
    char comando = SerialBT.read();
    Serial.println(comando);

    switch (comando) {
      case 'F':  // Forward - Adelante
        adelante();
        break;
      case 'B':  // Backward - Atrás
        atras();
        break;
      case 'L':  // Left - Izquierda
        izquierda();
        break;
      case 'R':  // Right - Derecha
        derecha();
        break;
      case 'S':  // Stop - Parar
        detenerCarro();
        break;
      default:
        detenerCarro();
        break;
    }
  }
}

void adelante() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void atras() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void izquierda() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void derecha() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void detenerCarro() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
