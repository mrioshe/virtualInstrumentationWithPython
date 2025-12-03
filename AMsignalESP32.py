import numpy as np
import matplotlib
matplotlib.use('Agg')  # Backend sin interfaz gráfica
import matplotlib.pyplot as plt
import serial
import time
import os

# ============================================
# PARÁMETROS DE LA SEÑAL AM
# ============================================
fc = 1000       # Frecuencia portadora (Hz)
fm = 50         # Frecuencia moduladora (Hz)
m = 0.5         # Índice de modulación (0-1)
offset = 1.65   # Offset DC (1.65V = punto medio del DAC)
amplitud = 1.0  # Amplitud de la portadora (V)

duracion_preview = 0.1  # Duración para vista previa (s)

# ============================================
# CONEXIÓN SERIAL
# ============================================
puerto_esp32 = 'COM9'  # Puerto del ESP32

print(f"Conectando al ESP32 en {puerto_esp32}...")
try:
    puerto = serial.Serial(puerto_esp32, 115200, timeout=2)
    time.sleep(2)
    print("✓ Conectado al ESP32")
except Exception as e:
    print(f"❌ Error al conectar: {e}")
    exit(1)

# ============================================
# ENVIAR PARÁMETROS AL ESP32
# ============================================
comando = f"START,{fc},{fm},{m},{offset},{amplitud}\n"
print(f"Enviando: {comando.strip()}")
puerto.write(comando.encode())

# Esperar confirmación
time.sleep(0.5)
if puerto.in_waiting:
    respuesta = puerto.readline().decode().strip()
    print(f"ESP32: {respuesta}")

print("\n✓ Señal AM generándose en ESP32")
print(f"  Portadora: {fc} Hz")
print(f"  Moduladora: {fm} Hz")
print(f"  Índice modulación: {m}")

# ============================================
# GENERAR Y GUARDAR GRÁFICAS (SIN MOSTRAR)
# ============================================
fs_preview = 50000  # Frecuencia de muestreo para gráfica
t = np.linspace(0, duracion_preview, int(fs_preview * duracion_preview))

# Señales
envolvente = m * np.sin(2 * np.pi * fm * t)
portadora = np.sin(2 * np.pi * fc * t)
senal_am = amplitud * (1 + m * np.sin(2 * np.pi * fm * t)) * np.sin(2 * np.pi * fc * t)
senal_am_offset = senal_am + offset

# Crear directorio para gráficas
if not os.path.exists('graficas'):
    os.makedirs('graficas')

# ============================================
# GRÁFICA 1: Señales
# ============================================
plt.figure(figsize=(14, 10))

# Gráfica 1: Envolvente
plt.subplot(4, 1, 1)
plt.plot(t * 1000, envolvente, color='orange', linewidth=1.5)
plt.title(f'Envolvente ({fm} Hz)', fontsize=12, fontweight='bold')
plt.ylabel('Amplitud')
plt.grid(True, alpha=0.3)
plt.xlim(0, duracion_preview * 1000)

# Gráfica 2: Portadora
plt.subplot(4, 1, 2)
plt.plot(t * 1000, portadora, color='red', linewidth=0.8)
plt.title(f'Portadora ({fc} Hz)', fontsize=12, fontweight='bold')
plt.ylabel('Amplitud')
plt.grid(True, alpha=0.3)
plt.xlim(0, duracion_preview * 1000)

# Gráfica 3: Señal AM
plt.subplot(4, 1, 3)
plt.plot(t * 1000, senal_am, color='blue', linewidth=0.8)
plt.title(f'Señal AM (m={m})', fontsize=12, fontweight='bold')
plt.ylabel('Amplitud')
plt.grid(True, alpha=0.3)
plt.xlim(0, duracion_preview * 1000)

# Gráfica 4: Señal con offset (salida del DAC)
plt.subplot(4, 1, 4)
plt.plot(t * 1000, senal_am_offset, color='purple', linewidth=0.8)
plt.axhline(y=0, color='gray', linestyle='--', linewidth=0.5, label='0V')
plt.axhline(y=3.3, color='gray', linestyle='--', linewidth=0.5, label='3.3V')
plt.title(f'Salida DAC (offset={offset}V)', fontsize=12, fontweight='bold')
plt.xlabel('Tiempo (ms)')
plt.ylabel('Voltaje (V)')
plt.grid(True, alpha=0.3)
plt.xlim(0, duracion_preview * 1000)
plt.ylim(-0.2, 3.5)
plt.legend(loc='upper right')

plt.tight_layout()
plt.savefig('graficas/senal_am_temporal.png', dpi=150)
print("✓ Gráfica temporal guardada: graficas/senal_am_temporal.png")
plt.close()

# ============================================
# GRÁFICA 2: FFT
# ============================================
fft_signal = np.fft.fft(senal_am_offset)
frecuencias = np.fft.fftfreq(len(t), 1/fs_preview)
n = len(t) // 2

plt.figure(figsize=(14, 5))
plt.plot(frecuencias[:n], np.abs(fft_signal[:n]), color='darkblue', linewidth=1)
plt.title('Espectro de Frecuencia - Señal AM', fontsize=14, fontweight='bold')
plt.xlabel('Frecuencia (Hz)')
plt.ylabel('Magnitud')
plt.grid(True, alpha=0.3)
plt.xlim(0, fc + 200)

# Marcar frecuencias clave
plt.axvline(x=fc, color='red', linestyle='--', linewidth=1.5, label=f'Portadora ({fc} Hz)')
plt.axvline(x=fc-fm, color='orange', linestyle='--', linewidth=1, label=f'LSB ({fc-fm} Hz)')
plt.axvline(x=fc+fm, color='orange', linestyle='--', linewidth=1, label=f'USB ({fc+fm} Hz)')
plt.legend()

plt.tight_layout()
plt.savefig('graficas/senal_am_fft.png', dpi=150)
print("✓ Gráfica FFT guardada: graficas/senal_am_fft.png")
plt.close()

# ============================================
# MANTENER CONEXIÓN ACTIVA
# ============================================
print("\n" + "="*50)
print("  SEÑAL AM ACTIVA EN ESP32")
print("="*50)
print(f"  Puerto: {puerto_esp32}")
print(f"  El Arduino debe estar leyendo en COM8")
print("  Ahora puedes ejecutar el script de lectura")
print("  Presiona Ctrl+C para detener")
print("="*50 + "\n")

try:
    while True:
        time.sleep(1)
        # Verificar que el puerto sigue abierto
        if not puerto.is_open:
            print("⚠ Puerto cerrado inesperadamente")
            break
except KeyboardInterrupt:
    print("\n\nDeteniendo señal...")
    try:
        puerto.write(b"STOP\n")
        time.sleep(0.2)
    except:
        pass
    puerto.close()
    print("✓ Desconectado del ESP32")