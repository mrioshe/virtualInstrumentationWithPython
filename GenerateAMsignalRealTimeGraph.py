import numpy as np
import matplotlib.pyplot as plt
import serial
import time

# Parámetros de la señal
fs = 1000  # Frecuencia de muestreo
fc = 60    # Portadora
fm = 8     # Envolvente
Am = 0.15
Ac = 0.3
offset = 2.5

# Abrir puerto serial
puerto = serial.Serial('COM9', 9600)  # Cambia el puerto según tu sistema
time.sleep(2)

# Inicializar gráfico interactivo
plt.ion()
fig, ax = plt.subplots()
xdata, ydata = [], []
line, = ax.plot([], [], lw=2)
ax.set_xlim(0, 1)
ax.set_ylim(offset - 0.5, offset + 0.5)
ax.set_title("Señal AM Enviada en Tiempo Real")
ax.set_xlabel("Tiempo (s)")
ax.set_ylabel("Amplitud")

# Envío y gráfica en tiempo real
t0 = time.time()

try:
    while True:
        t = time.time() - t0

        # Señal en tiempo t
        envolvente = Am * np.sin(2 * np.pi * fm * t)
        valor = Ac * (1 + envolvente / Ac) * np.sin(2 * np.pi * fc * t) + offset

        # Enviar por serial
        mensaje = f"{valor:.4f}\n"
        puerto.write(mensaje.encode())

        # Agregar a la gráfica
        xdata.append(t)
        ydata.append(valor)

        if len(xdata) > fs:
            xdata = xdata[-fs:]
            ydata = ydata[-fs:]

        line.set_data(xdata, ydata)
        ax.set_xlim(max(0, t - 1), t)
        fig.canvas.draw()
        fig.canvas.flush_events()

        time.sleep(1 / fs)

except KeyboardInterrupt:
    puerto.close()
    print("Transmisión detenida.")
