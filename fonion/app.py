from flask import Flask, render_template, jsonify
from sqlalchemy.ext.automap import automap_base
from sqlalchemy.orm import Session
from sqlalchemy import create_engine


DATABASE_URL = "sqlite:////home/pi/onion_logger/sensor_data.db"

Base = automap_base()

engine = create_engine(DATABASE_URL)

Base.prepare(engine, reflect=True)


SensorData = Base.classes.SensorData


db_session = Session(engine)

app = Flask(import_name=__name__)


@app.route('/')
def index():
    box1_values = 'Error'
    box2_values = 'Error'
    box3_values = 'Error'
    box4_values = 'Error'

    box1 = db_session.query(SensorData).filter_by(BoxID=1).order_by(SensorData.id.desc()).first()
    box2 = db_session.query(SensorData).filter_by(BoxID=2).order_by(SensorData.id.desc()).first()
    box3 = db_session.query(SensorData).filter_by(BoxID=3).order_by(SensorData.id.desc()).first()
    box4 = db_session.query(SensorData).filter_by(BoxID=4).order_by(SensorData.id.desc()).first()

    if box1 != None:
        box1_values = box1.CO2

    if box2 != None:
        box2_values = box2.CO2

    if box3 != None:
        box3_values = box3.CO2

    if box4 != None:
        box4_values = box4.CO2

    return render_template('index.html', box1=box1_values, box2=box2_values, box3=box3_values, box4=box4_values)


@app.route('/get_data')
def get_data():
    box1 = db_session.query(SensorData).filter_by(BoxID=1).order_by(SensorData.id.desc()).first()
    box2 = db_session.query(SensorData).filter_by(BoxID=2).order_by(SensorData.id.desc()).first()
    box3 = db_session.query(SensorData).filter_by(BoxID=3).order_by(SensorData.id.desc()).first()
    box4 = db_session.query(SensorData).filter_by(BoxID=4).order_by(SensorData.id.desc()).first()

    data = {
        "box1": box1.CO2 if box1 is not None else 'Error',
        "box2": box2.CO2 if box2 is not None else 'Error',
        "box3": box3.CO2 if box3 is not None else 'Error',
        "box4": box4.CO2 if box4 is not None else 'Error'
    }

    return jsonify(data)


if __name__ == '__main__':
    app.run('0.0.0.0')