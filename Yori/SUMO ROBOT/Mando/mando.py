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
print(f"Mando: {mando.get_name()} | Mecanum Mode: ACTIVADO")

DEADZONE = 0.3 

try:
    while True:
        pygame.event.pump()
        
        # LEER JOYSTICKS
        joy_izq_x = mando.get_axis(0)
        joy_izq_y = mando.get_axis(1)
        joy_der_x = mando.get_axis(2) 
        
        # LEER GATILLOS PARA LAS 5 VELOCIDADES (Ejes 4 y 5 en PC suelen ser L2 y R2)
        # Los gatillos en reposo valen -1.0, y pulsados a fondo valen 1.0
        try:
            l2 = mando.get_axis(4)
            r2 = mando.get_axis(5)
        except:
            l2, r2 = -1.0, -1.0
            
        # Nos quedamos con el gatillo que esté más pulsado
        gatillo_max = max(l2, r2)
        
        # Mapeamos la presión del gatillo a 5 niveles de marcha
        if gatillo_max > 0.6: nivel_vel = '5'
        elif gatillo_max > 0.2: nivel_vel = '4'
        elif gatillo_max > -0.2: nivel_vel = '3'
        elif gatillo_max > -0.6: nivel_vel = '2'
        else: nivel_vel = '1' # Velocidad base si no tocas nada

        comando_dir = 'P' # Por defecto: Parar

        # PRIORIDAD 1: ROTAR SOBRE SU EJE (Joystick Derecho)
        if joy_der_x < -DEADZONE:
            comando_dir = 'J' # Rotar Izquierda
        elif joy_der_x > DEADZONE:
            comando_dir = 'K' # Rotar Derecha
            
        # PRIORIDAD 2: MOVIMIENTO OMNIDIRECCIONAL MECANUM (Joystick Izquierdo)
        elif abs(joy_izq_x) > DEADZONE or abs(joy_izq_y) > DEADZONE:
            if joy_izq_y < -DEADZONE: # Zona Delantera
                if joy_izq_x < -DEADZONE: comando_dir = 'Q'   # Diagonal Adelante-Izquierda
                elif joy_izq_x > DEADZONE: comando_dir = 'E'  # Diagonal Adelante-Derecha
                else: comando_dir = 'W'                       # Adelante puro
                    
            elif joy_izq_y > DEADZONE: # Zona Trasera
                if joy_izq_x < -DEADZONE: comando_dir = 'Z'   # Diagonal Atrás-Izquierda
                elif joy_izq_x > DEADZONE: comando_dir = 'C'  # Diagonal Atrás-Derecha
                else: comando_dir = 'S'                       # Atrás puro
                
            else: # Zona Lateral Pura (Strafe - se mueve de lado sin girar)
                if joy_izq_x < -DEADZONE: comando_dir = 'A'   # Desplazamiento Izquierda
                elif joy_izq_x > DEADZONE: comando_dir = 'D'  # Desplazamiento Derecha

        # Unimos la letra de dirección con el número de velocidad (Ej: "W5" o "A2")
        paquete = (comando_dir + nivel_vel).encode()
        
        sock.sendto(paquete, (ROBOT_IP, PORT))
        time.sleep(0.04) 

except KeyboardInterrupt:
    print("\nConexión finalizada.")
    pygame.quit()