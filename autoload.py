#!/usr/bin/env python3
import os
import pyudev
import subprocess
import shutil
import glob

def eject_usb(usb_path):
    try:
        # Get device path from mount point
        result = subprocess.run(['findmnt', '-n', '-o', 'SOURCE', usb_path],
                              capture_output=True, text=True)
        device_path = result.stdout.strip()
        
        # Unmount
        subprocess.run(['udisksctl', 'unmount', '-b', device_path], check=True)
        # Power off
        subprocess.run(['udisksctl', 'power-off', '-b', device_path], check=True)
        
        subprocess.run(['zenity', '--info',
                       '--width=400',
                       '--height=200',
                       '--title=Success',
                       '--text=USB device ejected safely',
                       '--default-size=300,100'])
                       
    except subprocess.CalledProcessError as e:
        subprocess.run(['zenity', '--error',
                       '--width=400',
                       '--height=200',
                       '--title=Error',
                       '--text=Failed to eject USB device',
                       '--default-size=300,100'])

def download_data():
    try:
        # Find USB device
        usb_paths = glob.glob('/media/pi/*')
        if not usb_paths:
            subprocess.run(['zenity', '--error',
                            '--width=300',
                            '--title=Error',
                            '--text=No USB device found'])
            return

        usb_path = usb_paths[-1]
        source_file = 'sensor_data.db'
        
        if not os.path.exists(source_file):
            subprocess.run(['zenity', '--error',
                            '--width=300',
                            '--title=Error',
                            '--text=Database file not found'])
            return

        dest_file = os.path.join(usb_path, source_file)
        
        # Check if file exists on USB
        if os.path.exists(dest_file):
            # Ask user if they want to overwrite
            result = subprocess.run(['zenity', '--question',
                                     '--width=300',
                                     '--title=File Exists',
                                     '--text=File already exists. Overwrite?'],
                                     capture_output=True, text=True)
            
            if result.returncode != 0:  # User chose not to overwrite
                # Find next available filename
                counter = 1
                while os.path.exists(dest_file):
                    dest_file = os.path.join(usb_path, f"{os.path.splitext(source_file)[0]}_{counter}{os.path.splitext(source_file)[1]}")
                    counter += 1

        # Ensure the script has write permissions to the destination directory
        os.chmod(usb_path, 0o777)
        shutil.copy(source_file, dest_file)
        
        subprocess.run(['zenity', '--info',
                        '--width=300',
                        '--title=Success',
                        f'--text=Data downloaded successfully as {os.path.basename(dest_file)}'])
    except Exception as e:
        subprocess.run(['zenity', '--error',
                        '--width=300',
                        '--title=Error',
                        f'--text=Failed to download data: {str(e)}'])

def update_system():
    subprocess.run(['zenity', '--info',
                   '--width=300',
                   '--title=Update',
                   '--text=Updating system... (placeholder)'])

def show_popup(device_name):
    print("device inserted")
    result = subprocess.run(['zenity', '--question',
                           '--width=300',
                           '--title=USB Device Detected',
                           '--text=Device detected: ' + device_name,
                           '--ok-label=Download Data',
                           '--cancel-label=Cancel',
                           '--extra-button=Update'],
                           capture_output=True)
    print(result.returncode)
    
    if result.returncode == 0:  # Download Data
        download_data()
        print("download data")
    elif result.returncode == 1:  # Cancel
        pass
    elif result.returncode == 2:  # Update
        update_system()

def monitor_usb_devices():
    # Install required packages
    #os.system('sudo apt-get install -y zenity python3-pyudev')

    # Setup udev monitoring
    context = pyudev.Context()
    monitor = pyudev.Monitor.from_netlink(context)
    monitor.filter_by(subsystem='block')

    print("Monitoring for USB devices... (Press Ctrl+C to stop)")

    # Monitor continuously
    for device in iter(monitor.poll, None):
        if device.action == 'add' and device.get('ID_FS_USAGE') == 'filesystem':
            device_name = device.get('ID_FS_LABEL', 'USB Device')
            mount_point = f"/media/pi/{device_name}"
            show_popup(device_name)

if __name__ == "__main__":
    try:
        monitor_usb_devices()
    except KeyboardInterrupt:
        print("\nMonitoring stopped")