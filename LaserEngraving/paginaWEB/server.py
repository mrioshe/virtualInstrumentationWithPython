from flask import Flask, request, jsonify
app = Flask(__name__)

@app.route('/')
def index():
    return open("page.html").read()

@app.route('/sendText', methods=['POST'])
def send_text():
    text = request.data.decode('utf-8')
    print("Texto recibido:", text)
    # Aquí podrías reenviar al ESP32 vía serial, MQTT, etc.
    return 'OK'

@app.route('/getCNCStatus')
def get_cnc_status():
    return "Listo"

@app.route('/getLaserStatus')
def get_laser_status():
    return "OFF"

@app.route('/sensorData')
def sensor_data():
    return jsonify({
        "sensor1": 50 + 10,   # Ejemplo de dato simulado
        "sensor2": 30 + 5
    })

if __name__ == '__main__':
    app.run(port=3000)
