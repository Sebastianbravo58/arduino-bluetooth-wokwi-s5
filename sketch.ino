#include <SoftwareSerial.h>

// Definición de pines (coherente con diagrama Fritzing)
const int BT_RX_PIN = 10;    // Conectado a TXD del módulo HC-05/HC-06
const int BT_TX_PIN = 11;    // Conectado a RXD del módulo HC-05/HC-06
const int LED_ACTUADOR = 13; // Salida principal controlada por Bluetooth
const int LED_ESTADO = 12;   // Indicador de actividad no bloqueante (millis)

SoftwareSerial btSerial(BT_RX_PIN, BT_TX_PIN);

// Variables de control temporal no bloqueante
unsigned long tiempoPrevioHeartbeat = 0;
const unsigned long INTERVALO_HEARTBEAT = 1000; // 1 segundo
bool estadoHeartbeat = false;

unsigned long tiempoInicioComando = 0;
unsigned long totalTramasRecibidas = 0;
unsigned long totalTramasValidas = 0;

void setup() {
  pinMode(LED_ACTUADOR, OUTPUT);
  pinMode(LED_ESTADO, OUTPUT);
  digitalWrite(LED_ACTUADOR, LOW);
  digitalWrite(LED_ESTADO, LOW);

  // Inicialización de comunicación serial a 9600 baudios
  Serial.begin(9600);
  btSerial.begin(9600);

  Serial.println(F("=================================================="));
  Serial.println(F("SISTEMA DE TELEMETRIA SERIAL BLUETOOTH - ARDUINO"));
  Serial.println(F("Baud Rate: 9600 bps | Arquitectura: No bloqueante"));
  Serial.println(F("Comandos: [1] Encender | [0] Apagar | [E] Estado"));
  Serial.println(F("=================================================="));
}

void procesarTrama(char comando, const char* origen) {
  if (comando == '\n' || comando == '\r' || comando == ' ') return;

  tiempoInicioComando = micros();
  totalTramasRecibidas++;

  Serial.print(F("[t="));
  Serial.print(millis());
  Serial.print(F(" ms] Origen: "));
  Serial.print(origen);
  Serial.print(F(" | Dato: '"));
  Serial.print(comando);
  Serial.println(F("'"));

  if (comando == '1') {
    digitalWrite(LED_ACTUADOR, HIGH);
    totalTramasValidas++;
    unsigned long latencia = micros() - tiempoInicioComando;
    Serial.print(F("  -> ACCION: Actuador ENCENDIDO | Latencia proc.: "));
    Serial.print(latencia);
    Serial.println(F(" us"));
    btSerial.println(F("ACK:LED_ON"));
  } 
  else if (comando == '0') {
    digitalWrite(LED_ACTUADOR, LOW);
    totalTramasValidas++;
    unsigned long latencia = micros() - tiempoInicioComando;
    Serial.print(F("  -> ACCION: Actuador APAGADO | Latencia proc.: "));
    Serial.print(latencia);
    Serial.println(F(" us"));
    btSerial.println(F("ACK:LED_OFF"));
  } 
  else if (comando == 'E' || comando == 'e') {
    totalTramasValidas++;
    bool estadoActual = digitalRead(LED_ACTUADOR);
    Serial.print(F("  -> TELEMETRIA: Actuador="));
    Serial.print(estadoActual ? F("ON") : F("OFF"));
    Serial.print(F(" | Tramas validas="));
    Serial.print(totalTramasValidas);
    Serial.print(F("/"));
    Serial.println(totalTramasRecibidas);
    btSerial.println(estadoActual ? F("STAT:ON") : F("STAT:OFF"));
  } 
  else {
    Serial.println(F("  -> ERROR: Comando no reconocido (trama descartada)"));
    btSerial.println(F("ERR:INVALID_CMD"));
  }
}

void loop() {
  unsigned long tiempoActual = millis();

  // Tarea concurrente 1: Parpadeo de supervisión sin delay()
  if (tiempoActual - tiempoPrevioHeartbeat >= INTERVALO_HEARTBEAT) {
    tiempoPrevioHeartbeat = tiempoActual;
    estadoHeartbeat = !estadoHeartbeat;
    digitalWrite(LED_ESTADO, estadoHeartbeat);
  }

  // Tarea concurrente 2: Recepción desde puerto Bluetooth (SoftwareSerial)
  if (btSerial.available() > 0) {
    char datoBT = btSerial.read();
    procesarTrama(datoBT, "Bluetooth_UART");
  }

  // Tarea concurrente 3: Recepción desde consola serial Wokwi
  if (Serial.available() > 0) {
    char datoUSB = Serial.read();
    procesarTrama(datoUSB, "Monitor_Serial");
  }
}
