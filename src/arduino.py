from flask import Flask, request, jsonify
import numpy as np
from collections import defaultdict, deque

app = Flask(__name__)

# Store sensor data and configurations in dictionaries
sensor_data = defaultdict(deque)
sensor_config = {'cbuffsize': 10, 'mode': 'passive'}

@app.route('/config/<config_name>', methods=['PUT'])
def set_config(config_name):
    value = request.get_data(as_text=True)
    if config_name in sensor_config:
        sensor_config[config_name] = value
        return '', 201
    else:
        return jsonify({'error': 'Configuration not found'}), 404

@app.route('/sensors/<int:sensor_id>', methods=['POST', 'DELETE'])
def manage_sensor_data(sensor_id):
    if request.method == 'POST':
        value = request.get_data(as_text=True)
        try:
            if len(sensor_data[sensor_id]) >= int(sensor_config['cbuffsize']):
                sensor_data[sensor_id].popleft() # maintain buffer size
            sensor_data[sensor_id].append(float(value))
            return '', 201
        except ValueError:
            return jsonify({'error': 'Invalid data format'}), 400
    elif request.method == 'DELETE':
        sensor_data[sensor_id].clear()
        return '', 201

@app.route('/sensors/<int:sensor_id>/<stat>', methods=['GET'])
def get_sensor_statistics(sensor_id, stat):
    if sensor_id not in sensor_data or len(sensor_data[sensor_id]) == 0:
        return jsonify({'error': 'No data for sensor'}), 404
    data = np.array(sensor_data[sensor_id])
    if stat == 'avg':
        return str(data.mean()), 200
    elif stat == 'stdev':
        return str(data.std()), 200
    elif stat == 'actual':
        return str(data[-1]), 200
    else:
        return jsonify({'error': 'Statistic not found'}), 404

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port=80)
