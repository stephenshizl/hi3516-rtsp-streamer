#!/bin/sh
# Stop IPCamera streamer

PIDFILE="/var/run/ipcamera.pid"
TIMEOUT=10

if [ ! -f "$PIDFILE" ]; then
    echo "ipcamera not running (no pidfile)"
    exit 0
fi

PID=$(cat "$PIDFILE")
if ! kill -0 "$PID" 2>/dev/null; then
    echo "ipcamera not running (stale pidfile)"
    rm -f "$PIDFILE"
    exit 0
fi

echo "Stopping ipcamera (pid=$PID)..."
kill -TERM "$PID"

i=0
while kill -0 "$PID" 2>/dev/null && [ $i -lt $TIMEOUT ]; do
    sleep 1
    i=$((i+1))
done

if kill -0 "$PID" 2>/dev/null; then
    echo "Force killing ipcamera..."
    kill -KILL "$PID"
fi

rm -f "$PIDFILE"
echo "ipcamera stopped"
