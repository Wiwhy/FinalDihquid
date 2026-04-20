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
print(f"Mando: {mando.get_name()} | Conexión establecida.")

DEADZONE = 0.2 

try:
    while True:
        pygame.event.pump()
        
        # Lectura joysticks
        x = mando.get_axis(0)  # Joystick Izquierdo (Eje Horizontal)
        y = -mando.get_axis(1) # Joystick Izquierdo (Eje Vertical)
        r = mando.get_axis(2)  # Joystick Derecho (Rotación)

        # Punto muerto
        if abs(x) < DEADZONE: x = 0
        if abs(y) < DEADZONE: y = 0
        if abs(r) < DEADZONE: r = 0

        # Velocidad base y gatillos
        try:
            l2 = mando.get_axis(4)
            r2 = mando.get_axis(5)
        except:
            l2, r2 = -1.0, -1.0

        gatillo_max = max(l2, r2)

        if gatillo_max > 0.6: max_speed = 255
        elif gatillo_max > 0.2: max_speed = 255
        elif gatillo_max > -0.2: max_speed = 255
        elif gatillo_max > -0.6: max_speed = 255
        else: max_speed = 255 # Marcha básica

        # Potencia ruedas individuales
        front_left  = y + x + r
        front_right = y - x - r
        back_left   = y - x + r
        back_right  = y + x - r

        # Evitar que las matemáticas pasen del 100% de potencia
        max_val = max(abs(front_left), abs(front_right), abs(back_left), abs(back_right), 1.0)
        
        # Aplicamos la marcha (velocidad de los gatillos)
        fl = int((front_left / max_val) * max_speed)
        fr = int((front_right / max_val) * max_speed)
        bl = int((back_left / max_val) * max_speed)
        br = int((back_right / max_val) * max_speed)

        # 4. Enviar los 4 valores al Arduino separados por comas
        paquete = f"{fl},{fr},{bl},{br}\n".encode()
        
        sock.sendto(paquete, (ROBOT_IP, PORT))
        time.sleep(0.02) # Más rápido para mayor fluidez

except KeyboardInterrupt:
    print("\nConexión finalizada.")
    pygame.quit()