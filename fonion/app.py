from flask import Flask, render_template, jsonify, request, session
from sqlalchemy.ext.automap import automap_base
from sqlalchemy.orm import Session
from sqlalchemy import create_engine, desc
from datetime import datetime

DATABASE_URL = "sqlite:////home/sammy/Active workspace/onion_logger/sensor_data2.db"
# DATABASE_URL = "sqlite:////home/pi/onion_logger/sensor_data2.db"

Base = automap_base()

engine = create_engine(DATABASE_URL, connect_args={'check_same_thread': False})

Base.prepare(engine, reflect=True)

SensorData = Base.classes.SensorData

db_session = Session(engine)

app = Flask(import_name=__name__)
app.secret_key = 'your_secret_key'  # Set a secret key for session management



@app.route('/')
def index():
    try:
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

        return render_template('index.html', data=boxes_data)

    except Exception as e:
        app.logger.error(f"Error in index route: {str(e)}")
        return render_template('index.html', error="System error occurred", data={})
    


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


    if box1:
        quad1_val = box1.GW_datetime
    if box2:
        quad2_val = box2.GW_datetime
    if box3:
        quad3_val = box3.GW_datetime
    if box4:
        quad4_val = box4.GW_datetime

    # Calculate the time difference from the last data point to now

    now = datetime.now()
    time_diff1 = int((now - box1.GW_datetime).total_seconds() / 60) if box1 else 'Error'
    time_diff2 = int((now - box2.GW_datetime).total_seconds() / 60) if box2 else 'Error'
    time_diff3 = int((now - box3.GW_datetime).total_seconds() / 60) if box3 else 'Error'
    time_diff4 = int((now - box4.GW_datetime).total_seconds() / 60) if box4 else 'Error'

    data = {
            "box1": {
                "value": getattr(box1, field, 'Error') if box1 is not None else 'Error',
                "status": "Offline" if time_diff1 > 5 else "Online",
                "time_diff": time_diff1
            },
            "box2": {
                "value": getattr(box2, field, 'Error') if box2 is not None else 'Error',
                "status": "Offline" if time_diff2 > 5 else "Online",
                "time_diff": time_diff2
            },
            "box3": {
                "value": getattr(box3, field, 'Error') if box3 is not None else 'Error',
                "status": "Offline" if time_diff3 > 5 else "Online",
                "time_diff": time_diff3
            },
            "box4": {
                "value": getattr(box4, field, 'Error') if box4 is not None else 'Error',
                "status": "Offline" if time_diff4 > 5 else "Online",
                "time_diff": time_diff4
            }
        }

    return jsonify(data)

if __name__ == '__main__':
    app.run('0.0.0.0')
