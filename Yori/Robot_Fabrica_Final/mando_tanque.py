import pygame
import socket
import time

ROBOT_IP = "192.168.4.1" 
PORT = 8080
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("¡Conecta el mando de PS5 primero!")
    exit()

mando = pygame.joystick.Joystick(0)
mando.init()
print(f"Mando: {mando.get_name()} | Conexión establecida.")

DEADZONE = 0.2 
marchas      = [   80,   130,   190,   255  ]
nombres      = ["LENTA", "BASE", "RÁPIDA", "TURBO"]
indice_marcha = 1  

print(f"Iniciando en >> MODO {nombres[indice_marcha]}")

estado_abrir_ant = False
estado_cerrar_ant = False

try:
    while True:
        pygame.event.pump()
        
        # --- LECTURA DE LOS JOYSTICKS ---
        y = -mando.get_axis(1) # Adelante/Atrás (Joystick izquierdo)
        r = mando.get_axis(2)  # Giro (Joystick derecho - eje X)

        if abs(y) < DEADZONE: y = 0
        if abs(r) < DEADZONE: r = 0

        # --- LÓGICA DE LA PUERTA (CRUCETA) ---
        btn_abrir = False; btn_cerrar = False
        
        try:
            if mando.get_numhats() > 0:
                cruceta = mando.get_hat(0)
                if cruceta[1] == 1: btn_abrir = True    
                elif cruceta[1] == -1: btn_cerrar = True 
                
            if mando.get_numbuttons() > 12:
                if mando.get_button(11): btn_abrir = True
                if mando.get_button(12): btn_cerrar = True
        except:
            pass
            
        if btn_abrir and not estado_abrir_ant:
            sock.sendto(b"PUERTA_ABRIR\n", (ROBOT_IP, PORT))
            estado_abrir_ant = True
            time.sleep(0.1) 
            continue 
            
        if btn_cerrar and not estado_cerrar_ant:
            sock.sendto(b"PUERTA_CERRAR\n", (ROBOT_IP, PORT))
            estado_cerrar_ant = True
            time.sleep(0.1) 
            continue 

        estado_abrir_ant = btn_abrir
        estado_cerrar_ant = btn_cerrar

        # --- LÓGICA DEL SERVO (BOTONES) ---
        try:
            btn_x = mando.get_button(0)       
            btn_circulo = mando.get_button(1) 
        except:
            btn_x = False
            btn_circulo = False

        estado_pinza = 0 
        if btn_x and not btn_circulo: estado_pinza = 1  
        elif btn_circulo and not btn_x: estado_pinza = -1 

        # --- LÓGICA DE CAJA DE CAMBIOS ---
        try:
            l2_val = mando.get_axis(4) 
            r2_val = mando.get_axis(5) 
            l1_pulsado = mando.get_button(4) or (mando.get_numbuttons() > 9 and mando.get_button(9))
            r1_pulsado = mando.get_button(5) or (mando.get_numbuttons() > 10 and mando.get_button(10))
        except:
            l2_val = -1.0; r2_val = -1.0; l1_pulsado = False; r1_pulsado = False

        nuevo_indice = indice_marcha
        if l1_pulsado: nuevo_indice = 0       
        elif l2_val > 0.0: nuevo_indice = 1       
        elif r1_pulsado: nuevo_indice = 2       
        elif r2_val > 0.0: nuevo_indice = 3       

        if nuevo_indice != indice_marcha:
            indice_marcha = nuevo_indice
            print(f">> CAMBIO A: MODO {nombres[indice_marcha]}")

        # --- MATEMÁTICAS MODO TANQUE (2 CANALES) ---
        velocidad_actual = marchas[indice_marcha]

        # Mezcla de ejes (arcade drive)
        left_track = y + r
        right_track = y - r

        max_val = max(abs(left_track), abs(right_track), 1.0)
        
        vl = int((left_track / max_val) * velocidad_actual)
        vr = int((right_track / max_val) * velocidad_actual)

        # Enviar 3 valores: Izquierda, Derecha, Servo
        paquete = f"{vl},{vr},{estado_pinza}\n".encode()
        sock.sendto(paquete, (ROBOT_IP, PORT))
        
        time.sleep(0.02) 

except KeyboardInterrupt:
    pygame.quit()
