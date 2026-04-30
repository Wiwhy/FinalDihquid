// Version 3.5 (Dual Mode con Booleano)
// ============================================================
//  TIRABOLOS / SUMO - Robot Multi-Pista
//  BUSCAR -> ATACAR -> RETROCEDER (loop infinito)
//
//  v3.5:
//  - Control de modo cambiado a una variable bool simple.
//  - Variables dinámicas asignadas automáticamente en el setup().
// ============================================================


// ************************************************************
//  SECCION 1: MODO DE COMPETICION Y CONFIGURACION
// ************************************************************

// Cambia esta variable a true o false según lo que vayas a jugar:
// true  = MODO SUMO
// false = MODO TIRABOLOS
bool MODO_SUMO = true; 

// --- Variables dependientes del modo (se configuran solas en el setup) ---
bool INICIO_RETROCESO;
int  DIST_DETECCION;
int  TIEMPO_INICIO_AVANCE;
int  TIEMPO_RETROCESO;
int  VEL_BUSQUEDA;
int  TIEMPO_GIRO;
int  PAUSA_PRE_MEDIR;
unsigned long US_TIMEOUT;

// --- Configuraciones Generales Compartidas (Estas no cambian) ---

// Velocidades de la maniobra de inicio
const int VEL_INICIO_RETRO     = 80;  
const int VEL_INICIO_AVANCE    = 200; 

// Velocidades de movimiento compartidas
const int VEL_ATAQUE           = 200; 
const int VEL_RETROCESO        = 190; 

// Tiempos compartidos
const unsigned long TIMEOUT_BUSQUEDA = 4500UL; // ms: si no detecta, avanza recto
const int DELAY_PRE_ATAQUE           = 0;      // ms quieto antes de atacar
const int DELAY_POST_RETRO           = 200;    // ms quieto antes de volver a buscar


// ************************************************************
//  SECCION 2: PINES
// ************************************************************

#define speedPinR          9
#define RightMotorDirPin1  22
#define RightMotorDirPin2  24
#define LeftMotorDirPin1   26
#define LeftMotorDirPin2   28
#define speedPinL          10

#define speedPinRB         11
#define RightMotorDirPin1B  5
#define RightMotorDirPin2B  6
#define LeftMotorDirPin1B   7
#define LeftMotorDirPin2B   8
#define speedPinLB         12

#define TRIG  30
#define ECHO  31

#define S1  A4
#define S2  A3
#define S3  A2
#define S4  A1
#define S5  A0

#define ST1  A8
#define ST2  A9
#define ST3  A10
#define ST4  A11
#define ST5  A12


// ************************************************************
//  SECCION 3: ESTADOS Y VARIABLES GLOBALES
// ************************************************************

enum Estado { BUSCAR, ATACAR, RETROCEDER };
Estado        estado      = BUSCAR;
unsigned long tRetroIni   = 0;       
unsigned long tBuscarIni  = 0;       


// ************************************************************
//  SECCION 4: SETUP
// ************************************************************

void setup() {
  Serial.begin(9600);

  // --- CARGAR PERFIL SEGUN EL MODO ELEGIDO ---
  if (MODO_SUMO == true) {
    // Perfil SUMO (Diametro 1.1m | Objetivo Grande)
    INICIO_RETROCESO     = true; 
    DIST_DETECCION       = 100;
    TIEMPO_INICIO_AVANCE = 530;
    TIEMPO_RETROCESO     = 545;
    VEL_BUSQUEDA         = 160; 
    TIEMPO_GIRO          = 20;  
    PAUSA_PRE_MEDIR      = 30;  
    US_TIMEOUT           = 6000UL; 
    Serial.println(F("=== MODO: SUMO ==="));
  } else {
    // Perfil TIRABOLOS (Perimetro 2m | Objetivo Estrecho)
    INICIO_RETROCESO     = false; 
    DIST_DETECCION       = 200;
    TIEMPO_INICIO_AVANCE = 306;
    TIEMPO_RETROCESO     = 315;
    VEL_BUSQUEDA         = 120; 
    TIEMPO_GIRO          = 45;  
    PAUSA_PRE_MEDIR      = 60;  
    US_TIMEOUT           = 15000UL; 
    Serial.println(F("=== MODO: TIRABOLOS ==="));
  }

  // --- Configurar pines ---
  pinMode(RightMotorDirPin1,  OUTPUT); pinMode(RightMotorDirPin2,  OUTPUT);
  pinMode(LeftMotorDirPin1,   OUTPUT); pinMode(LeftMotorDirPin2,   OUTPUT);
  pinMode(speedPinR,          OUTPUT); pinMode(speedPinL,          OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT); pinMode(RightMotorDirPin2B, OUTPUT);
  pinMode(LeftMotorDirPin1B,  OUTPUT); pinMode(LeftMotorDirPin2B,  OUTPUT);
  pinMode(speedPinRB,         OUTPUT); pinMode(speedPinLB,         OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT);
  pinMode(S4, INPUT); pinMode(S5, INPUT);

  pinMode(ST1, INPUT); pinMode(ST2, INPUT); pinMode(ST3, INPUT);
  pinMode(ST4, INPUT); pinMode(ST5, INPUT);

  parar();

  if (INICIO_RETROCESO) {
    maniobraInicio();
  }

  tBuscarIni = millis();
}


// ************************************************************
//  SECCION 5: MANIOBRA DE INICIO
// ************************************************************

void maniobraInicio() {
  velocidad(VEL_INICIO_RETRO);
  retroceder();
  while (!hayLineaTrasera()) { delay(5); }

  parar();
  delay(200);  

  velocidad(VEL_INICIO_AVANCE);
  avanzar();
  delay(TIEMPO_INICIO_AVANCE);

  parar();
  delay(300);  
}


// ************************************************************
//  SECCION 6: LOOP PRINCIPAL
// ************************************************************

void loop() {
  bool linea = hayLinea();

  switch (estado) {

    // --------------------------------------------------------
    //  ESTADO: BUSCAR
    // --------------------------------------------------------
    case BUSCAR:
      if (linea) {
        parar();
        estado    = RETROCEDER;
        tRetroIni = millis();
        break;
      }

      if (millis() - tBuscarIni >= TIMEOUT_BUSQUEDA) {
        estado = ATACAR;
        break;
      }

      velocidad(VEL_BUSQUEDA);
      girar();
      delay(TIEMPO_GIRO);

      {
        parar();
        delay(PAUSA_PRE_MEDIR);
        int d = medirMediana();
        Serial.print(F("US: ")); Serial.print(d); Serial.println(F(" cm"));

        if (d > 0 && d <= DIST_DETECCION) {
          delay(DELAY_PRE_ATAQUE);
          estado = ATACAR;
        }
      }
      break;

    // --------------------------------------------------------
    //  ESTADO: ATACAR
    // --------------------------------------------------------
    case ATACAR:
      if (linea) {
        parar();
        estado    = RETROCEDER;
        tRetroIni = millis();
        break;
      }

      velocidad(VEL_ATAQUE);
      avanzar();
      break;

    // --------------------------------------------------------
    //  ESTADO: RETROCEDER
    // --------------------------------------------------------
    case RETROCEDER:
      if (millis() - tRetroIni < (unsigned long)TIEMPO_RETROCESO) {
        velocidad(VEL_RETROCESO);
        retroceder();
      } else {
        parar();
        delay(DELAY_POST_RETRO); 

        medirMediana(); // Limpieza eco fantasma

        estado = BUSCAR;
        tBuscarIni = millis();  
      }
      break;
  }
}


// ************************************************************
//  SECCION 7: ULTRASONIDO
// ************************************************************

int medir() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  
  long us = pulseIn(ECHO, HIGH, US_TIMEOUT); 
  
  if (us == 0) return 999;        
  int cm = (int)(us / 58);
  if (cm < 2) return 999;          
  return cm;
}

int medirMediana() {
  int a = medir(); delayMicroseconds(500);
  int b = medir(); delayMicroseconds(500);
  int c = medir();
  
  if (a > b) { int t = a; a = b; b = t; }
  if (b > c) { int t = b; b = c; c = t; }
  if (a > b) { int t = a; a = b; b = t; }
  
  return b;  
}


// ************************************************************
//  SECCION 8: SENSORES DE LINEA
// ************************************************************

bool hayLinea() {
  return (!digitalRead(S1) || !digitalRead(S2) || !digitalRead(S3) ||
          !digitalRead(S4) || !digitalRead(S5));
}

bool hayLineaTrasera() {
  return (!digitalRead(ST1) || !digitalRead(ST2) || !digitalRead(ST3) ||
          !digitalRead(ST4) || !digitalRead(ST5));
}


// ************************************************************
//  SECCION 10: MOVIMIENTO DE MOTORES
// ************************************************************

void velocidad(int v) {
  analogWrite(speedPinL,  v); analogWrite(speedPinR,  v);
  analogWrite(speedPinLB, v); analogWrite(speedPinRB, v);
}

void avanzar()    { FRf(); FLf(); RRf(); RLf(); }  
void retroceder() { FRb(); FLb(); RRb(); RLb(); }  
void girar()      { FRb(); FLf(); RRb(); RLf(); }  

void parar() {
  digitalWrite(RightMotorDirPin1,  LOW); digitalWrite(RightMotorDirPin2,  LOW);
  digitalWrite(LeftMotorDirPin1,   LOW); digitalWrite(LeftMotorDirPin2,   LOW);
  digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B,  LOW); digitalWrite(LeftMotorDirPin2B,  LOW);
  velocidad(0);
}

void FRf() { digitalWrite(RightMotorDirPin1,  LOW);  digitalWrite(RightMotorDirPin2,  HIGH); }
void FRb() { digitalWrite(RightMotorDirPin1,  HIGH); digitalWrite(RightMotorDirPin2,  LOW);  }
void FLf() { digitalWrite(LeftMotorDirPin1,   LOW);  digitalWrite(LeftMotorDirPin2,   HIGH); }
void FLb() { digitalWrite(LeftMotorDirPin1,   HIGH); digitalWrite(LeftMotorDirPin2,   LOW);  }
void RRf() { digitalWrite(RightMotorDirPin1B, LOW);  digitalWrite(RightMotorDirPin2B, HIGH); }
void RRb() { digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW);  }
void RLf() { digitalWrite(LeftMotorDirPin1B,  LOW);  digitalWrite(LeftMotorDirPin2B,  HIGH); }
void RLb() { digitalWrite(LeftMotorDirPin1B,  HIGH); digitalWrite(LeftMotorDirPin2B,  LOW);  }