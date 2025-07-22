import serial
import time

# Configuración del puerto serial
SERIAL_PORT = 'COM9'  # Asegúrate de que este sea el puerto COM correcto para tu ESP32
BAUD_RATE = 115200

# Dimensiones máximas del área de dibujo (en pasos o unidades arbitrarias)
# Ajusta estos valores según el tamaño de tu máquina CNC
# Si STEPS_PER_MM en ESP32 es 80, entonces 500 pasos = 6.25 mm.
# Ajusta estos valores para que las letras tengan un tamaño razonable en tu área de trabajo.
MAX_X = 500
MAX_Y = 500

# Función para obtener las coordenadas de un carácter
def char_to_coordinates(char):
    char = char.upper()
    # Las coordenadas son una secuencia de puntos para dibujar el carácter.
    # Cada tupla (x, y) es un punto.
    # Nota: El punto (-1, -1) se usa internamente para indicar "levantar lápiz"
    # antes de moverse al siguiente punto de inicio de trazo.
    if char == 'A':
        # Trazo 1: Diagonal izquierda
        return [(0, 0), (MAX_X // 2, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Diagonal derecha
                (MAX_X, 0), (MAX_X // 2, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Barra horizontal
                (MAX_X // 4, MAX_Y // 2), (MAX_X * 3 // 4, MAX_Y // 2)]
    elif char == 'B':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Curva superior
                (0, MAX_Y), (MAX_X * 0.75, MAX_Y), (MAX_X, MAX_Y * 0.75), (MAX_X * 0.75, MAX_Y // 2), (0, MAX_Y // 2),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Curva inferior
                (0, MAX_Y // 2), (MAX_X * 0.75, MAX_Y // 2), (MAX_X, MAX_Y * 0.25), (MAX_X * 0.75, 0), (0, 0)]
    elif char == 'C':
        # Dibuja una 'C' abierta
        return [(MAX_X, MAX_Y), (0, MAX_Y), (0, 0), (MAX_X, 0)]
    elif char == 'D':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Curva derecha
                (0, MAX_Y), (MAX_X, MAX_Y * 0.75), (MAX_X, MAX_Y * 0.25), (0, 0)]
    elif char == 'E':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Barra superior
                (0, MAX_Y), (MAX_X, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Barra central
                (0, MAX_Y // 2), (MAX_X * 0.75, MAX_Y // 2),
                (-1, -1), # Levantar lápiz
                # Trazo 4: Barra inferior
                (0, 0), (MAX_X, 0)]
    elif char == 'F':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Barra superior
                (0, MAX_Y), (MAX_X, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Barra central
                (0, MAX_Y // 2), (MAX_X * 0.75, MAX_Y // 2)]
    elif char == 'G':
        # Dibuja una 'G' simplificada (similar a una C con un trazo interno)
        return [(MAX_X, MAX_Y), (0, MAX_Y), (0, 0), (MAX_X, 0), (MAX_X, MAX_Y // 2), (MAX_X // 2, MAX_Y // 2)]
    elif char == 'H':
        # Trazo 1: Barra vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Barra vertical derecha
                (MAX_X, 0), (MAX_X, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Barra horizontal central
                (0, MAX_Y // 2), (MAX_X, MAX_Y // 2)]
    elif char == 'I':
        # Trazo 1: Barra central
        return [(MAX_X // 2, 0), (MAX_X // 2, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Barra superior
                (0, MAX_Y), (MAX_X, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Barra inferior
                (0, 0), (MAX_X, 0)]
    elif char == 'J':
        # Trazo 1: Vertical derecha
        return [(MAX_X, MAX_Y), (MAX_X, 0), (MAX_X // 2, 0), (0, MAX_Y // 4)]
    elif char == 'K':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Diagonal superior
                (0, MAX_Y // 2), (MAX_X, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Diagonal inferior
                (0, MAX_Y // 2), (MAX_X, 0)]
    elif char == 'L':
        # Trazo 1: Vertical izquierda
        return [(0, MAX_Y), (0, 0),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Horizontal inferior
                (0, 0), (MAX_X, 0)]
    elif char == 'M':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Diagonal superior izquierda
                (0, MAX_Y), (MAX_X // 2, MAX_Y // 2),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Diagonal superior derecha
                (MAX_X // 2, MAX_Y // 2), (MAX_X, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 4: Vertical derecha
                (MAX_X, MAX_Y), (MAX_X, 0)]
    elif char == 'N':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Diagonal
                (0, MAX_Y), (MAX_X, 0),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Vertical derecha
                (MAX_X, 0), (MAX_X, MAX_Y)]
    elif char == 'O':
        # Dibuja un cuadrado (o rectángulo) para simular la 'O'
        return [(0, 0), (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, 0), (0, 0)]
    elif char == 'P':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Curva superior
                (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, MAX_Y // 2), (0, MAX_Y // 2)]
    elif char == 'Q':
        # Dibuja una 'O' y añade una cola
        return [(0, 0), (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, 0), (0, 0), # La 'O'
                (-1, -1), # Levantar lápiz
                (MAX_X // 2, MAX_Y // 2), (MAX_X, 0)] # La cola
    elif char == 'R':
        # Trazo 1: Vertical izquierda
        return [(0, 0), (0, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Curva superior
                (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, MAX_Y // 2), (0, MAX_Y // 2),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Diagonal inferior
                (MAX_X // 2, MAX_Y // 2), (MAX_X, 0)]
    elif char == 'S':
        # Dibuja una 'S'
        return [(MAX_X, MAX_Y), (0, MAX_Y), (0, MAX_Y // 2), (MAX_X, MAX_Y // 2), (MAX_X, 0), (0, 0)]
    elif char == 'T':
        # Trazo 1: Barra superior
        return [(0, MAX_Y), (MAX_X, MAX_Y),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Vertical central
                (MAX_X // 2, MAX_Y), (MAX_X // 2, 0)]
    elif char == 'U':
        # Dibuja una 'U'
        return [(0, MAX_Y), (0, 0), (MAX_X, 0), (MAX_X, MAX_Y)]
    elif char == 'V':
        # Dibuja una 'V'
        return [(0, MAX_Y), (MAX_X // 2, 0), (MAX_X, MAX_Y)]
    elif char == 'W':
        # Dibuja una 'W'
        return [(0, MAX_Y), (0, 0), (MAX_X // 2, MAX_Y // 2), (MAX_X, 0), (MAX_X, MAX_Y)]
    elif char == 'X':
        # Trazo 1: Diagonal de arriba izquierda a abajo derecha
        return [(0, MAX_Y), (MAX_X, 0),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Diagonal de arriba derecha a abajo izquierda
                (MAX_X, MAX_Y), (0, 0)]
    elif char == 'Y':
        # Trazo 1: Diagonal superior izquierda
        return [(0, MAX_Y), (MAX_X // 2, MAX_Y // 2),
                (-1, -1), # Levantar lápiz
                # Trazo 2: Diagonal superior derecha
                (MAX_X, MAX_Y), (MAX_X // 2, MAX_Y // 2),
                (-1, -1), # Levantar lápiz
                # Trazo 3: Vertical inferior
                (MAX_X // 2, MAX_Y // 2), (MAX_X // 2, 0)]
    elif char == 'Z':
        # Dibuja una 'Z'
        return [(0, MAX_Y), (MAX_X, MAX_Y), (0, 0), (MAX_X, 0)]
    elif char == '0':
        # Dibuja un cuadrado (o rectángulo) para simular el '0'
        return [(0, 0), (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, 0), (0, 0)]
    elif char == '1':
        # Dibuja un '1'
        return [(MAX_X // 2, 0), (MAX_X // 2, MAX_Y), (0, MAX_Y * 0.75)]
    elif char == '2':
        return [(0, MAX_Y * 0.75), (MAX_X, MAX_Y), (MAX_X, MAX_Y // 2), (0, MAX_Y // 4), (MAX_X, 0)]
    elif char == '3':
        return [(0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, MAX_Y // 2), (0, MAX_Y // 2), (MAX_X, MAX_Y // 2), (MAX_X, 0), (0, 0)]
    elif char == '4':
        return [(MAX_X, MAX_Y), (0, MAX_Y // 2), (MAX_X, MAX_Y // 2), (MAX_X, 0)]
    elif char == '5':
        return [(MAX_X, MAX_Y), (0, MAX_Y), (0, MAX_Y // 2), (MAX_X, MAX_Y // 2), (MAX_X, 0), (0, 0)]
    elif char == '6':
        return [(MAX_X, MAX_Y), (0, MAX_Y), (0, 0), (MAX_X, 0), (MAX_X, MAX_Y // 2), (0, MAX_Y // 2)]
    elif char == '7':
        return [(0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, 0)]
    elif char == '8':
        return [(0, MAX_Y // 2), (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, 0), (0, 0), (0, MAX_Y // 2), (MAX_X, MAX_Y // 2)]
    elif char == '9':
        return [(MAX_X, MAX_Y // 2), (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, 0), (0, 0)]
    elif char == ' ':
        return [(0, 0), (MAX_X, 0)] # Espacio en blanco, solo para avanzar el offset
    else:
        print(f"[WARN] Carácter '{char}' no soportado. Se omitirá.")
        return []

# Función para esperar la confirmación 'OK' del ESP32
def wait_for_ok(ser, timeout=20): # Timeout de 20 segundos para movimientos largos
    start_time = time.time()
    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            # Imprimir todas las líneas recibidas mientras se espera el OK para depuración
            print(f"[RECEIVED ACK] {line}")
            if line == "OK":
                return True
        time.sleep(0.01) # Pequeña pausa para no saturar la CPU
    print("[ERROR] Timeout esperando confirmación 'OK' del ESP32. El ESP32 no respondió a tiempo.")
    return False

# Función para enviar el nombre/palabra a la CNC
def send_name_to_cnc(nombre, ser):
    print(f"[INFO] Recibido nombre para dibujar: '{nombre}'")
    
    # Lista para almacenar todos los puntos de todos los caracteres
    all_points = []
    x_offset = 0 # Desplazamiento horizontal para cada carácter

    for char_index, char in enumerate(nombre):
        print(f"[INFO] Procesando carácter: '{char}' ({char_index + 1}/{len(nombre)})")
        coords = char_to_coordinates(char)
        
        if coords:
            # Añadir un comando para levantar el lápiz antes de mover al inicio del carácter
            all_points.append((-1, -1)) # Indicador de "levantar lápiz"
            for x, y in coords:
                all_points.append((x + x_offset, y))
            
            # Ajustar el offset para el siguiente carácter.
            # Se añade un espacio extra (200) entre caracteres.
            x_offset += MAX_X + 200
        
        # --- Retardo entre letras ---
        # Este retardo se aplica después de que se han generado todos los puntos para una letra,
        # pero antes de pasar a la siguiente letra.
        if char_index < len(nombre) - 1: # No aplicar el retardo después de la última letra
            print(f"[INFO] Retardo de 10 segundos antes de la siguiente letra...")
            time.sleep(10) # Retardo de 10 segundos entre letras

    print(f"[INFO] Total de segmentos de dibujo a enviar: {len(all_points)}")

    for i, (x, y) in enumerate(all_points):
        if x == -1 and y == -1:
            # Comando para levantar el lápiz (láser apagado)
            print(f"[SEND] L (levantar lápiz) - Segmento {i+1}/{len(all_points)}")
            ser.write(b'L\n')
        else:
            # Comando de movimiento a una coordenada (láser encendido)
            comando = f"{x},{y}"
            print(f"[SEND] {comando} - Segmento {i+1}/{len(all_points)}")
            ser.write(f"{comando}\n".encode())
        
        # Esperar la confirmación 'OK' del ESP32 después de cada comando
        # Si el ESP32 no responde con 'OK', se detiene el envío.
        if not wait_for_ok(ser):
            print("[CRITICAL ERROR] No se recibió confirmación 'OK'. Deteniendo el envío de comandos.")
            break # Detener el proceso si no hay confirmación

    print("[INFO] Dibujo completado.\n")

# Función principal para manejar la comunicación serial
def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # Espera a que el puerto serial se inicialice

        print(f"[INFO] Conectado al puerto serial {SERIAL_PORT} a {BAUD_RATE} baudios.")
        print("[INFO] Esperando el mensaje de inicio del servidor del ESP32 ('Servidor HTTP iniciado') o un comando DRAW:...")

        # Leer líneas hasta que se encuentre el mensaje esperado o un comando DRAW:
        found_ready_signal = False
        start_time = time.time()
        while not found_ready_signal and (time.time() - start_time < 30): # Timeout de 30 segundos para inicio
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                print(f"[DISCARDED BOOT/PRE-READY] {line}") # Mostrar todos los mensajes descartados
                if "Servidor HTTP iniciado" in line:
                    found_ready_signal = True
                    print("[INFO] Mensaje de inicio del servidor recibido. Listo para comandos.")
                elif line.startswith("DRAW:"): # ¡Re-añadido esta condición!
                    print("[INFO] Comando DRAW: recibido antes del mensaje de inicio. Procesando y listo para comandos.")
                    found_ready_signal = True
                    # Procesar el comando DRAW: inmediatamente
                    nombre = line[5:].strip()
                    if nombre:
                        send_name_to_cnc(nombre, ser)
                    # No salir del bucle while True, solo de la fase de inicio
            time.sleep(0.05) # Pequeña pausa para evitar saturar la CPU

        if not found_ready_signal:
            print("[ERROR] Timeout esperando el mensaje de inicio del ESP32 o un comando DRAW:. Asegúrate de que el ESP32 esté encendido y conectado.")
            return # Salir si no se encontró el mensaje de inicio o un comando DRAW:

        print("[INFO] Esperando el comando 'DRAW:' desde el ESP32...")

        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                print(f"[RECEIVED] {line}") # Imprimir todas las líneas recibidas para depuración

                if line.startswith("DRAW:"):
                    nombre = line[5:].strip()
                    if nombre:
                        send_name_to_cnc(nombre, ser)
            time.sleep(0.01) # Pequeña pausa para no saturar la CPU

    except serial.SerialException as e:
        print(f"[ERROR] No se pudo abrir el puerto serial {SERIAL_PORT}: {e}")
        print("Asegúrate de que el ESP32 esté conectado y el puerto COM sea correcto.")
        print("Verifica que el ESP32 no esté siendo usado por otro programa (ej. Serial Monitor).")
    except Exception as e:
        print(f"[ERROR] Ocurrió un error inesperado: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("[INFO] Puerto serial cerrado.")

if __name__ == "__main__":
    main()
