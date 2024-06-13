from flask import Flask, request, Response, jsonify, send_file # pip install flask
from flask_basicauth import BasicAuth # pip install flask-basicauth
from datetime import datetime
import os.path
import sys
from urllib.parse import unquote

app = Flask(__name__)

# default 
PORT = 8080
FIRMWARE_PATH = "./build/" 
LOG_PATH = "./log/" 

# override from command line
if len(sys.argv)>= 1:
    PORT = sys.argv[1]

if len(sys.argv) >= 2:
    FIRMWARE_PATH = sys.argv[2]

if len(sys.argv) >= 3:
    LOG_PATH = sys.argv[3]

# Configure basic authentication
app.config['BASIC_AUTH_USERNAME'] = "admin"
app.config['BASIC_AUTH_PASSWORD'] = "myadminpw"
basic_auth = BasicAuth(app)

def get_file_mtime(file_path):
    mtime = os.path.getmtime(file_path)
    timestamp = datetime.fromtimestamp(mtime)
    timestamp = timestamp.replace(microsecond=0)
    return timestamp

def parse_version(version_string):
    parts = version_string.split('-')
    if len(parts) != 3:
        raise ValueError('Invalid version string')
    filename, date_str, time_str = parts
    date_time_str = f"{date_str.strip()} {time_str.strip()}"
    try:
        timestamp = datetime.strptime(date_time_str, '%b %d %Y %H:%M:%S')
        timestamp = timestamp.replace(microsecond=0)
    except ValueError:
        raise ValueError('Invalid version string')
    return filename, timestamp

# test: curl -H "x-ESP8266-version: ESP8266_Template.ino-May  7 2023-15:26:18" -u admin:myadminpw --output firmware.ino.bin http://localhost:8080/firmware
@app.route('/firmware', methods=['GET'])
@basic_auth.required
def get_firmware():
    firmware_version = request.headers.get('x-ESP8266-version')
    if not firmware_version:
        return 'Firmware version not specified', 400
    try:
        filename, firmware_version_mtime = parse_version(firmware_version)
    except ValueError:
        return 'Invalid firmware version', 400
    filepath = os.path.join( FIRMWARE_PATH, filename + '.bin')
    if not os.path.isfile(filepath):
        return 'Firmware not found', 404
    firmware_file_mtime = get_file_mtime(filepath)
    # difference needs to be min 60s as there are some small time differences
    if (firmware_file_mtime - firmware_version_mtime).total_seconds() >= 60:
        return send_file(filepath, as_attachment=True, mimetype='application/octet-stream')
    else:
        return '', 304  # Not Modified
    
# curl -X POST -H "Content-Type: application/json" -u admin:myadminpw -d @log_entry.json http://localhost:8080/log
@app.route('/log', methods=['POST'])
@basic_auth.required
def log_handler():
    data = request.json
    log_id = data.get('id')
    log_content = data.get('content')
    if log_content:
        log_content = unquote(log_content)

    if log_id is None or log_content is None:
        return jsonify({'error': 'id and content are required'}), 400

    log_filename = f'{LOG_PATH}{log_id}.log'

    try:
        # Check if the log file exists
        if os.path.exists(log_filename):
            mode = 'a'
        else:
            mode = 'w'

        # Open the log file in the determined mode
        with open(log_filename, mode) as log_file:
            log_file.write(log_content)

    except Exception as e:
        return jsonify({'error': str(e)}), 500

    return jsonify({'message': 'Log appended successfully'}), 200
    
if __name__ == '__main__':
    app.run(debug=True,host='0.0.0.0',port=PORT)


