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

# ==========================================
# LA CAJA DE CAMBIOS: 4 MODOS DE VUELO
# ==========================================
#              [ LENTA, BASE, RÁPIDA, TURBO ]
marchas      = [   80,   130,   190,   255  ]
nombres      = ["LENTA", "BASE", "RÁPIDA", "TURBO"]

indice_marcha = 0  # Empezamos en la posición 0 (LENTA = 80)

# Variables "Seguro" para no ametrallar los cambios de marcha
r2_pulsado_antes = False
l2_pulsado_antes = False

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

        # --- LECTURA DE GATILLOS Y CAMBIO DE MARCHAS ---
        try:
            l2 = mando.get_axis(4) # Gatillo Izquierdo (Bajar marcha)
            r2 = mando.get_axis(5) # Gatillo Derecho (Subir marcha)
        except:
            l2, r2 = -1.0, -1.0

        # SUBIR MARCHA (Gatillo Derecho R2)
        if r2 > 0.5 and not r2_pulsado_antes:
            # min(3, ...) evita que nos salgamos de la lista por arriba (Modo Turbo)
            indice_marcha = min(3, indice_marcha + 1)
            r2_pulsado_antes = True
            print(f"Subiendo a >> MODO {nombres[indice_marcha]} ({marchas[indice_marcha]}/255)")
        elif r2 < 0.0:
            r2_pulsado_antes = False # Quitamos el seguro al soltar

        # BAJAR MARCHA (Gatillo Izquierdo L2)
        if l2 > 0.5 and not l2_pulsado_antes:
            # max(0, ...) evita que nos salgamos de la lista por abajo (Modo Lenta)
            indice_marcha = max(0, indice_marcha - 1)
            l2_pulsado_antes = True
            print(f"Bajando a << MODO {nombres[indice_marcha]} ({marchas[indice_marcha]}/255)")
        elif l2 < 0.0:
            l2_pulsado_antes = False # Quitamos el seguro al soltar


        # Extraemos la velocidad real a la que tenemos que ir
        velocidad_actual = marchas[indice_marcha]

        # Potencia ruedas individuales (Matemáticas Mecanum)
        front_left  = y + x + r
        front_right = y - x - r
        back_left   = y - x + r
        back_right  = y + x - r

        # Evitar que las matemáticas se rompan
        max_val = max(abs(front_left), abs(front_right), abs(back_left), abs(back_right), 1.0)
        
        # Aplicamos la marcha en la que estemos (velocidad_actual)
        fl = int((front_left / max_val) * velocidad_actual)
        fr = int((front_right / max_val) * velocidad_actual)
        bl = int((back_left / max_val) * velocidad_actual)
        br = int((back_right / max_val) * velocidad_actual)

        # Enviar los 4 valores al Arduino separados por comas
        paquete = f"{fl},{fr},{bl},{br}\n".encode()
        
        sock.sendto(paquete, (ROBOT_IP, PORT))
        time.sleep(0.02) 

except KeyboardInterrupt:
    print("\nConexión finalizada.")
    pygame.quit()