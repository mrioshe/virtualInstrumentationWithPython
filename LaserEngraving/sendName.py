import serial
import time

# Configura el puerto serial (ajusta el nombre del puerto)
esp = serial.Serial('COM7', 115200, timeout=1)
time.sleep(2)  # Espera a que el ESP32 reinicie

nombre = "Luis"
esp.write((nombre + "\n").encode())  # Envía el nombre seguido de nueva línea
print(f"Enviado: {nombre}")

# Espera respuestas (opcional)
while True:
    if esp.in_waiting:
        respuesta = esp.readline().decode().strip()
        print("ESP32 dice:", respuesta)
        if respuesta == "FIN":
            break

esp.close()
