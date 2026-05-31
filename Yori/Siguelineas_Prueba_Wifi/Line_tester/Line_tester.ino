#include "WiFiEsp.h"
#include "WiFiEspUdp.h"

// --- CONFIGURACIÓN WI-FI ---
char ssid[] = "ROBOT_COMPETICION";
char pass[] = "12345678";
int status = WL_IDLE_STATUS;
unsigned int localPort = 8080;
WiFiEspUDP Udp;

// ⚠️ MODO DEL ROBOT (0 = Frenado, 1 = Corriendo, 2 = Girando Derecha, 3 =
// Adelante)
int modo_robot = 0;
unsigned long ultimo_wifi = 0;

#define MAX_SPEED 255
int velocidad_base = 150;
#define MIN_SPEED 50

// ================= FUZZY PID Y MULTIPLICADORES =================
float Kp_base = 0.2;
float Kd_base = 5.0;
float mult_verde_p = 0.4;
float mult_verde_d = 1.0;
float mult_ama_p = 1.0;
float mult_ama_d = 2.5;
float mult_rojo_p = 3.2;
float mult_rojo_d = 1.7;

float Kp_actual = 0;
float Kd_actual = 0;
float P = 0, D = 0, lastError = 0;
float filteredD = 0;
#define D_FILTER 0.3

// ================= VARIABLES MODO RESCATE (BÚSQUEDA) =================
unsigned long tiempo_perdido = 0;
bool linea_perdida = false;
int direccion_busqueda = 1;
bool direccion_cambiada = false;
int tiempo_barrido = 500; // 🚨 NUEVO: Variable controlada por Wi-Fi

// --- PINES DE MOTORES ---
#define speedPinR 9
#define RightMotorDirPin1 22
#define RightMotorDirPin2 24
#define LeftMotorDirPin1 26
#define LeftMotorDirPin2 28
#define speedPinL 10
#define speedPinRB 11
#define RightMotorDirPin1B 5
#define RightMotorDirPin2B 6
#define LeftMotorDirPin1B 7
#define LeftMotorDirPin2B 8
#define speedPinLB 12

// --- PINES DE SENSORES ---
#define sensor1 A4
#define sensor2 A3
#define sensor3 A2
#define sensor4 A1
#define sensor5 A0

/* ============ Control de Motores Básico ============ */
void FR_fwd(int speed) {
  digitalWrite(RightMotorDirPin1, LOW);
  digitalWrite(RightMotorDirPin2, HIGH);
  analogWrite(speedPinR, speed);
}
void FR_bck(int speed) {
  digitalWrite(RightMotorDirPin1, HIGH);
  digitalWrite(RightMotorDirPin2, LOW);
  analogWrite(speedPinR, speed);
}
void FL_fwd(int speed) {
  digitalWrite(LeftMotorDirPin1, LOW);
  digitalWrite(LeftMotorDirPin2, HIGH);
  analogWrite(speedPinL, speed);
}
void FL_bck(int speed) {
  digitalWrite(LeftMotorDirPin1, HIGH);
  digitalWrite(LeftMotorDirPin2, LOW);
  analogWrite(speedPinL, speed);
}
void RR_fwd(int speed) {
  digitalWrite(RightMotorDirPin1B, LOW);
  digitalWrite(RightMotorDirPin2B, HIGH);
  analogWrite(speedPinRB, speed);
}
void RR_bck(int speed) {
  digitalWrite(RightMotorDirPin1B, HIGH);
  digitalWrite(RightMotorDirPin2B, LOW);
  analogWrite(speedPinRB, speed);
}
void RL_fwd(int speed) {
  digitalWrite(LeftMotorDirPin1B, LOW);
  digitalWrite(LeftMotorDirPin2B, HIGH);
  analogWrite(speedPinLB, speed);
}
void RL_bck(int speed) {
  digitalWrite(LeftMotorDirPin1B, HIGH);
  digitalWrite(LeftMotorDirPin2B, LOW);
  analogWrite(speedPinLB, speed);
}

void stop_bot() {
  analogWrite(speedPinLB, 0);
  analogWrite(speedPinRB, 0);
  analogWrite(speedPinL, 0);
  analogWrite(speedPinR, 0);
  digitalWrite(RightMotorDirPin1B, LOW);
  digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B, LOW);
  digitalWrite(LeftMotorDirPin2B, LOW);
  digitalWrite(RightMotorDirPin1, LOW);
  digitalWrite(RightMotorDirPin2, LOW);
  digitalWrite(LeftMotorDirPin1, LOW);
  digitalWrite(LeftMotorDirPin2, LOW);
}

void init_GPIO() {
  pinMode(RightMotorDirPin1, OUTPUT);
  pinMode(RightMotorDirPin2, OUTPUT);
  pinMode(speedPinL, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT);
  pinMode(LeftMotorDirPin2, OUTPUT);
  pinMode(speedPinR, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT);
  pinMode(RightMotorDirPin2B, OUTPUT);
  pinMode(speedPinLB, OUTPUT);
  pinMode(LeftMotorDirPin1B, OUTPUT);
  pinMode(LeftMotorDirPin2B, OUTPUT);
  pinMode(speedPinRB, OUTPUT);

  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT);
  pinMode(sensor5, INPUT);
  stop_bot();
}

void setMotors(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -MAX_SPEED, MAX_SPEED);
  rightSpeed = constrain(rightSpeed, -MAX_SPEED, MAX_SPEED);
  if (leftSpeed >= 0) {
    FL_fwd(leftSpeed);
    RL_fwd(leftSpeed);
  } else {
    FL_bck(-leftSpeed);
    RL_bck(-leftSpeed);
  }

  if (rightSpeed >= 0) {
    FR_fwd(rightSpeed);
    RR_fwd(rightSpeed);
  } else {
    FR_bck(-rightSpeed);
    RR_bck(-rightSpeed);
  }
}

void setup() {
  init_GPIO();
  Serial.begin(115200);
  Serial1.begin(115200);

  Serial.println("Iniciando Wi-Fi...");
  WiFi.init(&Serial1);
  WiFi.beginAP(ssid, 10, pass, ENC_TYPE_WPA2_PSK);
  Udp.begin(localPort);

  Serial.println("¡Modo DIOS listo! Red: ROBOT_COMPETICION");
}

void loop() {
  if (millis() - ultimo_wifi > 50) {
    revisarTelemetria();
    ultimo_wifi = millis();
  }

  if (modo_robot == 1) {
    tracking();
  } else if (modo_robot == 2) {
    setMotors(120, -120);
  } else if (modo_robot == 3) {
    setMotors(velocidad_base, velocidad_base);
  } else {
    stop_bot();
  }
}

void revisarTelemetria() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    char packetBuffer[128];
    int len = Udp.read(packetBuffer, 127);
    if (len > 0) {
      packetBuffer[len] = '\0';
      if (packetBuffer[0] == 'F') {
        char *token = strtok(packetBuffer, ",");
        if (token != NULL) {
          // 🚨 NUEVO ORDEN DE LECTURA: Velocidad, Tiempo_Barrido, PID...
          token = strtok(NULL, ",");
          if (token)
            velocidad_base = atoi(token);
          token = strtok(NULL, ",");
          if (token)
            tiempo_barrido = atoi(token);
          token = strtok(NULL, ",");
          if (token)
            Kp_base = atof(token);
          token = strtok(NULL, ",");
          if (token)
            Kd_base = atof(token);
          token = strtok(NULL, ",");
          if (token)
            mult_verde_p = atof(token);
          token = strtok(NULL, ",");
          if (token)
            mult_verde_d = atof(token);
          token = strtok(NULL, ",");
          if (token)
            mult_ama_p = atof(token);
          token = strtok(NULL, ",");
          if (token)
            mult_ama_d = atof(token);
          token = strtok(NULL, ",");
          if (token)
            mult_rojo_p = atof(token);
          token = strtok(NULL, ",");
          if (token)
            mult_rojo_d = atof(token);
          Serial.println("¡Cerebro Fuzzy, Velocidad y Barrido Actualizados!");
        }
      } else if (packetBuffer[0] == 'S') {
        modo_robot = 0;
      } else if (packetBuffer[0] == 'R') {
        modo_robot = 1;
      } else if (packetBuffer[0] == 'D') {
        modo_robot = 2;
      } else if (packetBuffer[0] == 'A') {
        modo_robot = 3;
      }
    }
  }
}

void calcularFuzzyPID(float errorAbsoluto) {
  if (errorAbsoluto < 60) {
    Kp_actual = Kp_base * mult_verde_p;
    Kd_actual = Kd_base * mult_verde_d;
  } else if (errorAbsoluto >= 60 && errorAbsoluto < 150) {
    Kp_actual = Kp_base * mult_ama_p;
    Kd_actual = Kd_base * mult_ama_d;
  } else {
    Kp_actual = Kp_base * mult_rojo_p;
    Kd_actual = Kd_base * mult_rojo_d;
  }
}

int calcularError(int s0, int s1, int s2, int s3, int s4, int suma) {
  return (s0 * -250 + s1 * -100 + s2 * 0 + s3 * 100 + s4 * 250) / suma;
}

void tracking() {
  int s0 = !digitalRead(sensor1);
  int s1 = !digitalRead(sensor2);
  int s2 = !digitalRead(sensor3);
  int s3 = !digitalRead(sensor4);
  int s4 = !digitalRead(sensor5);
  int suma = s0 + s1 + s2 + s3 + s4;

  // =======================================================
  // 🚨 MODO RESCATE: BARRIDO INFINITO MEJORADO 🚨
  // =======================================================
  if (suma == 0) {
    if (!linea_perdida) {
      // Acaba de perder la línea
      linea_perdida = true;
      direccion_cambiada = false;
      tiempo_perdido = millis();
      direccion_busqueda = (lastError > 0) ? 1 : -1;
    }
    // 🚨 AHORA UTILIZAMOS LA VARIABLE tiempo_barrido AQUÍ
    else if (!direccion_cambiada &&
             (millis() - tiempo_perdido > tiempo_barrido)) {
      direccion_busqueda = -direccion_busqueda;
      direccion_cambiada = true;
    }

    setMotors(180 * direccion_busqueda, -180 * direccion_busqueda);
    return;
  } else {
    linea_perdida = false;
  }

  float error = calcularError(s0, s1, s2, s3, s4, suma);
  float absError = abs(error);

  int baseSpeed;
  if (absError < 20) {
    baseSpeed = velocidad_base;
  } else {
    baseSpeed = map(absError, 20, 250, velocidad_base, MIN_SPEED);
    baseSpeed = constrain(baseSpeed, MIN_SPEED, velocidad_base);
  }

  calcularFuzzyPID(absError);

  P = error;
  D = error - lastError;
  lastError = error;
  filteredD = D_FILTER * D + (1.0 - D_FILTER) * filteredD;
  float correction = (Kp_actual * P) + (Kd_actual * filteredD);

  int leftSpeed = baseSpeed + correction;
  int rightSpeed = baseSpeed - correction;

  setMotors(leftSpeed, rightSpeed);
}