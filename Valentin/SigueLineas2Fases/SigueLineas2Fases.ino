// ============================================================
//  SIGUE LÍNEAS — 2 ETAPAS CON TIMER
//  Etapa 1 → Curvas cerradas (PID agresivo, velocidad baja)
//  Etapa 2 → Rectas        (PID suave,     velocidad alta)
// ============================================================

// ──────────────────────────────────────────────────────────────
//  ★  TIMER DE ETAPAS  (modifica estos valores libremente)
// ──────────────────────────────────────────────────────────────
#define DURACION_ETAPA1_MS  9500  // Tiempo en ms antes de pasar a Etapa 2 (15 s)

// ──────────────────────────────────────────────────────────────
//  ★  VELOCIDADES — ETAPA 1 (curvas cerradas)
// ──────────────────────────────────────────────────────────────
#define MAX_SPEED_E1   240   // Velocidad máxima permitida (PWM 0-255)
#define BASE_SPEED_E1  230   // Velocidad en recta perfecta
#define MIN_SPEED_E1     0   // Velocidad mínima en curva extrema (pivote)

// ──────────────────────────────────────────────────────────────
//  ★  VELOCIDADES — ETAPA 2 (rectas)
// ──────────────────────────────────────────────────────────────
#define MAX_SPEED_E2   130   // Velocidad máxima permitida (PWM 0-255)
#define BASE_SPEED_E2  110   // Velocidad en recta perfecta
#define MIN_SPEED_E2     0   // Velocidad mínima si aparece alguna curva suave

// ──────────────────────────────────────────────────────────────
//  ★  PARÁMETROS PID — ETAPA 1 (curvas, más agresivo)
// ──────────────────────────────────────────────────────────────
#define KP_E1    0.5f    // Proporcional
#define KI_E1    0.0f    // Integral
#define KD_E1    4.0f    // Derivativo
#define D_FILTER_E1  0.3f   // Suavizado derivativo (0=máximo filtro, 1=sin filtro)

// ──────────────────────────────────────────────────────────────
//  ★  PARÁMETROS PID — ETAPA 2 (rectas, más suave)
// ──────────────────────────────────────────────────────────────
#define KP_E2    0.4f   // Proporcional
#define KI_E2    0.0f    // Integral
#define KD_E2    2f    // Derivativo
#define D_FILTER_E2  0.2f   // Suavizado derivativo

// ──────────────────────────────────────────────────────────────
//  Límite anti-windup del integrador (mismo para ambas etapas)
// ──────────────────────────────────────────────────────────────
#define I_MAX  2000

// ──────────────────────────────────────────────────────────────
//  Pines de motores
// ──────────────────────────────────────────────────────────────
#define speedPinR        9
#define RightMotorDirPin1 22
#define RightMotorDirPin2 24
#define LeftMotorDirPin1  26
#define LeftMotorDirPin2  28
#define speedPinL        10

#define speedPinRB       11
#define RightMotorDirPin1B 5
#define RightMotorDirPin2B 6
#define LeftMotorDirPin1B  7
#define LeftMotorDirPin2B  8
#define speedPinLB       12

// ──────────────────────────────────────────────────────────────
//  Pines de sensores (A4 = izq extremo … A0 = der extremo)
// ──────────────────────────────────────────────────────────────
#define sensor1 A4   // Izquierdo extremo
#define sensor2 A3   // Izquierdo
#define sensor3 A2   // Centro
#define sensor4 A1   // Derecho
#define sensor5 A0   // Derecho extremo

// ──────────────────────────────────────────────────────────────
//  Variables de estado PID
// ──────────────────────────────────────────────────────────────
float P = 0, I = 0, D = 0;
float lastError  = 0;
float filteredD  = 0;

// ──────────────────────────────────────────────────────────────
//  Variables del timer de etapas
// ──────────────────────────────────────────────────────────────
unsigned long timerInicio = 0;   // Marca de tiempo al arrancar la etapa actual
int etapaActual = 1;             // 1 = curvas, 2 = rectas

// =============================================================
//  CONTROL DE MOTORES
// =============================================================
void FR_fwd(int s) { digitalWrite(RightMotorDirPin1, LOW);  digitalWrite(RightMotorDirPin2, HIGH); analogWrite(speedPinR, s); }
void FR_bck(int s) { digitalWrite(RightMotorDirPin1, HIGH); digitalWrite(RightMotorDirPin2, LOW);  analogWrite(speedPinR, s); }
void FL_fwd(int s) { digitalWrite(LeftMotorDirPin1,  LOW);  digitalWrite(LeftMotorDirPin2,  HIGH); analogWrite(speedPinL, s); }
void FL_bck(int s) { digitalWrite(LeftMotorDirPin1,  HIGH); digitalWrite(LeftMotorDirPin2,  LOW);  analogWrite(speedPinL, s); }
void RR_fwd(int s) { digitalWrite(RightMotorDirPin1B, LOW);  digitalWrite(RightMotorDirPin2B, HIGH); analogWrite(speedPinRB, s); }
void RR_bck(int s) { digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW);  analogWrite(speedPinRB, s); }
void RL_fwd(int s) { digitalWrite(LeftMotorDirPin1B,  LOW);  digitalWrite(LeftMotorDirPin2B,  HIGH); analogWrite(speedPinLB, s); }
void RL_bck(int s) { digitalWrite(LeftMotorDirPin1B,  HIGH); digitalWrite(LeftMotorDirPin2B,  LOW);  analogWrite(speedPinLB, s); }

void stop_bot() {
  analogWrite(speedPinLB, 0); analogWrite(speedPinRB, 0);
  analogWrite(speedPinL,  0); analogWrite(speedPinR,  0);
  digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B,  LOW); digitalWrite(LeftMotorDirPin2B,  LOW);
  digitalWrite(RightMotorDirPin1,  LOW); digitalWrite(RightMotorDirPin2,  LOW);
  digitalWrite(LeftMotorDirPin1,   LOW); digitalWrite(LeftMotorDirPin2,   LOW);
  delay(40);
}

void setMotors(int leftSpeed, int rightSpeed, int maxSpd) {
  leftSpeed  = constrain(leftSpeed,  -maxSpd, maxSpd);
  rightSpeed = constrain(rightSpeed, -maxSpd, maxSpd);
  if (leftSpeed  >= 0) { FL_fwd(leftSpeed);  RL_fwd(leftSpeed);  }
  else                 { FL_bck(-leftSpeed); RL_bck(-leftSpeed); }
  if (rightSpeed >= 0) { FR_fwd(rightSpeed); RR_fwd(rightSpeed); }
  else                 { FR_bck(-rightSpeed);RR_bck(-rightSpeed);}
}

// =============================================================
//  GPIO INIT
// =============================================================
void init_GPIO() {
  pinMode(RightMotorDirPin1, OUTPUT); pinMode(RightMotorDirPin2, OUTPUT); pinMode(speedPinL, OUTPUT);
  pinMode(LeftMotorDirPin1,  OUTPUT); pinMode(LeftMotorDirPin2,  OUTPUT); pinMode(speedPinR, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT); pinMode(RightMotorDirPin2B, OUTPUT); pinMode(speedPinLB, OUTPUT);
  pinMode(LeftMotorDirPin1B,  OUTPUT); pinMode(LeftMotorDirPin2B,  OUTPUT); pinMode(speedPinRB, OUTPUT);
  pinMode(sensor1, INPUT); pinMode(sensor2, INPUT); pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT); pinMode(sensor5, INPUT);
  stop_bot();
}

// =============================================================
//  SETUP
// =============================================================
void setup() {
  init_GPIO();
  Serial.begin(9600);
  timerInicio = millis();   // Arranca el timer desde el inicio
  etapaActual = 1;
  Serial.println("=== INICIO ETAPA 1 (CURVAS) ===");
}

// =============================================================
//  GESTIÓN DEL TIMER DE ETAPAS
//  Llama a esta función cada iteración del loop.
//  Cambia etapaActual y resetea el PID al cambiar de etapa.
// =============================================================
void actualizarEtapa() {
  // Solo actúa una vez: cuando se cumple el tiempo en Etapa 1 pasa a Etapa 2
  if (etapaActual == 1 && (millis() - timerInicio) >= DURACION_ETAPA1_MS) {
    etapaActual = 2;
    // Reset del estado PID para evitar saltos al cambiar parámetros
    I = 0; lastError = 0; filteredD = 0;
    Serial.println("=== CAMBIO A ETAPA 2 (RECTAS) ===");
  }
  // En Etapa 2 no hace nada → se queda ahí para siempre
}

// =============================================================
//  CÁLCULO DEL ERROR DE POSICIÓN
// =============================================================
int calcularError(int s0, int s1, int s2, int s3, int s4, int suma) {
  if (suma == 0) {
    // Línea perdida → extrapola en la dirección del último error
    return (lastError > 0) ? 250 : -250;
  }
  // Pesos: -250 (izq extremo) … +250 (der extremo)
  return (s0 * -250 + s1 * -100 + s2 * 0 + s3 * 100 + s4 * 250) / suma;
}

// =============================================================
//  TRACKING  (usa los parámetros de la etapa activa)
// =============================================================
void tracking() {
  // --- Lectura de sensores (activo en LOW → invertimos) ---
  int s0 = !digitalRead(sensor1);
  int s1 = !digitalRead(sensor2);
  int s2 = !digitalRead(sensor3);
  int s3 = !digitalRead(sensor4);
  int s4 = !digitalRead(sensor5);
  int suma = s0 + s1 + s2 + s3 + s4;

  float error    = calcularError(s0, s1, s2, s3, s4, suma);
  float absError = abs(error);

  // --- Selección de parámetros según la etapa activa ---
  float kp, ki, kd, dFilter;
  int   maxSpd, baseSpd, minSpd;

  if (etapaActual == 1) {
    // ── Etapa 1: CURVAS ──
    kp      = KP_E1;
    ki      = KI_E1;
    kd      = KD_E1;
    dFilter = D_FILTER_E1;
    maxSpd  = MAX_SPEED_E1;
    baseSpd = BASE_SPEED_E1;
    minSpd  = MIN_SPEED_E1;
  } else {
    // ── Etapa 2: RECTAS ──
    kp      = KP_E2;
    ki      = KI_E2;
    kd      = KD_E2;
    dFilter = D_FILTER_E2;
    maxSpd  = MAX_SPEED_E2;
    baseSpd = BASE_SPEED_E2;
    minSpd  = MIN_SPEED_E2;
  }

  // --- Velocidad base variable según el error ---
  // Error bajo  → baseSpd (avanza rápido)
  // Error alto  → minSpd  (pivote / giro)
  int baseSpeed;
  if (absError < 20) {
    baseSpeed = baseSpd;
  } else {
    baseSpeed = map((int)absError, 20, 250, baseSpd, minSpd);
    baseSpeed = constrain(baseSpeed, minSpd, baseSpd);
  }

  // --- Cálculo PID ---
  P  = error;
  I += error;
  I  = constrain(I, -I_MAX, I_MAX);
  if (absError < 10) I = 0;          // Reset integral cuando está centrado

  D = error - lastError;
  lastError = error;

  // Filtro low-pass sobre la derivada (suaviza saltos discretos del sensor)
  filteredD = dFilter * D + (1.0f - dFilter) * filteredD;

  float correction = (kp * P) + (ki * I) + (kd * filteredD);

  int leftSpeed  = baseSpeed + correction;
  int rightSpeed = baseSpeed - correction;

  setMotors(leftSpeed, rightSpeed, maxSpd);
}

// =============================================================
//  LOOP PRINCIPAL
// =============================================================
void loop() {
  actualizarEtapa();   // 1. Comprueba y actualiza la etapa según el timer
  tracking();          // 2. Ejecuta el PID de la etapa activa
}