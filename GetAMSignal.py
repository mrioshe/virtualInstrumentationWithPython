import pyqtgraph as pg
from PyQt5.QtWidgets import QApplication
import numpy as np
import serial
from collections import deque
import sys
import threading
import time
from scipy.signal import butter, sosfiltfilt

# ============================================
# CONFIGURACIÓN
# ============================================
fs = 5000  # 5 kHz de muestreo (coincide con Arduino)
N = 10000  # 2 segundos de datos (5000 * 2)
buffer = deque([0]*N, maxlen=N)

# Puerto serial - ARDUINO (NO el ESP32)
PUERTO_SERIAL = 'COM7'  # Puerto del ARDUINO que recibe la señal
BAUD_RATE = 115200

# IMPORTANTE: El ESP32 debe estar en COM9 enviando la señal
# Este script lee desde el Arduino en COM8

# Filtros
fc_envolvente = 100  # Hz - Pasa-bajos para extraer envolvente de 50 Hz
orden = 4

# Crear filtro pasa-bajos para envolvente
sos_envolvente = butter(orden, fc_envolvente / (fs / 2), btype='low', output='sos')

# ============================================
# CONEXIÓN SERIAL
# ============================================
print(f"Intentando conectar a {PUERTO_SERIAL}...")
try:
    ser = serial.Serial(PUERTO_SERIAL, BAUD_RATE, timeout=1)
    time.sleep(2)  # Esperar a que Arduino se reinicie
    ser.reset_input_buffer()  # Limpiar buffer
    print(f"✓ Conectado a {PUERTO_SERIAL}")
except Exception as e:
    print(f"❌ Error al conectar: {e}")
    sys.exit(1)

# ============================================
# INTERFAZ GRÁFICA
# ============================================
app = QApplication([])
win = pg.GraphicsLayoutWidget(title="Análisis Señal AM - 1000Hz Portadora / 50Hz Envolvente")
win.resize(1400, 900)

# === Señal AM original ===
plot_am = win.addPlot(title="Señal AM Recibida (1000 Hz portadora)")
plot_am.setLabel('left', 'Voltaje', units='V')
plot_am.setLabel('bottom', 'Muestras')
plot_am.showGrid(x=True, y=True, alpha=0.3)
curve_am = plot_am.plot(pen=pg.mkPen('c', width=1))
label_am = pg.TextItem(color='cyan', anchor=(0, 1))
plot_am.addItem(label_am)

# === Envolvente ===
win.nextRow()
plot_envolvente = win.addPlot(title="Envolvente Extraída (50 Hz)")
plot_envolvente.setLabel('left', 'Voltaje', units='V')
plot_envolvente.setLabel('bottom', 'Muestras')
plot_envolvente.showGrid(x=True, y=True, alpha=0.3)
curve_envolvente = plot_envolvente.plot(pen=pg.mkPen('g', width=2))
label_envolvente = pg.TextItem(color='lime', anchor=(0, 1))
plot_envolvente.addItem(label_envolvente)

# === FFT ===
win.nextRow()
plot_fft = win.addPlot(title="Espectro de Frecuencia (FFT)")
plot_fft.setLabel('left', 'Magnitud', units='dB')
plot_fft.setLabel('bottom', 'Frecuencia', units='Hz')
plot_fft.setXRange(0, 1200)
plot_fft.setYRange(-60, 5)
plot_fft.showGrid(x=True, y=True, alpha=0.3)
curve_fft = plot_fft.plot(pen=pg.mkPen('m', width=1))

# Marcadores para las 3 frecuencias principales
marcadores_fft = []
colores = ['yellow', 'orange', 'red']
for i in range(3):
    line = pg.InfiniteLine(
        angle=90, 
        movable=False, 
        pen=pg.mkPen(colores[i], width=2, style=pg.QtCore.Qt.DashLine)
    )
    plot_fft.addItem(line)
    line.setVisible(False)
    marcadores_fft.append(line)

# Panel de información
info_text = pg.TextItem(
    html='<div style="background-color: rgba(0,0,0,150); padding: 10px; border-radius: 5px;">'
         '<span style="color: white; font-size: 12pt;">Iniciando...</span></div>',
    anchor=(0, 0)
)
plot_fft.addItem(info_text)
info_text.setPos(50, -5)

frecuencias = np.fft.fftfreq(N, 1/fs)[:N//2]

# ============================================
# VARIABLES DE CONTROL
# ============================================
datos_recibidos = 0
errores_lectura = 0
running = True

# ============================================
# HILO DE LECTURA SERIAL
# ============================================
def leer_serial():
    global datos_recibidos, errores_lectura, running
    
    while running:
        try:
            if ser.in_waiting > 0:
                linea = ser.readline().decode('utf-8').strip()
                
                if linea and linea.isdigit():
                    valor_adc = int(linea)
                    
                    # Validar rango (0-1023 para Arduino)
                    if 0 <= valor_adc <= 1023:
                        # Convertir ADC a voltaje (0-5V)
                        voltaje = (valor_adc / 1023.0) * 5.0
                        buffer.append(voltaje)
                        datos_recibidos += 1
                    else:
                        errores_lectura += 1
                else:
                    if linea:  # Solo contar si hay datos
                        errores_lectura += 1
                        
        except Exception as e:
            errores_lectura += 1
            print(f"Error en lectura: {e}")
            time.sleep(0.001)

# Iniciar hilo de lectura
hilo_lectura = threading.Thread(target=leer_serial, daemon=True)
hilo_lectura.start()
print("✓ Hilo de lectura iniciado")

# ============================================
# FUNCIONES DE PROCESAMIENTO
# ============================================
def extraer_envolvente(datos):
    """Extrae la envolvente usando filtro pasa-bajos"""
    try:
        envolvente = sosfiltfilt(sos_envolvente, datos)
        return envolvente
    except:
        return datos

def calcular_rms(datos):
    """Calcula el voltaje RMS"""
    return np.sqrt(np.mean(datos**2))

def encontrar_picos_fft(fft_vals, num_picos=3):
    """Encuentra las N frecuencias principales en la FFT"""
    # Evitar DC (frecuencia 0)
    fft_vals_copia = fft_vals.copy()
    fft_vals_copia[0:5] = 0  # Ignorar primeros bins (DC y ruido bajo)
    
    picos_idx = []
    fft_temp = fft_vals_copia.copy()
    
    for _ in range(num_picos):
        idx = np.argmax(fft_temp)
        if fft_temp[idx] > 0:
            picos_idx.append(idx)
            # Eliminar pico y vecinos (ventana de 50 bins)
            inicio = max(0, idx - 25)
            fin = min(len(fft_temp), idx + 25)
            fft_temp[inicio:fin] = 0
    
    return sorted(picos_idx)

# ============================================
# ACTUALIZACIÓN GRÁFICA
# ============================================
frames_procesados = 0

def actualizar():
    global frames_procesados
    
    # Convertir buffer a array
    datos_np = np.array(buffer)
    
    # Verificar que hay suficientes datos
    if len(datos_np) < orden * 6:
        info_text.setHtml(
            '<div style="background-color: rgba(0,0,0,150); padding: 10px; border-radius: 5px;">'
            f'<span style="color: yellow; font-size: 12pt;">Llenando buffer... ({len(datos_np)}/{N})</span></div>'
        )
        return
    
    frames_procesados += 1
    
    # Remover componente DC
    datos_centrados = datos_np - np.mean(datos_np)
    
    # Extraer envolvente
    envolvente = extraer_envolvente(datos_centrados)
    
    # Calcular RMS
    rms_am = calcular_rms(datos_centrados)
    rms_envolvente = calcular_rms(envolvente)
    
    # Actualizar gráfica de señal AM
    curve_am.setData(datos_centrados)
    y_max_am = np.max(np.abs(datos_centrados))
    if y_max_am > 0:
        label_am.setText(f"RMS: {rms_am:.3f} V | Pico: {y_max_am:.3f} V")
        label_am.setPos(100, y_max_am * 0.85)
    
    # Actualizar gráfica de envolvente
    curve_envolvente.setData(envolvente)
    y_max_env = np.max(np.abs(envolvente))
    if y_max_env > 0:
        label_envolvente.setText(f"RMS: {rms_envolvente:.3f} V | Pico: {y_max_env:.3f} V")
        label_envolvente.setPos(100, y_max_env * 0.85)
    
    # FFT (calcular cada 5 frames para mejor rendimiento)
    if frames_procesados % 5 == 0:
        # Aplicar ventana
        windowed = datos_centrados * np.hanning(len(datos_centrados))
        
        # Calcular FFT
        fft_vals = np.abs(np.fft.fft(windowed))[:N//2]
        
        # Normalizar y convertir a dB
        max_fft = np.max(fft_vals)
        if max_fft > 0:
            fft_db = 20 * np.log10(fft_vals / max_fft + 1e-12)
            fft_db = np.clip(fft_db, -60, 5)
            curve_fft.setData(frecuencias, fft_db)
            
            # Encontrar y marcar las 3 frecuencias principales
            picos_idx = encontrar_picos_fft(fft_vals, num_picos=3)
            
            # Construir texto de información
            info_html = '<div style="background-color: rgba(0,0,0,180); padding: 10px; border-radius: 5px;">'
            info_html += f'<span style="color: cyan; font-size: 11pt; font-weight: bold;">RMS Total: {rms_am:.3f} V</span><br>'
            info_html += f'<span style="color: lime; font-size: 11pt;">RMS Envolvente: {rms_envolvente:.3f} V</span><br><br>'
            info_html += '<span style="color: white; font-size: 10pt; font-weight: bold;">Frecuencias principales:</span><br>'
            
            for i, idx in enumerate(picos_idx):
                freq = frecuencias[idx]
                mag = fft_db[idx]
                marcadores_fft[i].setValue(freq)
                marcadores_fft[i].setVisible(True)
                
                color = colores[i]
                info_html += f'<span style="color: {color}; font-size: 10pt;">● f{i+1}: {freq:.1f} Hz ({mag:.1f} dB)</span><br>'
            
            info_html += f'<br><span style="color: gray; font-size: 9pt;">Datos: {datos_recibidos} | Errores: {errores_lectura}</span>'
            info_html += '</div>'
            
            info_text.setHtml(info_html)

# ============================================
# TIMER Y EJECUCIÓN
# ============================================
timer = pg.QtCore.QTimer()
timer.timeout.connect(actualizar)
timer.start(50)  # Actualizar cada 50 ms (20 FPS)

win.show()
print("\n✓ Interfaz gráfica iniciada")
print("  Esperando datos del Arduino...")
print("  Presiona Ctrl+C o cierra la ventana para salir\n")

# Ejecutar aplicación
try:
    sys.exit(app.exec_())
except KeyboardInterrupt:
    print("\n\nCerrando...")
finally:
    running = False
    ser.close()
    print("✓ Puerto serial cerrado")