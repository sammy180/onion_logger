from flask import Flask, render_template, request, redirect, url_for
import smbus
import time

# Initialize the bus for SMBus
DEVICE_BUS = 1
DEVICE_ADDR = 0x13
bus = smbus.SMBus(DEVICE_BUS)

# Keep track of relay states (initially off)
relay_states = [0, 0, 0, 0]

# Initialize Flask app
app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html', relay_states=relay_states)

@app.route('/toggle/<int:relay_id>', methods=['POST'])
def toggle_relay(relay_id):
    if 1 <= relay_id <= 4:
        relay_index = relay_id - 1
        # Toggle relay state
        if relay_states[relay_index] == 0:
            bus.write_byte_data(DEVICE_ADDR, relay_id, 0xFF)  # Turn on relay
            relay_states[relay_index] = 1
        else:
            bus.write_byte_data(DEVICE_ADDR, relay_id, 0x00)  # Turn off relay
            relay_states[relay_index] = 0
        time.sleep(0.1)  # Small delay for stability
    return redirect(url_for('index'))

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)

# HTML template (index.html)
# Save this as "templates/index.html" in the same directory as your Flask app.
"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Relay Control</title>
    <style>
        .button {
            width: 150px;
            height: 150px;
            font-size: 24px;
            margin: 20px;
        }
    </style>
</head>
<body>
    <h1>Relay Control Interface</h1>
    <div>
        {% for i in range(4) %}
            <form action="/toggle/{{ i + 1 }}" method="post">
                <button class="button" type="submit" style="background-color: {% if relay_states[i] == 1 %}green{% else %}red{% endif %};">
                    Relay {{ i + 1 }}
                </button>
            </form>
        {% endfor %}
    </div>
</body>
</html>
"""

