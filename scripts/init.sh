#!/bin/sh
# IPCamera init script (SysV compatible)

### BEGIN INIT INFO
# Provides:          ipcamera
# Required-Start:    $network
# Required-Stop:     $network
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Hi3516CV300 IPCamera RTSP Streamer
### END INIT INFO

SCRIPT_DIR="$(dirname "$0")"
SYSTEM_INIT="$SCRIPT_DIR/system_init.sh"

case "$1" in
    start)
        if [ -f "$SYSTEM_INIT" ]; then
            sh "$SYSTEM_INIT"
        fi
        sh "$SCRIPT_DIR/start.sh"
        ;;
    stop)
        sh "$SCRIPT_DIR/stop.sh"
        ;;
    restart)
        sh "$SCRIPT_DIR/stop.sh"
        sleep 2
        if [ -f "$SYSTEM_INIT" ]; then
            sh "$SYSTEM_INIT"
        fi
        sh "$SCRIPT_DIR/start.sh"
        ;;
    status)
        PIDFILE="/var/run/ipcamera.pid"
        if [ -f "$PIDFILE" ] && kill -0 "$(cat $PIDFILE)" 2>/dev/null; then
            echo "ipcamera is running (pid=$(cat $PIDFILE))"
        else
            echo "ipcamera is not running"
        fi
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac
