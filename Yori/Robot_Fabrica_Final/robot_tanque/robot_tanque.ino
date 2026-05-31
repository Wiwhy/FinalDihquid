#include "WiFiEsp.h"
#include "WiFiEspUdp.h"
#include <Servo.h>

char ssid[] = "ROBOT_COMPETICION";
char pass[] = "12345678";
int status = WL_IDLE_STATUS;
unsigned int localPort = 8080;
WiFiEspUDP Udp;

unsigned int puertoPuerta = 8081;

// ==========================================
// PINES MODO TANQUE (Usando Canales BK)
// ==========================================
// PINES DERECHOS (AOUT - AK3/AK4 -> Motor A / ENB, IN3, IN4)
#define speedPinR 12
#define RightMotorDirPin1 7
#define RightMotorDirPin2 8

// PINES IZQUIERDOS (BOUT - BK3/BK4 -> Motor B / ENB, IN3, IN4)
#define speedPinL 10
#define LeftMotorDirPin1 26
#define LeftMotorDirPin2 28

// ==========================================
// CONFIGURACIÓN DEL SERVO (GANCHO)
// ==========================================
#define PIN_SERVO 4
Servo servoGancho;
int angulo_gancho = 90;

// ==========================================
// VARIABLES DE DATOS
// ==========================================
int cur_l = 0, target_l = 0;
int cur_r = 0, target_r = 0;
int target_pinza = 0;
unsigned long last_millis = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  pinMode(RightMotorDirPin1, OUTPUT);
  pinMode(RightMotorDirPin2, OUTPUT);
  pinMode(speedPinR, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT);
  pinMode(LeftMotorDirPin2, OUTPUT);
  pinMode(speedPinL, OUTPUT);

  apagar_motores();

  servoGancho.attach(PIN_SERVO);
  servoGancho.write(angulo_gancho);

  WiFi.init(&Serial1);
  WiFi.beginAP(ssid, 10, pass, ENC_TYPE_WPA2_PSK);
  Udp.begin(localPort);
  Serial.println("¡Modo Tanque (AK/BK) + Servo + Puerta listos!");
}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    char packetBuffer[64];
    int len = Udp.read(packetBuffer, 63);
    if (len > 0)
      packetBuffer[len] = '\0';

    // FILTRO DE LA PUERTA
    if (strncmp(packetBuffer, "PUERTA_ABRIR", 12) == 0) {
      disparoAbanico("ABRIR");
    } else if (strncmp(packetBuffer, "PUERTA_CERRAR", 13) == 0) {
      disparoAbanico("CERRAR");
    }
    // LECTURA DE TELEMETRÍA (TANQUE + SERVO)
    else {
      // Leemos 3 valores: Izquierda, Derecha, Servo
      sscanf(packetBuffer, "%d,%d,%d", &target_l, &target_r, &target_pinza);
    }
  }

  // BUCLE FÍSICO (10 ms)
  if (millis() - last_millis >= 10) {
    last_millis = millis();

    suavizar_rueda(cur_l, target_l);
    suavizar_rueda(cur_r, target_r);

    aplicarMotor(1, cur_l); // Izquierdo
    aplicarMotor(2, cur_r); // Derecho

    aplicarPinza(target_pinza);
  }
}

// =========================================================
void disparoAbanico(const char *mensaje) {
  for (byte i = 2; i <= 5; i++) {
    IPAddress ipDestino(192, 168, 4, i);
    Udp.beginPacket(ipDestino, puertoPuerta);
    Udp.write(mensaje);
    Udp.endPacket();
    delay(20);
  }
}

// =========================================================
void aplicarPinza(int estado) {
  if (estado == 1) {
    angulo_gancho += 3;
    if (angulo_gancho > 180)
      angulo_gancho = 180;
  } else if (estado == -1) {
    angulo_gancho -= 3;
    if (angulo_gancho < 0)
      angulo_gancho = 0;
  }
  servoGancho.write(angulo_gancho);
}

void suavizar_rueda(int &current, int target) {
  if (target == 0 || (current > 0 && target < 0) ||
      (current < 0 && target > 0)) {
    current = target;
    return;
  }
  if (abs(target) < abs(current)) {
    current = target;
    return;
  }

  if (current < target) {
    current += 8;
    if (current > target)
      current = target;
  } else if (current > target) {
    current -= 8;
    if (current < target)
      current = target;
  }
}

void aplicarMotor(int motor, int velocidad) {
  bool hacia_adelante = (velocidad > 0);
  int pwm = abs(velocidad);
  if (pwm > 255)
    pwm = 255;

  switch (motor) {
  case 1: // Motores Izquierdos (BK3/BK4)
    if (pwm == 0) {
      digitalWrite(LeftMotorDirPin1, LOW);
      digitalWrite(LeftMotorDirPin2, LOW);
    } else {
      digitalWrite(LeftMotorDirPin1, hacia_adelante ? HIGH : LOW);
      digitalWrite(LeftMotorDirPin2, hacia_adelante ? LOW : HIGH);
    }
    analogWrite(speedPinL, pwm);
    break;
  case 2: // Motores Derechos (AK3/AK4)
    if (pwm == 0) {
      digitalWrite(RightMotorDirPin1, LOW);
      digitalWrite(RightMotorDirPin2, LOW);
    } else {
      digitalWrite(RightMotorDirPin1, hacia_adelante ? LOW : HIGH);
      digitalWrite(RightMotorDirPin2, hacia_adelante ? HIGH : LOW);
    }
    analogWrite(speedPinR, pwm);
    break;
  }
}

void apagar_motores() {
  target_l = 0;
  target_r = 0;
  aplicarMotor(1, 0);
  aplicarMotor(2, 0);
}
