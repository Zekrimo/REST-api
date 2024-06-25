from flask import Flask, request, jsonify
import sqlite3
import numpy as np

app = Flask(__name__)

DATABASE = 'sensor_data.db'

def get_db():
    conn = sqlite3.connect(DATABASE)
    return conn

def init_db():
    with app.app_context():
        db = get_db()
        cursor = db.cursor()
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS measurements (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                sensor1 REAL,
                sensor2 REAL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        db.commit()

@app.route('/data', methods=['POST'])
def post_data():
    data = request.data.decode('utf-8').strip().split()
    if len(data) != 2:
        return "Invalid data", 400

    try:
        sensor1 = float(data[0])
        sensor2 = float(data[1])
    except ValueError:
        return "Invalid data format", 400

    db = get_db()
    cursor = db.cursor()
    cursor.execute('INSERT INTO measurements (sensor1, sensor2) VALUES (?, ?)', (sensor1, sensor2))
    db.commit()
    return "Data received", 201

@app.route('/statistics', methods=['GET'])
def get_statistics():
    db = get_db()
    cursor = db.cursor()
    cursor.execute('SELECT sensor1, sensor2 FROM measurements')
    rows = cursor.fetchall()

    if len(rows) < 3:
        return "Not enough data", 400

    sensor1_values = [row[0] for row in rows]
    sensor2_values = [row[1] for row in rows]

    coeffs = np.polyfit(sensor1_values, sensor2_values, 2)
    polynomial = np.poly1d(coeffs)
    yhat = polynomial(sensor1_values)
    ybar = np.mean(sensor2_values)
    ssreg = np.sum((yhat - ybar) ** 2)
    sstot = np.sum((sensor2_values - ybar) ** 2)
    r2 = ssreg / sstot

    result = f"{coeffs[2]:.1f} {coeffs[1]:.1f} {coeffs[0]:.1f} {r2:.1f}"

    return result, 200

@app.route('/statistics', methods=['DELETE'])
def delete_statistics():
    db = get_db()
    cursor = db.cursor()
    cursor.execute('DELETE FROM measurements')
    db.commit()
    return "All data deleted", 201

if __name__ == '__main__':
    init_db()
    app.run(host='0.0.0.0', port=5000)
