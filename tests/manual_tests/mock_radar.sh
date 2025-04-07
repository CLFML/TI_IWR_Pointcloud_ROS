#!/bin/bash

CMD_WRITE="/dev/pts/5"  
DATA_WRITE="/dev/pts/8"   
BIN_FILE="serial_dump.bin"

echo "[Listening on $CMD_WRITE for 'sensorStart']"

# Open ports
exec 3<> "$CMD_WRITE"
exec 4> "$DATA_WRITE"

sensor_started=0

while true; do
    if read -r -t 1 line <&3; then
        echo "[RX] $line"

        if  [[ "$line" == *"sensorStart"* ]]; then
            echo "[!] Got sensorStart. Replaying binary to $DATA_WRITE"
            pv -L 92160 "$BIN_FILE" >&4 &
            sensor_started=1
        fi

        echo -ne "Done\n" >&3
    fi
done
