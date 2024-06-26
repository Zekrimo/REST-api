import time
import requests

# Configuration
ARDUINO_URL = "http://192.168.1.21:80"
RASPBERRY_PI_URL = "192.168.219.134:5000"

def fetch_data_from_arduino(sensor_id):
    try:
        response_avg = requests.get(f"{ARDUINO_URL}/sensors/{sensor_id}/avg")
        response_avg.raise_for_status()
        avg = response_avg.json()['get_avg']

        return avg
    except requests.RequestException as e:
        print(f"Failed to fetch data from Arduino: {e}")
        return None

def send_data_to_raspberry_pi(avg_sensor1, avg_sensor2):
    try:
        data = f"{avg_sensor1} {avg_sensor2}"
        response = requests.post(f"{RASPBERRY_PI_URL}/data", data=data)
        response.raise_for_status()
        print("Data sent to Raspberry Pi")
    except requests.RequestException as e:
        print(f"Failed to send data to Raspberry Pi: {e}")

def main():
    while True:
        avg_sensor1 = fetch_data_from_arduino(1)
        avg_sensor2 = fetch_data_from_arduino(2)

        if avg_sensor1 is not None and avg_sensor2 is not None:
            send_data_to_raspberry_pi(avg_sensor1, avg_sensor2)
        
        # Sleep for 12 seconds before next fetch
        time.sleep(12)

if __name__ == "__main__":
    main()
