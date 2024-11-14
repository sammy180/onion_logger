from flask import Flask, render_template, jsonify, request, session
from sqlalchemy.ext.automap import automap_base
from sqlalchemy.orm import Session
from sqlalchemy import create_engine, desc

DATABASE_URL = "sqlite:////home/sammy/Active workspace/onion_logger/sensor_data2.db"
# DATABASE_URL = "sqlite:////home/pi/onion_logger/sensor_data2.db"

Base = automap_base()

engine = create_engine(DATABASE_URL, connect_args={'check_same_thread': False})

Base.prepare(engine, reflect=True)

SensorData = Base.classes.SensorData

db_session = Session(engine)

app = Flask(import_name=__name__)
app.secret_key = 'your_secret_key'  # Set a secret key for session management

@app.route('/setup')
def setup():
    # Query the database for the highest IDs and find the first 4 unique BoxIDs
    unique_box_ids = []
    results = db_session.query(SensorData.BoxID).order_by(desc(SensorData.id)).all()

    for result in results:
        if result.BoxID not in unique_box_ids:
            unique_box_ids.append(result.BoxID)
        if len(unique_box_ids) == 4:
            break

    box_ids = sorted(unique_box_ids[:4])
    session['box_ids'] = box_ids  # Store box_ids in the session

    # Read the headers.txt file to get scroll_labels
    headers_file_path = 'headers.txt'
    with open(headers_file_path, 'r') as file:
        scroll_labels = [line.strip() for line in file.readlines()]

    return jsonify({"box_ids": box_ids, "scroll_labels": scroll_labels})

@app.route('/')
def index():
    quad1_val = 'Error'
    quad2_val = 'Error'
    quad3_val = 'Error'
    quad4_val = 'Error'

    box_ids = session.get('box_ids', [])

    if len(box_ids) < 4:
        # Retrieve the unique BoxIDs from the setup route
        response = setup()
        if response.status_code == 400:
            return render_template('index.html', error="Less than 4 unique BoxIDs found")

        box_ids = response.json['box_ids']
    
    box1 = db_session.query(SensorData).filter_by(BoxID=box_ids[0]).order_by(SensorData.id.desc()).first()
    box2 = db_session.query(SensorData).filter_by(BoxID=box_ids[1]).order_by(SensorData.id.desc()).first()
    box3 = db_session.query(SensorData).filter_by(BoxID=box_ids[2]).order_by(SensorData.id.desc()).first()
    box4 = db_session.query(SensorData).filter_by(BoxID=box_ids[3]).order_by(SensorData.id.desc()).first()
    
    if box1 is not None:
        quad1_val = box1.CO2
    if box2 is not None:
        quad2_val = box2.CO2
    if box3 is not None:
        quad3_val = box3.CO2
    if box4 is not None:
        quad4_val = box4.CO2

    return render_template('index.html', box1=quad1_val, box2=quad2_val, box3=quad3_val, box4=quad4_val)

@app.route('/get_data')
def get_data():
    field = request.args.get('field')
    box_ids = session.get('box_ids', [])

    if len(box_ids) < 4:
        return jsonify({"error": "Less than 4 unique BoxIDs found"}), 400

    box1 = db_session.query(SensorData).filter_by(BoxID=box_ids[0]).order_by(SensorData.id.desc()).first()
    box2 = db_session.query(SensorData).filter_by(BoxID=box_ids[1]).order_by(SensorData.id.desc()).first()
    box3 = db_session.query(SensorData).filter_by(BoxID=box_ids[2]).order_by(SensorData.id.desc()).first()
    box4 = db_session.query(SensorData).filter_by(BoxID=box_ids[3]).order_by(SensorData.id.desc()).first()

    data = {
        "box1": getattr(box1, field, 'Error') if box1 is not None else 'Error',
        "box2": getattr(box2, field, 'Error') if box2 is not None else 'Error',
        "box3": getattr(box3, field, 'Error') if box3 is not None else 'Error',
        "box4": getattr(box4, field, 'Error') if box4 is not None else 'Error'
    }

    return jsonify(data)

if __name__ == '__main__':
    app.run('0.0.0.0')