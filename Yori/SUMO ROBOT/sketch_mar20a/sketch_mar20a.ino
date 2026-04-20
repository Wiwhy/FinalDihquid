#include "WiFiEsp.h"
#include "WiFiEspUdp.h"

char ssid[] = "ROBOT_COMPETICION"; 
char pass[] = "12345678"; 
int status = WL_IDLE_STATUS;
unsigned int localPort = 8080;  
WiFiEspUDP Udp;

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

// ==========================================
// VARIABLES DEL "SUAVIZADOR" DE VELOCIDAD
// ==========================================
int current_speed = 0;       // La velocidad real que tienen los motores en este milisegundo
int target_speed = 0;        // La velocidad que le pide el mando de PS5
char active_command = 'P';   // La dirección en la que nos estamos moviendo
unsigned long last_millis = 0; // Temporizador

void setup() {
  Serial.begin(115200);   
  Serial1.begin(115200);  

  pinMode(RightMotorDirPin1, OUTPUT); pinMode(RightMotorDirPin2, OUTPUT); pinMode(speedPinL, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT); pinMode(LeftMotorDirPin2, OUTPUT); pinMode(speedPinR, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT); pinMode(RightMotorDirPin2B, OUTPUT); pinMode(speedPinLB, OUTPUT);
  pinMode(LeftMotorDirPin1B, OUTPUT); pinMode(LeftMotorDirPin2B, OUTPUT); pinMode(speedPinRB, OUTPUT);

  stop_Stop();

  WiFi.init(&Serial1);
  WiFi.beginAP(ssid, 10, pass, ENC_TYPE_WPA2_PSK);
  Udp.begin(localPort);
  Serial.println("¡Mecanum listo! Modo Turbo Suave. Esperando comandos...");
}

void loop() {
  // 1. LEER EL MANDO (Rápido y sin pausas)
  int packetSize = Udp.parsePacket();
  if (packetSize >= 2) { 
    char command = Udp.read();
    char speed_char = Udp.read();

    if (command == 'P') {
      target_speed = 0; // Si soltamos el joystick, queremos parar (velocidad objetivo 0)
    } else {
      active_command = command; // Guardamos hacia dónde queremos ir
      
      // Asignamos la velocidad objetivo
      if (speed_char == '1') target_speed = 130;       
      else if (speed_char == '2') target_speed = 160;  
      else if (speed_char == '3') target_speed = 200;  
      else if (speed_char == '4') target_speed = 230;  
      else if (speed_char == '5') target_speed = 255;  
    }
  }

  // 2. LA RAMPA DE ACELERACIÓN (Se ejecuta cada 10 milisegundos)
  if (millis() - last_millis >= 10) { 
    last_millis = millis();

    // Si vamos más lentos de lo que debemos, ACELERAMOS SUAVEMENTE
    if (current_speed < target_speed) {
      current_speed += 4; // Suma 4 puntos de velocidad. Evita el tirón eléctrico.
      if (current_speed > target_speed) current_speed = target_speed;
    }
    // Si vamos más rápido de lo que debemos, FRENAMOS SUAVEMENTE
    else if (current_speed > target_speed) {
      current_speed -= 8; // Frenamos un poco más rápido de lo que aceleramos
      if (current_speed < target_speed) current_speed = target_speed;
    }

    // 3. APLICAR POTENCIA A LOS MOTORES
    if (current_speed == 0) {
      stop_Stop();
      active_command = 'P';
    } else {
      ejecutar_movimiento(active_command);
    }
  }
}

// Función auxiliar para aplicar la dirección correcta con la velocidad suavizada
void ejecutar_movimiento(char cmd) {
  switch(cmd) {
    case 'W': advance(); break;           
    case 'S': back(); break;              
    case 'A': turnLeft(); break;          
    case 'D': turnRight(); break;         
    case 'Q': go_Diag_FL(); break;        
    case 'E': go_Diag_FR(); break;        
    case 'Z': go_Diag_BL(); break;        
    case 'C': go_Diag_BR(); break;        
    case 'J': rotate_Left(); break;       
    case 'K': rotate_Right(); break;      
  }
}

// =========================================================
// CINEMÁTICA PARA RUEDAS MECANUM OSOYOO
// =========================================================
void set_Motorspeed(int FL, int FR, int BL, int BR) {
  analogWrite(speedPinL, FL);  
  analogWrite(speedPinR, FR);  
  analogWrite(speedPinLB, BL); 
  analogWrite(speedPinRB, BR); 
}

void FR_fwd() { digitalWrite(RightMotorDirPin1, LOW); digitalWrite(RightMotorDirPin2, HIGH); }
void FR_bck() { digitalWrite(RightMotorDirPin1, HIGH); digitalWrite(RightMotorDirPin2, LOW); }
void FL_fwd() { digitalWrite(LeftMotorDirPin1, LOW); digitalWrite(LeftMotorDirPin2, HIGH); }
void FL_bck() { digitalWrite(LeftMotorDirPin1, HIGH); digitalWrite(LeftMotorDirPin2, LOW); }
void RR_fwd() { digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, HIGH); }
void RR_bck() { digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW); }
void RL_fwd() { digitalWrite(LeftMotorDirPin1B, LOW); digitalWrite(LeftMotorDirPin2B, HIGH); }
void RL_bck() { digitalWrite(LeftMotorDirPin1B, HIGH); digitalWrite(LeftMotorDirPin2B, LOW); }

void advance() { FL_fwd(); FR_fwd(); RL_fwd(); RR_fwd(); set_Motorspeed(current_speed, current_speed, current_speed, current_speed); }
void back()    { FL_bck(); FR_bck(); RL_bck(); RR_bck(); set_Motorspeed(current_speed, current_speed, current_speed, current_speed); }
void turnLeft() { FL_bck(); FR_fwd(); RL_fwd(); RR_bck(); set_Motorspeed(current_speed, current_speed, current_speed, current_speed); } 
void turnRight(){ FL_fwd(); FR_bck(); RL_bck(); RR_fwd(); set_Motorspeed(current_speed, current_speed, current_speed, current_speed); } 
void rotate_Left()  { FL_bck(); FR_fwd(); RL_bck(); RR_fwd(); set_Motorspeed(current_speed, current_speed, current_speed, current_speed); }
void rotate_Right() { FL_fwd(); FR_bck(); RL_fwd(); RR_bck(); set_Motorspeed(current_speed, current_speed, current_speed, current_speed); }
void go_Diag_FL() { FL_bck(); FR_fwd(); RL_fwd(); RR_bck(); set_Motorspeed(0, current_speed, current_speed, 0); } 
void go_Diag_FR() { FL_fwd(); FR_bck(); RL_bck(); RR_fwd(); set_Motorspeed(current_speed, 0, 0, current_speed); }
void go_Diag_BL() { FL_fwd(); FR_bck(); RL_bck(); RR_fwd(); set_Motorspeed(0, current_speed, current_speed, 0); }
void go_Diag_BR() { FL_bck(); FR_fwd(); RL_fwd(); RR_bck(); set_Motorspeed(current_speed, 0, 0, current_speed); }

void stop_Stop() {
  digitalWrite(RightMotorDirPin1, LOW); digitalWrite(RightMotorDirPin2, LOW);
  digitalWrite(LeftMotorDirPin1, LOW); digitalWrite(LeftMotorDirPin2, LOW);
  digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B, LOW); digitalWrite(LeftMotorDirPin2B, LOW);
  set_Motorspeed(0,0,0,0);
}