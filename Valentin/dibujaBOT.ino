// Pines de los motores (L298N)
#define speedPinR 9
#define RightMotorDirPin1  22
#define RightMotorDirPin2  24
#define LeftMotorDirPin1  26
#define LeftMotorDirPin2  28
#define speedPinL 10

#define speedPinRB 11
#define RightMotorDirPin1B  5
#define RightMotorDirPin2B 6
#define LeftMotorDirPin1B 7
#define LeftMotorDirPin2B 8
#define speedPinLB 12

// ========================================================
// ======== CONFIGURACIÓN PRINCIPAL DE FIGURAS ============
// ========================================================

// 1. SELECTOR DE FIGURA (1 = Triángulo | 2 = Cuadrado | 3 = Rectángulo)
int figura_a_dibujar = 1; 

// 2. Velocidad General del Robot (0 a 255)
int velocidad_movimiento = 180;

// ========================================================
// ================= CALIBRACIÓN FÍSICA ===================
// ========================================================
// ¿Cuántos milisegundos tarda en hacer 1 CM real? 
// Ajusta estos 3 valores por separado si ves que patina o se queda corto.

float ms_cm_frente_atras = 35.0; // Calibración para ir recto
float ms_cm_diagonal     = 48.0; // Calibración para diagonales (Triángulo)
float ms_cm_lateral      = 45.0; // Calibración para ir de lado puro (Cuadr/Rect)

// ========================================================
// ============= MEDIDAS INDEPENDIENTES (CM) ==============
// ========================================================

// --- TRIÁNGULO ---
float tri_lado_recto    = 30.0; // Hacia adelante
float tri_diagonal_der  = 30.0; // Hacia abajo-derecha
float tri_diagonal_izq  = 30.0; // Hacia abajo-izquierda

// --- CUADRADO ---
float cuad_frente_atras = 20.0; // Lados que van hacia adelante y atrás
float cuad_lados        = 20.0; // Lados que van hacia izquierda y derecha

// --- RECTÁNGULO ---
float rect_frente_atras = 30.0; // Lados largos
float rect_lados        = 10.0; // Lados cortos

// ========================================================

void setup() {
  pinMode(RightMotorDirPin1, OUTPUT); pinMode(RightMotorDirPin2, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT); pinMode(LeftMotorDirPin2, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT); pinMode(RightMotorDirPin2B, OUTPUT);
  pinMode(LeftMotorDirPin1B, OUTPUT); pinMode(LeftMotorDirPin2B, OUTPUT);

  pinMode(speedPinL, OUTPUT); pinMode(speedPinR, OUTPUT);
  pinMode(speedPinLB, OUTPUT); pinMode(speedPinRB, OUTPUT);

  stop_Stop();
  delay(5000); // 5 segundos para ponerlo en el suelo

  if (figura_a_dibujar == 1) { dibujarTriangulo(); } 
  else if (figura_a_dibujar == 2) { dibujarCuadrado(); } 
  else if (figura_a_dibujar == 3) { dibujarRectangulo(); }
}

void loop() {
  // Vacío. Dibuja una vez y termina.
}

// ==========================================
// RUTINAS DE DIBUJO HOLONÓMICO
// ==========================================

void dibujarTriangulo() {
  int v_fuerte = velocidad_movimiento;
  int v_suave = velocidad_movimiento * 0.268; // Proporción matemática exacta para 120 grados

  // Calculamos los tiempos usando las calibraciones y medidas separadas
  int t_lado_1 = tri_lado_recto * ms_cm_frente_atras;
  int t_lado_2 = tri_diagonal_der * ms_cm_diagonal; 
  int t_lado_3 = tri_diagonal_izq * ms_cm_diagonal;

  // LADO 1: ADELANTE
  setMotors(v_fuerte, v_fuerte, v_fuerte, v_fuerte);
  delay(t_lado_1);
  stop_Stop(); delay(300); 

  // LADO 2: DIAGONAL ABAJO-DERECHA
  setMotors(v_suave, -v_fuerte, -v_fuerte, v_suave);
  delay(t_lado_2);
  stop_Stop(); delay(300);

  // LADO 3: DIAGONAL ABAJO-IZQUIERDA
  setMotors(-v_fuerte, v_suave, v_suave, -v_fuerte);
  delay(t_lado_3);
  stop_Stop();
}

void dibujarCuadrado() {
  int v = velocidad_movimiento;
  
  int t_frente = cuad_frente_atras * ms_cm_frente_atras;
  int t_lados = cuad_lados * ms_cm_lateral;

  // 1: ADELANTE
  setMotors(v, v, v, v);
  delay(t_frente); stop_Stop(); delay(300);

  // 2: DERECHA (Strafe Right)
  setMotors(v, -v, -v, v);
  delay(t_lados); stop_Stop(); delay(300);

  // 3: ATRÁS
  setMotors(-v, -v, -v, -v);
  delay(t_frente); stop_Stop(); delay(300);

  // 4: IZQUIERDA (Strafe Left)
  setMotors(-v, v, v, -v);
  delay(t_lados); stop_Stop();
}

void dibujarRectangulo() {
  int v = velocidad_movimiento;
  
  int t_frente = rect_frente_atras * ms_cm_frente_atras;
  int t_lados = rect_lados * ms_cm_lateral;

  // 1: ADELANTE
  setMotors(v, v, v, v);
  delay(t_frente); stop_Stop(); delay(300);

  // 2: DERECHA
  setMotors(v, -v, -v, v);
  delay(t_lados); stop_Stop(); delay(300);

  // 3: ATRÁS
  setMotors(-v, -v, -v, -v);
  delay(t_frente); stop_Stop(); delay(300);

  // 4: IZQUIERDA
  setMotors(-v, v, v, -v);
  delay(t_lados); stop_Stop();
}

// ==========================================
// CONTROL DE MOTORES UNIFICADO
// ==========================================

void setMotors(int FL, int FR, int RL, int RR) {
  // Front Left
  if (FL >= 0) {
    digitalWrite(LeftMotorDirPin1, LOW); digitalWrite(LeftMotorDirPin2, HIGH); analogWrite(speedPinL, FL);
  } else {
    digitalWrite(LeftMotorDirPin1, HIGH); digitalWrite(LeftMotorDirPin2, LOW); analogWrite(speedPinL, -FL);
  }
  // Front Right
  if (FR >= 0) {
    digitalWrite(RightMotorDirPin1, LOW); digitalWrite(RightMotorDirPin2, HIGH); analogWrite(speedPinR, FR);
  } else {
    digitalWrite(RightMotorDirPin1, HIGH); digitalWrite(RightMotorDirPin2, LOW); analogWrite(speedPinR, -FR);
  }
  // Rear Left
  if (RL >= 0) {
    digitalWrite(LeftMotorDirPin1B, LOW); digitalWrite(LeftMotorDirPin2B, HIGH); analogWrite(speedPinLB, RL);
  } else {
    digitalWrite(LeftMotorDirPin1B, HIGH); digitalWrite(LeftMotorDirPin2B, LOW); analogWrite(speedPinLB, -RL);
  }
  // Rear Right
  if (RR >= 0) {
    digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, HIGH); analogWrite(speedPinRB, RR);
  } else {
    digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW); analogWrite(speedPinRB, -RR);
  }
}

void stop_Stop() {
  setMotors(0, 0, 0, 0);
}0.