#include "WiFiEsp.h"
#include "WiFiEspUdp.h"

char ssid[] = "ROBOT_COMPETICION"; 
char pass[] = "12345678"; 
int status = WL_IDLE_STATUS;
unsigned int localPort = 8080;  
WiFiEspUDP Udp;

// Pines de los motores
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

// ==========================================
// SUAVIZADOR DE VELOCIDAD (SOLO ACELERACIÓN)
// ==========================================
int cur_fl = 0, target_fl = 0;
int cur_fr = 0, target_fr = 0;
int cur_bl = 0, target_bl = 0;
int cur_br = 0, target_br = 0;
unsigned long last_millis = 0; 

void setup() {
  Serial.begin(115200);   
  Serial1.begin(115200);  

  pinMode(RightMotorDirPin1, OUTPUT); pinMode(RightMotorDirPin2, OUTPUT); pinMode(speedPinL, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT); pinMode(LeftMotorDirPin2, OUTPUT); pinMode(speedPinR, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT); pinMode(RightMotorDirPin2B, OUTPUT); pinMode(speedPinLB, OUTPUT);
  pinMode(LeftMotorDirPin1B, OUTPUT); pinMode(LeftMotorDirPin2B, OUTPUT); pinMode(speedPinRB, OUTPUT);

  apagar_motores();

  WiFi.init(&Serial1);
  WiFi.beginAP(ssid, 10, pass, ENC_TYPE_WPA2_PSK);
  Udp.begin(localPort);
  Serial.println("¡Mecanum 360 listo! Freno en seco ACTIVADO...");
}

void loop() {
  // 1. LEER EL PAQUETE DE DATOS DE PYTHON (Ejemplo: "150,-100,150,-100")
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    char packetBuffer[64];
    int len = Udp.read(packetBuffer, 63);
    if (len > 0) {
      packetBuffer[len] = '\0'; // Cerrar el texto
    }
    
    // Extraer los 4 números y guardarlos como objetivo
    sscanf(packetBuffer, "%d,%d,%d,%d", &target_fl, &target_fr, &target_bl, &target_br);
  }

  // 2. LA FÍSICA DE MOVIMIENTO (Se ejecuta cada 10ms)
  if (millis() - last_millis >= 10) { 
    last_millis = millis();

    suavizar_rueda(cur_fl, target_fl);
    suavizar_rueda(cur_fr, target_fr);
    suavizar_rueda(cur_bl, target_bl);
    suavizar_rueda(cur_br, target_br);

    // 3. APLICAR POTENCIA A LOS MOTORES
    aplicarMotor(1, cur_fl); // Frontal Izquierdo
    aplicarMotor(2, cur_fr); // Frontal Derecho
    aplicarMotor(3, cur_bl); // Trasero Izquierdo
    aplicarMotor(4, cur_br); // Trasero Derecho
  }
}

// =========================================================
// LÓGICA DE FRENADO EN SECO Y ACELERACIÓN SUAVE
// =========================================================
void suavizar_rueda(int &current, int target) {
  // CASO 1: Si soltamos el mando (0) o cambiamos de sentido bruscamente -> FRENAMOS EN SECO
  if (target == 0 || (current > 0 && target < 0) || (current < 0 && target > 0)) {
    current = target;
    return;
  }
  
  // CASO 2: Si estamos reduciendo la velocidad (pero sin llegar a 0) -> FRENAMOS EN SECO
  if (abs(target) < abs(current)) {
    current = target;
    return;
  }

  // CASO 3: Si estamos ACELERANDO (Aumentando la potencia hacia adelante o hacia atrás) -> SUAVIZAMOS
  if (current < target) {
    current += 8; // Aceleración hacia adelante
    if (current > target) current = target;
  } 
  else if (current > target) {
    current -= 8; // Aceleración hacia atrás (números negativos)
    if (current < target) current = target;
  }
}

// =========================================================
// CONTROL INDIVIDUAL DE MOTORES
// =========================================================
void aplicarMotor(int motor, int velocidad) {
  bool hacia_adelante = (velocidad >= 0);
  int pwm = abs(velocidad);
  if (pwm > 255) pwm = 255; // Seguridad para no quemar motores

  switch(motor) {
    case 1: // FL (Frontal Izquierdo)
      digitalWrite(LeftMotorDirPin1, hacia_adelante ? LOW : HIGH);
      digitalWrite(LeftMotorDirPin2, hacia_adelante ? HIGH : LOW);
      analogWrite(speedPinL, pwm);
      break;
    case 2: // FR (Frontal Derecho)
      digitalWrite(RightMotorDirPin1, hacia_adelante ? LOW : HIGH);
      digitalWrite(RightMotorDirPin2, hacia_adelante ? HIGH : LOW);
      analogWrite(speedPinR, pwm);
      break;
    case 3: // BL (Trasero Izquierdo)
      digitalWrite(LeftMotorDirPin1B, hacia_adelante ? LOW : HIGH);
      digitalWrite(LeftMotorDirPin2B, hacia_adelante ? HIGH : LOW);
      analogWrite(speedPinLB, pwm);
      break;
    case 4: // BR (Trasero Derecho)
      digitalWrite(RightMotorDirPin1B, hacia_adelante ? LOW : HIGH);
      digitalWrite(RightMotorDirPin2B, hacia_adelante ? HIGH : LOW);
      analogWrite(speedPinRB, pwm);
      break;
  }
}

void apagar_motores() {
  target_fl = 0; target_fr = 0; target_bl = 0; target_br = 0;
  aplicarMotor(1, 0); aplicarMotor(2, 0); aplicarMotor(3, 0); aplicarMotor(4, 0);
}