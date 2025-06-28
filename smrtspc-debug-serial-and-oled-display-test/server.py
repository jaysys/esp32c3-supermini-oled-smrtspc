#
# pip install flask flask-cors
# python server.py
#


from flask import Flask, request, jsonify
from datetime import datetime
from flask_cors import CORS

app = Flask(__name__)
app.config['TEMPLATES_AUTO_RELOAD'] = True
app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 0
app.config['DEBUG'] = True
CORS(app)  # Enable CORS for all routes

# Store received data
received_data = {
    'last_update': None,
    'sensor_data': None,
    'history': []
}

@app.route('/api/data', methods=['POST'])
def receive_data():
    if request.is_json:
        data = request.get_json()
        client_ip = request.remote_addr
        # Add client IP to the data
        data_with_ip = data.copy()
        data_with_ip['client_ip'] = client_ip
        
        received_data['sensor_data'] = data_with_ip
        received_data['last_update'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        received_data['history'].append({
            'timestamp': received_data['last_update'],
            'data': data_with_ip
        })
        # Keep only last 10 entries
        if len(received_data['history']) > 10:
            received_data['history'] = received_data['history'][-10:]
        
        print(f"Received data from {client_ip}: {data}")
        return jsonify({"status": "success", "message": "Data received"}), 200
    return jsonify({"status": "error", "message": "Invalid data"}), 400

@app.route('/api/data', methods=['GET'])
def get_data():
    return jsonify(received_data)

@app.route('/')
def index():
    client_ip = request.remote_addr
    return f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32 Sensor Data</title>
        <meta http-equiv="refresh" content="2">
        <style>
            body {{ 
                font-family: Arial, sans-serif; 
                max-width: 800px; 
                margin: 0 auto; 
                padding: 20px; 
                background-color: #f5f5f5;
            }}
            .client-info {{
                background: #e9ecef;
                padding: 10px 15px;
                border-radius: 4px;
                margin-bottom: 20px;
                font-size: 0.9em;
                color: #495057;
            }}
            .container {{ 
                background: white; 
                padding: 20px; 
                border-radius: 8px; 
                box-shadow: 0 2px 4px rgba(0,0,0,0.1); 
                margin-bottom: 20px;
            }}
            .data-box {{ 
                background: #f8f9fa; 
                padding: 15px; 
                border-left: 4px solid #007bff; 
                margin: 10px 0; 
                border-radius: 4px;
            }}
            .timestamp {{ 
                color: #6c757d; 
                font-size: 0.9em; 
                margin-bottom: 10px;
            }}
            .value {{ 
                font-size: 1.2em; 
                font-weight: bold; 
                color: #28a745;
            }}
            .history-item {{ 
                padding: 10px; 
                border-bottom: 1px solid #eee; 
                margin: 5px 0;
            }}
            .history-item:last-child {{ 
                border-bottom: none; 
            }}
            .history-table {{
                width: 100%;
                border-collapse: collapse;
                margin-top: 10px;
            }}
            .history-table th, .history-table td {{
                border: 1px solid #dee2e6;
                padding: 8px 12px;
                text-align: left;
            }}
            .history-table th {{
                background-color: #f8f9fa;
                font-weight: bold;
            }}
            .history-table tr:nth-child(even) {{
                background-color: #f8f9fa;
            }}
            .history-table tr:hover {{
                background-color: #e9ecef;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>ESP32 Sensor Data</h1>
            <div class="client-info">
                <strong>Your IP:</strong> {client_ip}
            </div>
            <div id="current-data">
                <p>No data received yet...</p>
            </div>
        </div>

        <div class="container">
            <h2>History (Last 10 entries)</h2>
            <div id="history"></div>
        </div>

        <script>
            function updateData() {{
                fetch('/api/data')
                    .then(function(response) {{ return response.json(); }})
                    .then(function(data) {{
                        // Update current data
                        if (data.sensor_data) {{
                            var lastUpdate = data.last_update || 'N/A';
                            var clientIp = data.sensor_data.client_ip || 'N/A';
                            var sensor = data.sensor_data.sensor || 'N/A';
                            var val1 = data.sensor_data.value1 !== undefined ? data.sensor_data.value1 : 'N/A';
                            var val2 = data.sensor_data.value2 !== undefined ? data.sensor_data.value2 : 'N/A';
                            
                            var currentHtml = 
                                '<div class="data-box">' +
                                    '<div class="timestamp">' + lastUpdate + '</div>' +
                                    '<div>Client IP: <strong>' + clientIp + '</strong></div>' +
                                    '<div>Sensor: ' + sensor + '</div>' +
                                    '<div>Value 1: <span class="value">' + val1 + '</span></div>' +
                                    '<div>Value 2: <span class="value">' + val2 + '</span></div>' +
                                '</div>';
                            document.getElementById('current-data').innerHTML = currentHtml;
                        }}

                        // Update history table
                        var historyHtml = '';
                        if (data.history && data.history.length > 0) {{
                            // Create table header
                            historyHtml = `
                                <table class="history-table">
                                    <thead>
                                        <tr>
                                            <th>일시</th>
                                            <th>IP 주소</th>
                                            <th>Value 1</th>
                                            <th>Value 2</th>
                                        </tr>
                                    </thead>
                                    <tbody>`;
                            
                            // Add table rows with history data (newest first)
                            data.history.slice().reverse().forEach(function(item) {{
                                var timestamp = item.timestamp || 'N/A';
                                var ip = item.data.client_ip || 'N/A';
                                var val1 = item.data.value1 !== undefined ? item.data.value1 : 'N/A';
                                var val2 = item.data.value2 !== undefined ? item.data.value2 : 'N/A';
                                
                                historyHtml += `
                                    <tr>
                                        <td>${{timestamp}}</td>
                                        <td>${{ip}}</td>
                                        <td class="value">${{val1}}</td>
                                        <td class="value">${{val2}}</td>
                                    </tr>`;
                            }});
                            
                            // Close table
                            historyHtml += `
                                    </tbody>
                                </table>`;
                        }} else {{
                            historyHtml = '<p>No history data available</p>';
                        }}
                        document.getElementById('history').innerHTML = historyHtml;
                    }})
                    .catch(function(error) {{
                        console.error('Error fetching data:', error);
                    }});
            }}

            // Update data immediately and then every 2 seconds
            updateData();
            setInterval(updateData, 2000);
        </script>
        </body>
        </html>
        """

if __name__ == '__main__':
    print("Starting Flask server on http://0.0.0.0:5003")
    app.run(host='0.0.0.0', port=5003, debug=True, use_reloader=True, use_debugger=True)