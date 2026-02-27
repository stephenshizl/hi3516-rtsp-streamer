#!/bin/sh
# Start IPCamera streamer

PROG="/usr/bin/ipcamera"
CONF="/etc/ipcamera.conf"
PIDFILE="/var/run/ipcamera.pid"
LOGFILE="/var/log/ipcamera.log"

if [ ! -x "$PROG" ]; then
    echo "Error: $PROG not found"
    exit 1
fi

if [ -f "$PIDFILE" ]; then
    PID=$(cat "$PIDFILE")
    if kill -0 "$PID" 2>/dev/null; then
        echo "ipcamera already running (pid=$PID)"
        exit 0
    fi
    rm -f "$PIDFILE"
fi

echo "Starting ipcamera..."
$PROG -c "$CONF" >> "$LOGFILE" 2>&1 &
echo $! > "$PIDFILE"
echo "ipcamera started (pid=$(cat $PIDFILE))"
