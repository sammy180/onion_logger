from flask import Flask, render_template, jsonify, request, session
from sqlalchemy.ext.automap import automap_base
from sqlalchemy.orm import Session
from sqlalchemy import create_engine, desc
from datetime import datetime
import os

#DATABASE_URL = "sqlite:////home/sammy/Active workspace/onion_logger/sensor_data2.db"
DATABASE_URL = "sqlite:////home/pi/onion_logger/sensor_data.db"

Base = automap_base()

engine = create_engine(DATABASE_URL, connect_args={'check_same_thread': False})

Base.prepare(engine, reflect=True)

SensorData = Base.classes.SensorData

db_session = Session(engine)

app = Flask(import_name=__name__)
app.secret_key = 'your_secret_key'  # Set a secret key for session management

# Read headers from file
headers = []
try:
    with open("headers.txt", "r") as file:
        headers = [line.strip() for line in file.readlines()]
    print(f"Headers loaded: {headers}")
except FileNotFoundError:
    print("Failed to open headers.txt")
    exit(1)

def get_scroll_headers():
    try:
        # Get absolute path
        script_dir = os.path.dirname(os.path.abspath(__file__))
        file_path = os.path.join(script_dir, "scroll_headers.txt")
        
        print(f"Attempting to open: {file_path}")  # Debug print
        
        with open(file_path, "r") as file:
            headers = [line.strip() for line in file.readlines()]
        print(f"Scroll headers loaded: {headers}")
        return headers
    except FileNotFoundError as e:
        print(f"Failed to open scroll_headers.txt: {e}")
        print(f"Current working directory: {os.getcwd()}")  # Debug print
        exit(1)
    except Exception as e:
        print(f"Unexpected error reading scroll_headers.txt: {e}")
        exit(1)

@app.route('/')
def index():
    try:
        # Get scroll headers
        scroll_headers = get_scroll_headers()
        
        # Initialize defaults
        box_ids = session.get('box_ids', [])
        if not box_ids:
            # Query for unique BoxIDs
            unique_box_ids = []
            results = db_session.query(SensorData.BoxID).order_by(desc(SensorData.id)).all()
            
            for result in results:
                if result.BoxID not in unique_box_ids:
                    unique_box_ids.append(result.BoxID)
                if len(unique_box_ids) == 4:
                    break
                    
            box_ids = sorted(unique_box_ids)
            session['box_ids'] = box_ids

        # Initialize default values
        boxes_data = {f"box{i}": {
            "value": 'No Data',
            "time_diff": 'N/A'
        } for i in range(1, 5)}

        # Try to get data for each box
        for i, box_id in enumerate(box_ids, 1):
            try:
                box = db_session.query(SensorData).filter_by(BoxID=box_id).order_by(SensorData.id.desc()).first()
                if box:
                    now = datetime.now()
                    time_diff = int((now - box.GW_datetime).total_seconds() / 60)
                    boxes_data[f"box{i}"].update({
                        "value": box.GW_datetime,
                        "time_diff": time_diff
                    })
            except Exception as e:
                app.logger.error(f"Error getting data for box {box_id}: {str(e)}")
                continue

        # Return the data for JavaScript to use
        if request.headers.get('X-Requested-With') == 'XMLHttpRequest':
            return jsonify({
                "scroll_labels": scroll_headers,
                "box_ids": box_ids
            })

        return render_template('index.html', data=boxes_data)

    except Exception as e:
        app.logger.error(f"Error in index route: {str(e)}")
        return render_template('index.html', error="System error occurred", data={})

@app.route('/get_data')
def get_data():
    field = request.args.get('field')
    box_ids = session.get('box_ids', [])

    # Initialize data structure for all possible boxes
    data = {}
    for i in range(1, 5):
        data[f"box{i}"] = {
            "value": 'No Box',
            "status": "Offline",
            "time_diff": 'N/A'
        }

    # Process available boxes
    for i, box_id in enumerate(box_ids, 1):
        try:
            box = db_session.query(SensorData).filter_by(BoxID=box_id).order_by(SensorData.id.desc()).first()
            if box:
                now = datetime.now()
                time_diff = int((now - box.GW_datetime).total_seconds() / 60) if box else 'Error'
                
                data[f"box{i}"] = {


                    "value": getattr(box, field, 'Error') if box is not None else 'Error',
                    "status": "Offline" if isinstance(time_diff, int) and time_diff > 5 else "Online",
                    "time_diff": time_diff
                }
        except Exception as e:
            app.logger.error(f"Error processing box {box_id}: {str(e)}")
            continue



    return jsonify(data)

if __name__ == '__main__':
    app.run('0.0.0.0')
