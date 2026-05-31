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

# ==========================================
# LA CAJA DE CAMBIOS
# ==========================================
marchas      = [   80,   110,   170,   255  ]
nombres      = ["LENTA", "BASE", "RÁPIDA", "TURBO"]
indice_marcha = 1  

print(f"Iniciando en >> MODO {nombres[indice_marcha]} ({marchas[indice_marcha]}/255)")

try:
    while True:
        pygame.event.pump()
        
        # --- LECTURA DE LOS JOYSTICKS (INVERTIDOS) ---
        # Al quitarle el '-' a la Y y ponérselo a la R, invertimos las físicas
        y = mando.get_axis(1)   # Adelante / Atrás 
        r = -mando.get_axis(2)  # Rotación Izquierda/Derecha 

        if abs(y) < DEADZONE: y = 0
        if abs(r) < DEADZONE: r = 0

        # --- LECTURA DE CAJA DE CAMBIOS ---
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
            print(f">> CAMBIO A: MODO {nombres[indice_marcha]} ({marchas[indice_marcha]}/255)")

        # --- LÓGICA DE LAS PINZAS ---
        try:
            btn_x = mando.get_button(0)       
            btn_circulo = mando.get_button(1) 
        except:
            btn_x = False; btn_circulo = False

        estado_pinza = 0 
        if btn_x and not btn_circulo: estado_pinza = 1  
        elif btn_circulo and not btn_x: estado_pinza = -1 

        # --- MATEMÁTICAS MODO TANQUE ---
        velocidad_actual = marchas[indice_marcha]

        val_izq = y + r  
        val_der = y - r  

        max_val = max(abs(val_izq), abs(val_der), 1.0)
        
        motor_izq = int((val_izq / max_val) * velocidad_actual)
        motor_der = int((val_der / max_val) * velocidad_actual)

        # Enviamos: Izquierda, Derecha, 0, 0, Pinza
        paquete = f"{motor_izq},{motor_der},0,0,{estado_pinza}\n".encode()
        sock.sendto(paquete, (ROBOT_IP, PORT))
        
        time.sleep(0.02) 

except KeyboardInterrupt:
    print("\nConexión finalizada.")
    pygame.quit()