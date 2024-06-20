from flask import Flask, request, jsonify
import os
import numpy as np
from sklearn.linear_model import LinearRegression

app = Flask(__name__)

# This will store our data points
data_points = []

@app.route('/data', methods=['POST'])
def add_data():
    try:
        # Extract data from request
        content = request.get_data(as_text=True)
        numbers = content.strip().split(' ')
        x, y = map(int, numbers)
        data_points.append([x, y])
        return '', 201
    except Exception as e:
        return str(e), 400

@app.route('/statistics', methods=['DELETE'])
def delete_statistics():
    data_points.clear() # Clear all data points
    return '', 201

@app.route('/statistics', methods=['GET'])
def get_statistics():
    if len(data_points) < 2:
        return "Insufficient data", 400
    try:
        # Convert data points to numpy array for analysis
        X = np.array(data_points)[:, 0].reshape(-1, 1) # Feature
        y = np.array(data_points)[:, 1] # Target

        # Perform linear regression
        model = LinearRegression().fit(X, y)
        b0 = model.intercept_
        b1 = model.coef_[0]
        R2 = model.score(X, y)

        # Placeholder for b2, assuming a simple linear model for now
        b2 = 0.0 

        return f"{b0} {b1} {b2} {R2}", 200
    except Exception as e:
        return str(e), 400

if __name__ == "__main__":
    app.run(debug=True, host=os.getenv('IP', '0.0.0.0'), port=int(os.getenv('PORT', 8080)))
