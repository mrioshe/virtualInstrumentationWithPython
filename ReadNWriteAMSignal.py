import pyqtgraph as pg
from PyQt5.QtWidgets import QApplication, QPushButton, QVBoxLayout, QWidget, QHBoxLayout, QLabel, QTextEdit
from PyQt5.QtCore import QTimer
import numpy as np
import serial
from collections import deque
import sys
import threading
import time
from scipy.signal import butter, sosfiltfilt, hilbert
import os

# ============================================
# CONFIGURACIÓN
# ============================================
# Puertos seriales
PUERTO_ESP32 = 'COM9'   # ESP32 que GENERA la señal AM
PUERTO_ARDUINO = 'COM7' # Arduino que LEE la señal AM
BAUD_RATE = 115200

# Parámetros de la señal AM a generar
fc = 1000    # Frecuencia portadora (Hz)
fm = 50      # Frecuencia moduladora (Hz)
m = 0.5      # Índice de modulación (0-1)
offset = 1.65  # Offset DC (V)
amplitud = 1.0 # Amplitud (V)

# Parámetros de lectura
fs = 5000  # 5 kHz de muestreo
N = 10000  # 2 segundos de datos
buffer = deque([0]*N, maxlen=N)

# Filtros mejorados para extraer envolvente
# Usar frecuencia de corte muy baja para extraer solo la moduladora de 50 Hz
fc_envolvente = 80  # Hz - Pasa-bajos más agresivo
orden = 6  # Orden más alto para mejor filtrado
sos_envolvente = butter(orden, fc_envolvente / (fs / 2), btype='low', output='sos')

# Variables de control
datos_recibidos = 0
errores_lectura = 0
running = True
generando = False

# Conexiones seriales
ser_esp32 = None
ser_arduino = None

# ============================================
# FUNCIONES DE CALCULO DE DISTANCIA
# ============================================
def calcular_distancia_logaritmica(voltaje_rms):
    """Calcula distancia usando fórmula logarítmica: y = -2.802ln(x) - 1.927"""
    if voltaje_rms <= 0:
        return float('inf')  # Distancia infinita si no hay voltaje
    distancia = -2.802 * np.log(voltaje_rms) - 1.927
    return max(0, distancia)  # No permitir distancias negativas

def calcular_distancia_polinomica(voltaje_rms):
    """Calcula distancia usando fórmula polinómica: y = 57.077x² - 44.634x + 8.9461"""
    distancia = 57.077 * (voltaje_rms**2) - 44.634 * voltaje_rms + 8.9461
    return max(0, distancia)  # No permitir distancias negativas

# ============================================
# CONEXIÓN A PUERTOS SERIALES
# ============================================
def conectar_puertos():
    global ser_esp32, ser_arduino
    
    print("Conectando a dispositivos...\n")
    
    # Conectar ESP32
    try:
        ser_esp32 = serial.Serial(PUERTO_ESP32, BAUD_RATE, timeout=1)
        time.sleep(2)  # Esperar reset
        ser_esp32.reset_input_buffer()
        print(f"✓ ESP32 conectado en {PUERTO_ESP32}")
    except Exception as e:
        print(f"❌ Error al conectar ESP32: {e}")
        return False
    
    # Conectar Arduino
    try:
        ser_arduino = serial.Serial(PUERTO_ARDUINO, BAUD_RATE, timeout=1)
        time.sleep(2)  # Esperar reset
        ser_arduino.reset_input_buffer()
        print(f"✓ Arduino conectado en {PUERTO_ARDUINO}")
    except Exception as e:
        print(f"❌ Error al conectar Arduino: {e}")
        ser_esp32.close()
        return False
    
    print("\n✓ Todos los dispositivos conectados\n")
    return True

# ============================================
# CONTROL DE GENERACIÓN DE SEÑAL
# ============================================
def iniciar_generacion():
    global generando
    
    if ser_esp32 and not generando:
        comando = f"START,{fc},{fm},{m},{offset},{amplitud}\n"
        ser_esp32.write(comando.encode())
        generando = True
        print(f"✓ Señal AM iniciada: fc={fc}Hz, fm={fm}Hz, m={m}")
        return True
    return False

def detener_generacion():
    global generando
    
    if ser_esp32 and generando:
        ser_esp32.write(b"STOP\n")
        generando = False
        print("✓ Señal AM detenida")
        return True
    return False

# ============================================
# HILO DE LECTURA SERIAL (ARDUINO)
# ============================================
def leer_serial():
    global datos_recibidos, errores_lectura, running
    
    print("✓ Hilo de lectura iniciado")
    
    while running:
        try:
            if ser_arduino and ser_arduino.in_waiting > 0:
                linea = ser_arduino.readline().decode('utf-8').strip()
                
                if linea and linea.isdigit():
                    valor_adc = int(linea)
                    
                    if 0 <= valor_adc <= 1023:
                        voltaje = (valor_adc / 1023.0) * 5.0
                        buffer.append(voltaje)
                        datos_recibidos += 1
                    else:
                        errores_lectura += 1
                else:
                    if linea:
                        errores_lectura += 1
                        
        except Exception as e:
            errores_lectura += 1
            time.sleep(0.001)

# ============================================
# FUNCIONES DE PROCESAMIENTO MEJORADAS
# ============================================
def extraer_envolvente_hilbert(datos):
    """Extrae la envolvente usando Transformada de Hilbert"""
    try:
        # Método Hilbert - más preciso para señales AM
        analitica = hilbert(datos)
        envolvente = np.abs(analitica)
        
        # Aplicar filtro pasa-bajos adicional para suavizar
        envolvente_filtrada = sosfiltfilt(sos_envolvente, envolvente)
        
        return envolvente_filtrada
    except:
        return datos

def calcular_rms(datos):
    return np.sqrt(np.mean(datos**2))

def encontrar_picos_fft(fft_vals, num_picos=3):
    fft_vals_copia = fft_vals.copy()
    fft_vals_copia[0:5] = 0
    
    picos_idx = []
    fft_temp = fft_vals_copia.copy()
    
    for _ in range(num_picos):
        idx = np.argmax(fft_temp)
        if fft_temp[idx] > 0:
            picos_idx.append(idx)
            inicio = max(0, idx - 25)
            fin = min(len(fft_temp), idx + 25)
            fft_temp[inicio:fin] = 0
    
    return sorted(picos_idx)

# ============================================
# INTERFAZ GRÁFICA OPTIMIZADA
# ============================================
class VentanaPrincipal(QWidget):
    def __init__(self):
        super().__init__()
        self.frames_procesados = 0
        self.ultimo_fft_time = time.time()
        self.init_ui()
    
    def init_ui(self):
        self.setWindowTitle("Generador y Analizador de Señal AM")
        self.setGeometry(100, 100, 1600, 1000)
        
        layout_principal = QHBoxLayout()
        
        # ===== COLUMNA IZQUIERDA: GRÁFICAS =====
        layout_graficas = QVBoxLayout()
        
        # Panel de control
        panel_control = QHBoxLayout()
        
        self.btn_iniciar = QPushButton("▶ Iniciar Generación")
        self.btn_iniciar.clicked.connect(self.on_iniciar)
        self.btn_iniciar.setStyleSheet("background-color: #2ecc71; color: white; font-weight: bold; padding: 10px;")
        
        self.btn_detener = QPushButton("⬛ Detener Generación")
        self.btn_detener.clicked.connect(self.on_detener)
        self.btn_detener.setStyleSheet("background-color: #e74c3c; color: white; font-weight: bold; padding: 10px;")
        self.btn_detener.setEnabled(False)
        
        self.label_estado = QLabel("Estado: Esperando...")
        self.label_estado.setStyleSheet("font-size: 14pt; font-weight: bold;")
        
        panel_control.addWidget(self.btn_iniciar)
        panel_control.addWidget(self.btn_detener)
        panel_control.addWidget(self.label_estado)
        panel_control.addStretch()
        
        layout_graficas.addLayout(panel_control)
        
        # Gráficas
        self.graphics_widget = pg.GraphicsLayoutWidget()
        
        # === Señal AM original ===
        self.plot_am = self.graphics_widget.addPlot(title="Señal AM Recibida (1000 Hz portadora)")
        self.plot_am.setLabel('left', 'Voltaje', units='V')
        self.plot_am.setLabel('bottom', 'Muestras')
        self.plot_am.showGrid(x=True, y=True, alpha=0.3)
        self.curve_am = self.plot_am.plot(pen=pg.mkPen('c', width=1))
        
        # === Envolvente ===
        self.graphics_widget.nextRow()
        self.plot_envolvente = self.graphics_widget.addPlot(title="Envolvente Extraída (Hilbert + Filtro)")
        self.plot_envolvente.setLabel('left', 'Voltaje', units='V')
        self.plot_envolvente.setLabel('bottom', 'Muestras')
        self.plot_envolvente.showGrid(x=True, y=True, alpha=0.3)
        self.curve_envolvente = self.plot_envolvente.plot(pen=pg.mkPen('g', width=2))
        
        # === FFT ===
        self.graphics_widget.nextRow()
        self.plot_fft = self.graphics_widget.addPlot(title="Espectro de Frecuencia (FFT)")
        self.plot_fft.setLabel('left', 'Magnitud', units='dB')
        self.plot_fft.setLabel('bottom', 'Frecuencia', units='Hz')
        self.plot_fft.setXRange(0, 1500)
        self.plot_fft.setYRange(-60, 5)
        self.plot_fft.showGrid(x=True, y=True, alpha=0.3)
        self.curve_fft = self.plot_fft.plot(pen=pg.mkPen('m', width=1))
        
        # Marcadores FFT
        self.marcadores_fft = []
        colores = ['yellow', 'orange', 'red']
        for i in range(3):
            line = pg.InfiniteLine(
                angle=90, 
                movable=False, 
                pen=pg.mkPen(colores[i], width=2, style=pg.QtCore.Qt.DashLine)
            )
            self.plot_fft.addItem(line)
            line.setVisible(False)
            self.marcadores_fft.append(line)
        
        self.frecuencias = np.fft.fftfreq(N, 1/fs)[:N//2]
        
        layout_graficas.addWidget(self.graphics_widget)
        
        # ===== COLUMNA DERECHA: PANEL DE INFORMACIÓN =====
        panel_info_widget = QWidget()
        panel_info_widget.setMaximumWidth(350)
        panel_info_widget.setMinimumWidth(350)
        layout_info = QVBoxLayout()
        
        # Título del panel
        titulo_info = QLabel("📊 INFORMACIÓN DE ANÁLISIS")
        titulo_info.setStyleSheet("""
            background-color: #2c3e50;
            color: white;
            padding: 15px;
            font-size: 14pt;
            font-weight: bold;
            border-radius: 5px;
        """)
        layout_info.addWidget(titulo_info)
        
        # Área de texto para información
        self.info_text = QTextEdit()
        self.info_text.setReadOnly(True)
        self.info_text.setStyleSheet("""
            QTextEdit {
                background-color: #1a1a1a;
                color: #00ff00;
                font-family: 'Courier New', monospace;
                font-size: 11pt;
                padding: 10px;
                border: 2px solid #2c3e50;
                border-radius: 5px;
            }
        """)
        layout_info.addWidget(self.info_text)
        
        panel_info_widget.setLayout(layout_info)
        
        # Agregar columnas al layout principal
        layout_principal.addLayout(layout_graficas, stretch=3)
        layout_principal.addWidget(panel_info_widget, stretch=1)
        
        self.setLayout(layout_principal)
        
        # Timer de actualización - REDUCIR FRECUENCIA
        self.timer = QTimer()
        self.timer.timeout.connect(self.actualizar)
        self.timer.start(100)  # 10 FPS (antes 50ms = 20 FPS)
    
    def on_iniciar(self):
        if iniciar_generacion():
            self.btn_iniciar.setEnabled(False)
            self.btn_detener.setEnabled(True)
            self.label_estado.setText("Estado: ✓ Generando señal")
            self.label_estado.setStyleSheet("font-size: 14pt; font-weight: bold; color: #2ecc71;")
    
    def on_detener(self):
        if detener_generacion():
            self.btn_iniciar.setEnabled(True)
            self.btn_detener.setEnabled(False)
            self.label_estado.setText("Estado: ⬛ Detenido")
            self.label_estado.setStyleSheet("font-size: 14pt; font-weight: bold; color: #e74c3c;")
    
    def actualizar(self):
        datos_np = np.array(buffer)
        
        if len(datos_np) < orden * 6:
            self.info_text.setHtml(f"""
                <div style='color: yellow; font-size: 12pt;'>
                <b>⏳ LLENANDO BUFFER...</b><br><br>
                Progreso: {len(datos_np)}/{N}<br>
                {int((len(datos_np)/N)*100)}% completado
                </div>
            """)
            return
        
        self.frames_procesados += 1
        
        # Remover DC y procesar
        datos_centrados = datos_np - np.mean(datos_np)
        
        # Extraer envolvente con método Hilbert (más preciso)
        envolvente = extraer_envolvente_hilbert(datos_centrados)
        
        rms_am = calcular_rms(datos_centrados)
        rms_envolvente = calcular_rms(envolvente)
        
        # CALCULAR DISTANCIAS
        distancia_log = calcular_distancia_logaritmica(rms_envolvente)
        distancia_poli = calcular_distancia_polinomica(rms_envolvente)
        
        # Actualizar señal AM (diezmar para mejor rendimiento)
        step = 5 if len(datos_centrados) > 5000 else 1
        self.curve_am.setData(datos_centrados[::step])
        
        # Actualizar envolvente
        self.curve_envolvente.setData(envolvente[::step])
        
        # FFT - OPTIMIZADO: calcular cada 10 frames en lugar de 5
        tiempo_actual = time.time()
        if self.frames_procesados % 10 == 0 or (tiempo_actual - self.ultimo_fft_time) > 0.5:
            self.ultimo_fft_time = tiempo_actual
            
            # Diezmar señal para FFT más rápido
            datos_diezmados = datos_centrados[::2]  # Reducir a la mitad
            
            # Aplicar ventana
            windowed = datos_diezmados * np.hanning(len(datos_diezmados))
            
            # Calcular FFT
            fft_vals = np.abs(np.fft.fft(windowed))[:len(datos_diezmados)//2]
            frecuencias_diezmadas = np.fft.fftfreq(len(datos_diezmados), 1/fs)[:len(datos_diezmados)//2]
            
            # Normalizar y convertir a dB
            max_fft = np.max(fft_vals)
            if max_fft > 0:
                fft_db = 20 * np.log10(fft_vals / max_fft + 1e-12)
                fft_db = np.clip(fft_db, -60, 5)
                self.curve_fft.setData(frecuencias_diezmadas, fft_db)
                
                # Encontrar picos
                picos_idx = encontrar_picos_fft(fft_vals, num_picos=3)
                
                # Actualizar panel de información
                info_html = f"""
                <div style='color: #00ff00;'>
                <h2 style='color: cyan; border-bottom: 2px solid cyan;'>MEDICIONES RMS</h2>
                <p style='font-size: 13pt;'>
                <b style='color: cyan;'>RMS Total:</b> {rms_am:.4f} V<br>
                <b style='color: lime;'>RMS Envolvente:</b> {rms_envolvente:.4f} V<br>
                </p>
                
                <h2 style='color: #FF5722; border-bottom: 2px solid #FF5722; margin-top: 20px;'>DISTANCIA ESTIMADA</h2>
                <p style='font-size: 13pt; color: #FF5722;'>
                <b>Voltaje RMS envolvente:</b> {rms_envolvente:.4f} V<br>
                <b>Distancia (logarítmica):</b> {distancia_log:.2f} cm<br>
                &nbsp;&nbsp;<i>Fórmula: y = -2.802·ln(x) - 1.927</i><br>
                <b>Distancia (polinómica):</b> {distancia_poli:.2f} cm<br>
                &nbsp;&nbsp;<i>Fórmula: y = 57.077x² - 44.634x + 8.9461</i>
                </p>
                
                <h2 style='color: yellow; border-bottom: 2px solid yellow; margin-top: 20px;'>FRECUENCIAS PRINCIPALES</h2>
                """
                
                colores_html = ['yellow', 'orange', 'red']
                nombres = ['Portadora', 'Banda Lateral 1', 'Banda Lateral 2']
                
                for i, idx in enumerate(picos_idx):
                    freq = frecuencias_diezmadas[idx]
                    mag = fft_db[idx]
                    self.marcadores_fft[i].setValue(freq)
                    self.marcadores_fft[i].setVisible(True)
                    
                    info_html += f"""
                    <p style='font-size: 12pt; color: {colores_html[i]};'>
                    <b>● {nombres[i]}:</b><br>
                    &nbsp;&nbsp;&nbsp;Frecuencia: <b>{freq:.1f} Hz</b><br>
                    &nbsp;&nbsp;&nbsp;Magnitud: <b>{mag:.1f} dB</b>
                    </p>
                    """
                
                # Calcular índice de modulación estimado
                if len(picos_idx) >= 3:
                    portadora_mag = fft_vals[picos_idx[0]]
                    lateral_mag = (fft_vals[picos_idx[1]] + fft_vals[picos_idx[2]]) / 2
                    m_estimado = (2 * lateral_mag) / portadora_mag if portadora_mag > 0 else 0
                    
                    info_html += f"""
                    <h2 style='color: orange; border-bottom: 2px solid orange; margin-top: 20px;'>ÍNDICE DE MODULACIÓN</h2>
                    <p style='font-size: 13pt; color: orange;'>
                    <b>m estimado:</b> {m_estimado:.3f} ({m_estimado*100:.1f}%)<br>
                    <b>m teórico:</b> {m:.3f} ({m*100:.1f}%)
                    </p>
                    """
                
                info_html += f"""
                <h2 style='color: gray; border-bottom: 2px solid gray; margin-top: 20px;'>ESTADÍSTICAS</h2>
                <p style='font-size: 10pt; color: gray;'>
                Datos recibidos: {datos_recibidos}<br>
                Errores: {errores_lectura}<br>
                FPS: {1000/100:.0f} Hz<br>
                Frames: {self.frames_procesados}
                </p>
                </div>
                """
                
                self.info_text.setHtml(info_html)

# ============================================
# MAIN
# ============================================
if __name__ == "__main__":
    # Conectar puertos
    if not conectar_puertos():
        print("\n❌ No se pudieron conectar los dispositivos")
        sys.exit(1)
    
    # Iniciar hilo de lectura
    hilo_lectura = threading.Thread(target=leer_serial, daemon=True)
    hilo_lectura.start()
    
    # Crear aplicación
    app = QApplication(sys.argv)
    ventana = VentanaPrincipal()
    ventana.show()
    
    print("\n✓ Interfaz gráfica iniciada")
    print("  Presiona 'Iniciar Generación' para comenzar\n")
    
    # Ejecutar
    try:
        sys.exit(app.exec_())
    except KeyboardInterrupt:
        print("\n\nCerrando...")
    finally:
        running = False
        if ser_esp32:
            ser_esp32.write(b"STOP\n")
            ser_esp32.close()
        if ser_arduino:
            ser_arduino.close()
        print("✓ Puertos seriales cerrados")