// Version 1.6
// ============================================================
//  TIRABOLOS - Robot Demoledor de Bolos
//  Maquina de Estados: BUSCAR -> ATACAR -> RETROCEDER
//
//  CAMBIOS v1.6:
//  - TIMEOUT_US subido a 6000 us para detectar correctamente 90 cm
//  - PROBLEMA ANTERIOR: pulseIn() tiene ~250 us de overhead (el sensor
//    procesa y lanza los 8 pulsos ultrasonicos antes de medir).
//    Con TIMEOUT=5000 el rango real era solo ~80 cm, no 85-90 cm.
//  - FORMULA CORRECTA: TIMEOUT = (DIST*2/0.034) + 300 us de margen
//    Para 90 cm: 5294 + 300 = 5594 -> usamos 6000 us
// ============================================================

// ----------------------------------------------------------
//  VARIABLES PRINCIPALES - AJUSTAR AQUI
// ----------------------------------------------------------

// --- Multiplicador global de velocidad ---
// Cambia solo este numero para subir o bajar TODAS las velocidades
const double mul_speed = 1.0;     // 1.0 = base | 1.15 = +15% | 0.9 = -10%

// --- Velocidades (se multiplican por mul_speed) ---
const int VEL_BUSQUEDA  = 70  * mul_speed;  // Giro buscando objetivo
const int VEL_ATAQUE    = 175 * mul_speed;  // Avance de ataque (bajado para no pasarse)
const int VEL_RETROCESO = 210 * mul_speed;  // Retroceso hacia el centro

// --- Ultrasonido ---
const int          DIST_DETECCION = 90;     // cm: distancia de deteccion
const int          PULSO_TRIGGER  = 10;     // us del pulso TRIG (min 10 us)
// FORMULA CORRECTA (con overhead del HC-SR04):
//   TIMEOUT = (DIST_cm * 2 / 0.034) + ~300 us overhead de startup
//   90 cm -> 5294 + 300 = 5594 us minimo -> usamos 6000 con margen
//
//   Tabla rapida (ya con overhead incluido):
//     60 cm ->  3840 us    80 cm -> 5020 us
//     70 cm ->  4420 us    90 cm -> 5600 us  <- actual
//    100 cm ->  6180 us   120 cm -> 7360 us
const unsigned long TIMEOUT_US    = 6000;   // us: rango real fiable ~90 cm

// --- Retroceso al centro ---
const int TIEMPO_RETROCESO = 700;  // ms marcha atras para volver al centro
// (Ya no hay giro post-retroceso: BUSCAR ya se encarga de girar)

// --- Busqueda ---
const int TIEMPO_GIRO_BUSQUEDA = 60; // ms de giro entre cada disparo (min 60 ms)

// ----------------------------------------------------------
//  PINES - MOTORES (L298N)
// ----------------------------------------------------------
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

// ----------------------------------------------------------
//  PINES - ULTRASONIDO HC-SR04
// ----------------------------------------------------------
#define Trig_PIN  30
#define Echo_PIN  31

// ----------------------------------------------------------
//  PINES - SENSORES DE LINEA IR (borde del ring)
// ----------------------------------------------------------
#define sensor1  A4
#define sensor2  A3
#define sensor3  A2
#define sensor4  A1
#define sensor5  A0

// ----------------------------------------------------------
//  ESTADOS DE LA MAQUINA
// ----------------------------------------------------------
enum Estado {
  BUSCAR,      // Girar escaneando con ultrasonido
  ATACAR,      // Objeto detectado: avanzar a toda velocidad
  RETROCEDER   // Linea negra detectada: volver al centro
};

// ----------------------------------------------------------
//  VARIABLES GLOBALES
// ----------------------------------------------------------
Estado        estadoActual      = BUSCAR;
unsigned long tiempoRetroInicio = 0;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(9600);

  // Motores delanteros
  pinMode(RightMotorDirPin1, OUTPUT);
  pinMode(RightMotorDirPin2, OUTPUT);
  pinMode(LeftMotorDirPin1,  OUTPUT);
  pinMode(LeftMotorDirPin2,  OUTPUT);
  pinMode(speedPinR,         OUTPUT);
  pinMode(speedPinL,         OUTPUT);

  // Motores traseros
  pinMode(RightMotorDirPin1B, OUTPUT);
  pinMode(RightMotorDirPin2B, OUTPUT);
  pinMode(LeftMotorDirPin1B,  OUTPUT);
  pinMode(LeftMotorDirPin2B,  OUTPUT);
  pinMode(speedPinRB,         OUTPUT);
  pinMode(speedPinLB,         OUTPUT);

  // Ultrasonido
  pinMode(Trig_PIN, OUTPUT);
  pinMode(Echo_PIN, INPUT);
  digitalWrite(Trig_PIN, LOW);

  // Sensores de linea
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT);
  pinMode(sensor5, INPUT);

  stop_Stop();
  Serial.println(F("=== TIRABOLOS v1.2 LISTO ==="));
}

// ============================================================
//  LOOP PRINCIPAL - MAQUINA DE ESTADOS
// ============================================================
void loop() {

  // PRIORIDAD 1: leer linea ANTES que nada (digitalRead ~1us, no bloquea)
  bool lineaDetectada = detectarLinea();

  // PRIORIDAD 2: ultrasonido SOLO en BUSCAR
  // En ATACAR pulseIn bloquea ~5ms -> el robot se pasaria de la linea
  int distancia = 999;
  if (estadoActual == BUSCAR) {
    distancia = medirDistancia();
    Serial.print(F("[BUSCAR] Dist: "));
    Serial.print(distancia);
    Serial.print(F(" cm | Linea: "));
    Serial.println(lineaDetectada ? F("SI") : F("no"));
  }

  // ----------------------------------------------------------
  switch (estadoActual) {

    // ========================================================
    //  ESTADO 1: BUSCAR
    //  Gira y dispara el ultrasonido cada 60 ms.
    //  Con 1 sola lectura positiva pasa a ATACAR (sin esperas).
    // ========================================================
    case BUSCAR:

      // Si estamos sobre la linea de salida, retroceder primero
      if (lineaDetectada) {
        estadoActual      = RETROCEDER;
        tiempoRetroInicio = millis();
        Serial.println(F(">> BUSCAR: linea -> RETROCEDER"));
        break;
      }

      // Girar y leer distancia
      go_Right();
      set_Motorspeed(VEL_BUSQUEDA, VEL_BUSQUEDA, VEL_BUSQUEDA, VEL_BUSQUEDA);
      delay(TIEMPO_GIRO_BUSQUEDA);

      // UNA sola lectura positiva es suficiente para atacar
      if (distancia > 0 && distancia <= DIST_DETECCION) {
        estadoActual = ATACAR;
        Serial.println(F(">> BUSCAR -> ATACAR!"));
      }
      break;

    // ========================================================
    //  ESTADO 2: ATACAR
    //  Avanza a maxima velocidad.
    //  El loop corre en microsegundos aqui (sin ultrasonido ni Serial).
    //  Reaccion a la linea practicamente instantanea.
    // ========================================================
    case ATACAR:

      if (lineaDetectada) {
        // Parar inmediatamente - sin Serial antes para no anadir latencia
        stop_Stop();
        estadoActual      = RETROCEDER;
        tiempoRetroInicio = millis();
        // Serial DESPUES de parar (no afecta a los motores)
        Serial.println(F(">> ATACAR: linea detectada -> RETROCEDER"));
        break;
      }

      go_Advance();
      set_Motorspeed(VEL_ATAQUE, VEL_ATAQUE, VEL_ATAQUE, VEL_ATAQUE);
      break;

    // ========================================================
    //  ESTADO 3: RETROCEDER
    //  millis() en vez de delay para no bloquear el loop.
    //  Fase 1: marcha atras al centro.
    //  Fase 2: giro de reorientacion.
    //  Fase 3: vuelve a BUSCAR.
    // ========================================================
    case RETROCEDER:
    {
      unsigned long elapsed = millis() - tiempoRetroInicio;

      if (elapsed < (unsigned long)TIEMPO_RETROCESO) {
        // Fase 1: retroceder al centro
        go_Back();
        set_Motorspeed(VEL_RETROCESO, VEL_RETROCESO, VEL_RETROCESO, VEL_RETROCESO);

      } else {
        // Fase 2: parar y volver a BUSCAR (que ya gira por si solo)
        stop_Stop();
        estadoActual = BUSCAR;
        Serial.println(F(">> RETROCEDER terminado -> BUSCAR"));
      }
      break;
    }

  } // fin switch
}

// ============================================================
//  medirDistancia()
//  Devuelve distancia en cm. Sin eco -> 999.
// ============================================================
int medirDistancia() {
  digitalWrite(Trig_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(Trig_PIN, HIGH);
  delayMicroseconds(PULSO_TRIGGER);
  digitalWrite(Trig_PIN, LOW);

  long dur = pulseIn(Echo_PIN, HIGH, TIMEOUT_US);
  if (dur == 0) return 999;
  return (int)(dur * 0.01657);
}

// ============================================================
//  detectarLinea()
//  true si cualquier sensor IR detecta la linea negra.
//  HIGH = blanco, LOW = negro -> invertimos con !digitalRead
// ============================================================
bool detectarLinea() {
  return ( !digitalRead(sensor1) ||
           !digitalRead(sensor2) ||
           !digitalRead(sensor3) ||
           !digitalRead(sensor4) ||
           !digitalRead(sensor5) );
}

// ============================================================
//  FUNCIONES DE MOVIMIENTO
// ============================================================
void go_Advance() { FR_fwd(); FL_fwd(); RR_fwd(); RL_fwd(); }
void go_Back()    { FR_bck(); FL_bck(); RR_bck(); RL_bck(); }
void go_Left()    { FR_fwd(); FL_bck(); RR_fwd(); RL_bck(); }
void go_Right()   { FR_bck(); FL_fwd(); RR_bck(); RL_fwd(); }

void stop_Stop() {
  digitalWrite(RightMotorDirPin1,  LOW); digitalWrite(RightMotorDirPin2,  LOW);
  digitalWrite(LeftMotorDirPin1,   LOW); digitalWrite(LeftMotorDirPin2,   LOW);
  digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B,  LOW); digitalWrite(LeftMotorDirPin2B,  LOW);
  set_Motorspeed(0, 0, 0, 0);
}

void set_Motorspeed(int lf, int rf, int lb, int rb) {
  analogWrite(speedPinL,  lf);
  analogWrite(speedPinR,  rf);
  analogWrite(speedPinLB, lb);
  analogWrite(speedPinRB, rb);
}

// Helpers de direccion de motores
void FR_fwd() { digitalWrite(RightMotorDirPin1,  LOW);  digitalWrite(RightMotorDirPin2,  HIGH); }
void FR_bck() { digitalWrite(RightMotorDirPin1,  HIGH); digitalWrite(RightMotorDirPin2,  LOW);  }
void FL_fwd() { digitalWrite(LeftMotorDirPin1,   LOW);  digitalWrite(LeftMotorDirPin2,   HIGH); }
void FL_bck() { digitalWrite(LeftMotorDirPin1,   HIGH); digitalWrite(LeftMotorDirPin2,   LOW);  }
void RR_fwd() { digitalWrite(RightMotorDirPin1B, LOW);  digitalWrite(RightMotorDirPin2B, HIGH); }
void RR_bck() { digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW);  }
void RL_fwd() { digitalWrite(LeftMotorDirPin1B,  LOW);  digitalWrite(LeftMotorDirPin2B,  HIGH); }
void RL_bck() { digitalWrite(LeftMotorDirPin1B,  HIGH); digitalWrite(LeftMotorDirPin2B,  LOW);  }
