#!/bin/bash

# Define the folder containing your systemd unit files
UNIT_FILES_FOLDER="/home/pi/onion_logger/unit_files"  # Update this path if necessary
TARGET_FOLDER="/etc/systemd/system"

# Check if the folder with unit files exists
if [ ! -d "$UNIT_FILES_FOLDER" ]; then
    echo "The specified folder '$UNIT_FILES_FOLDER' does not exist."
    exit 1
fi

# Loop through each unit file in the folder
for unit_file in "$UNIT_FILES_FOLDER"/*.service; do
    # Extract the filename
    filename=$(basename "$unit_file")
    target_file="$TARGET_FOLDER/$filename"

    # Check if the file already exists in the target directory
    if [ -f "$target_file" ]; then
        # Prompt the user if they want to overwrite the existing file
        read -p "The file '$filename' already exists in $TARGET_FOLDER. Do you want to overwrite it? (y/n): " response
        if [[ "$response" =~ ^[Yy]$ ]]; then
            # Overwrite the file
            sudo cp "$unit_file" "$target_file"
            echo "Overwrote '$filename'."
        else
            echo "Skipped '$filename'."
        fi
    else
        # Copy the file as it doesn't exist in the target directory
        sudo cp "$unit_file" "$target_file"
        echo "Installed '$filename'."
    fi
done

# Reload systemd manager configuration
sudo systemctl daemon-reload
echo "Systemd manager configuration reloaded."
