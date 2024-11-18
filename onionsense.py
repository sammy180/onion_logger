import serial
from serial.serialutil import SerialException
import threading
import time
import os
from datetime import datetime
from sqlalchemy import create_engine, Column, Integer, Float, String, DateTime, ForeignKey
from sqlalchemy.orm import declarative_base
from sqlalchemy.orm import sessionmaker

# Initialize SQLAlchemy
DATABASE_URL = "sqlite:///sensor_data.db"
engine = create_engine(DATABASE_URL)
Session = sessionmaker(bind=engine)
session = Session()
Base = declarative_base()

# Read headers from file
headers = []
try:
    with open("headers.txt", "r") as file:
        headers = [line.strip() for line in file.readlines()]
    print(f"Headers loaded: {headers}")
except FileNotFoundError:
    print("Failed to open headers.txt")
    exit(1)

# Define the SensorData model based on headers
class SensorData(Base):
    __tablename__ = 'SensorData'

    id = Column(Integer, primary_key=True, unique=True, nullable=False)
    BoxID = Column(String, nullable=True)
    FuseID = Column(String, nullable=True)
    Time = Column(String, nullable=True)
    minutePt = Column(String, nullable=True)
    GW_datetime = Column(DateTime, default=datetime.now)
    Ext_Temp = Column(Float, nullable=True)
    Ext_Hum = Column(Float, nullable=True)
    Ext_Pressure = Column(Float, nullable=True)
    EC_Temp = Column(Float, nullable=True)
    EC_Hum = Column(Float, nullable=True)
    MOX_Temp = Column(Float, nullable=True)
    MOX_Hum = Column(Float, nullable=True)
    MOX_Pres = Column(Float, nullable=True)
    MOX_Heat = Column(Float, nullable=True)
    Ext_Gas = Column(Float, nullable=True)
    EC0_NO2 = Column(Float, nullable=True)
    EC1_H2S = Column(Float, nullable=True)
    EC2_SO2 = Column(Float, nullable=True)
    EC3_VOC = Column(Float, nullable=True)
    EC0_Ref = Column(Float, nullable=True)
    EC1_Ref = Column(Float, nullable=True)
    EC2_Ref = Column(Float, nullable=True)
    EC3_Ref = Column(Float, nullable=True)
    CO2 = Column(Float, nullable=True)
    ENS160 = Column(Float, nullable=True)
    SGPVRaw = Column(Float, nullable=True)
    SGPNRaw = Column(Float, nullable=True)
    #SGPVin = Column(Float, nullable=True)
    #SGPNin = Column(Float, nullable=True)
    TGS2603 = Column(Float, nullable=True)
    TGS2620 = Column(Float, nullable=True)
    TGS2602 = Column(Float, nullable=True)
    HP0 = Column(Float, nullable=True)
    HP1 = Column(Float, nullable=True)
    HP2 = Column(Float, nullable=True)
    HP3 = Column(Float, nullable=True)
    Error = Column(Float, nullable=True)


# Create the tables in the database
Base.metadata.create_all(engine)

class SerialMonitor:
    def __init__(self, port_name):
        self.port_name = port_name
        self.running = False
        self.thread = None

    def start(self):
        print(f"Starting monitoring on port: {self.port_name}")
        self.running = True
        self.thread = threading.Thread(target=self.monitor_serial)
        self.thread.start()

    def stop(self):
        print(f"Stopping monitoring on port: {self.port_name}")
        self.running = False
        if self.thread is not None:
            self.thread.join()

    def monitor_serial(self):
        # Adding initial delay to allow the device to be ready
        print(f"[{self.port_name}] Initial delay before starting to read from device.")
        time.sleep(1)

        try:
            with serial.Serial(self.port_name, baudrate=115200, timeout=1) as ser:
                print(f"[{self.port_name}] Serial port opened successfully.")
                while self.running:
                    if ser.in_waiting > 0:
                        data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')

                        # Sanitize data by removing newlines, carriage returns, and trailing semicolons
                        data = data.replace('\n', '').replace('\r', '').replace(' ', '')
                        if data.endswith(';'):
                            data = data[:-1]

                        print(f"[{self.port_name}] Data read: {data}")

                        # Check if the data contains a complete message
                        if data.startswith("Box") and data.endswith("X"):
                            print(f"[{self.port_name}] Complete message received: {data}")
                            values = self.split_data(data)
                            print(f"[{self.port_name}] Values split: {values}")  # Debugging statement to see split values
                            self.insert_data(values)
                    else:
                        time.sleep(0.1)  # Sleep for 100 milliseconds to prevent tight looping
        except serial.SerialException as e:
            print(f"[{self.port_name}] Error opening or reading from serial port: {e}")

    def split_data(self, data):
        # Remove the 'Box:' prefix and 'X' suffix
        if data.startswith('Box:') and data.endswith('X'):
            data = data[4:-1]
        # Remove any trailing empty fields resulting from a trailing semicolon
        data = data.rstrip(';')
        print(f"[{self.port_name}] Splitting data: {data}")
        return data.split(';')

    def insert_data(self, values):
        # Insert data into the database using SQLAlchemy
        try:
            print(f"[{self.port_name}] Inserting data into database.")
            sensor_data = SensorData(
                
                BoxID=values[0] if len(values) > 0 else None,
                FuseID=self.port_name,
                Time=values[1] if len(values) > 1 else None,
                minutePt=values[2] if len(values) > 2 else None,
                Ext_Temp=float(values[3]) if len(values) > 3 else None,
                Ext_Hum=float(values[4]) if len(values) > 4 else None,
                Ext_Pressure=float(values[5]) if len(values) > 5 else None,
                EC_Temp=float(values[6]) if len(values) > 6 else None,
                EC_Hum=float(values[7]) if len(values) > 7 else None,
                MOX_Temp=float(values[8]) if len(values) > 8 else None,
                MOX_Hum=float(values[9]) if len(values) > 9 else None,
                MOX_Pres=float(values[10]) if len(values) > 10 else None,
                MOX_Heat=float(values[11]) if len(values) > 11 else None,
                Ext_Gas=float(values[12]) if len(values) > 12 else None,
                EC0_NO2=float(values[13]) if len(values) > 13 else None,
                EC1_H2S=float(values[14]) if len(values) > 14 else None,
                EC2_SO2=float(values[15]) if len(values) > 15 else None,
                EC3_VOC=float(values[16]) if len(values) > 16 else None,
                EC0_Ref=float(values[17]) if len(values) > 17 else None,
                EC1_Ref=float(values[18]) if len(values) > 18 else None,
                EC2_Ref=float(values[19]) if len(values) > 19 else None,
                EC3_Ref=float(values[20]) if len(values) > 20 else None,
                CO2=float(values[21]) if len(values) > 21 else None,
                ENS160=float(values[22]) if len(values) > 22 else None,
                SGPVRaw=float(values[23]) if len(values) > 23 else None,
                SGPNRaw=float(values[24]) if len(values) > 24 else None,
                #SGPVin=float(values[25]) if len(values) > 25 else None,
                #SGPNin=float(values[26]) if len(values) > 26 else None,
                TGS2603=float(values[27]) if len(values) > 27 else None,
                TGS2620=float(values[28]) if len(values) > 28 else None,
                TGS2602=float(values[29]) if len(values) > 29 else None,
                HP0=float(values[30]) if len(values) > 30 else None,
                HP1=float(values[31]) if len(values) > 31 else None,
                HP2=float(values[32]) if len(values) > 32 else None,
                HP3=float(values[33]) if len(values) > 33 else None,
                Error=float(values[34]) if len(values) > 34 else None,
                
            )
            session.add(sensor_data)
            session.commit()
            print(f"[{self.port_name}] Data inserted successfully.")
        except Exception as e:
            print(f"[{self.port_name}] Failed to insert data: {e}")
            session.rollback()


def monitor_for_devices():
    monitors = []
    try:
        while True:
            # List /dev directory to find Onion devices
            devices = [f"/dev/{d}" for d in os.listdir('/dev') if d.startswith("Onion")]
            for device in devices:
                if not any(m.port_name == device for m in monitors):
                    print(f"Found new device: {device}")
                    monitor = SerialMonitor(device)
                    monitors.append(monitor)
                    monitor.start()
            time.sleep(5)  # Check for new devices every 5 seconds
    except KeyboardInterrupt:
        print("Stopping device monitoring...")
        for monitor in monitors:
            monitor.stop()


def main():
    monitor_for_devices()


if __name__ == "__main__":
    main()