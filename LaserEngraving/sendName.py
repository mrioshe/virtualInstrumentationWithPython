import serial
import time
import numpy as np

# Configuración de la conexión serial
SERIAL_PORT = 'COM7'  # Cambiar por el puerto correcto
BAUD_RATE = 115200

# Dimensiones del área de dibujo (en pasos)
MAX_X = 500
MAX_Y = 500

# Mapeo de caracteres a coordenadas (sistema simplificado)
def char_to_coordinates(char):
    char = char.upper()
    if char == 'A':
        return [
            (0, 0), (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, 0),
            (0, MAX_Y//2), (MAX_X, MAX_Y//2)
        ]
    elif char == 'B':
        return [
            (0, 0), (0, MAX_Y), (MAX_X, MAX_Y*3//4), (0, MAX_Y//2),
            (MAX_X, MAX_Y//4), (0, 0), (MAX_X, 0)
        ]
    elif char == 'C':
        return [
            (MAX_X, MAX_Y), (0, MAX_Y), (0, 0), (MAX_X, 0)
        ]
    elif char == 'D':
        return [
            (0, 0), (0, MAX_Y), (MAX_X, MAX_Y*3//4), 
            (MAX_X, MAX_Y//4), (0, 0)
        ]
    elif char == 'E':
        return [
            (MAX_X, MAX_Y), (0, MAX_Y), (0, 0), (MAX_X, 0),
            (0, MAX_Y//2), (MAX_X//2, MAX_Y//2)
        ]
    elif char == 'F':
        return [
            (0, 0), (0, MAX_Y), (MAX_X, MAX_Y),
            (0, MAX_Y//2), (MAX_X//2, MAX_Y//2)
        ]
    elif char == 'G':
        return [
            (MAX_X, MAX_Y), (0, MAX_Y), (0, 0), (MAX_X, 0),
            (MAX_X, MAX_Y//2), (MAX_X//2, MAX_Y//2)
        ]
    elif char == 'H':
        return [
            (0, 0), (0, MAX_Y), (0, MAX_Y//2), (MAX_X, MAX_Y//2),
            (MAX_X, MAX_Y), (MAX_X, 0)
        ]
    elif char == 'I':
        return [
            (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X//2, MAX_Y), 
            (MAX_X//2, 0), (0, 0), (MAX_X, 0)
        ]
    elif char == 'J':
        return [
            (0, MAX_Y), (MAX_X, MAX_Y), (MAX_X, 0),
            (MAX_X//2, 0), (MAX_X//2, MAX_Y//2)
        ]
    elif char == 'K':
        return [
            (0, 0), (0, MAX_Y), (0, MAX_Y//2), (MAX_X, MAX_Y),
            (0, MAX_Y//2), (MAX_X, 0)
        ]
    elif char == 'L':
        return [
            (0, MAX_Y), (0, 0), (MAX_X, 0)
        ]
    elif char == 'M':
        return [
            (0, 0), (0, MAX_Y), (MAX_X//2, MAX_Y//2), 
            (MAX_X, MAX_Y), (MAX_X, 0)
        ]
    elif char == 'N':
        return [
            (0, 0), (0, MAX_Y), (MAX_X, 0), (MAX_X, MAX_Y)
        ]
    elif char == 'O':
        return [
            (0, MAX_Y//2), (0, MAX_Y), (MAX_X, MAX_Y), 
            (MAX_X, 0), (0, 0), (0, MAX_Y//2)
        ]
    elif char == ' ':
        return [(0, 0), (MAX_X//2, 0)]
    else:
        return []  # Caracteres no soportados

def main():
    nombre = input("Ingrese el nombre a dibujar: ")
    
    # Generar secuencia de puntos
    puntos = []
    x_offset = 0
    
    for char in nombre:
        coords = char_to_coordinates(char)
        if coords:
            # Aplicar offset y agregar punto de levantamiento
            puntos.append((-1, -1))  # Marca para levantar el lápiz
            for x, y in coords:
                puntos.append((x + x_offset, y))
            x_offset += MAX_X + 200  # Espacio entre caracteres
    
    # Iniciar comunicación serial
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # Esperar inicialización
        
        # Enviar puntos
        for x, y in puntos:
            if x == -1 and y == -1:  # Comando especial
                ser.write(b'L\n')
            else:
                ser.write(f"{int(x)},{int(y)}\n".encode())
            time.sleep(0.01)  # Pequeña pausa
        
        print("Dibujo enviado al Arduino!")
        ser.close()
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()