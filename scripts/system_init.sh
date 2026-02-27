#!/bin/sh
# Hi3516CV300 System Initialization Script

set -e

DRIVER_PATH="/ko"
SYSFS_MMZ_PATH="/proc/umap/mmz"

load_driver()
{
    local ko="$1"
    local args="${2:-}"

    if lsmod | grep -q "$(basename $ko .ko)"; then
        echo "[SKIP] $ko already loaded"
        return 0
    fi

    if [ -f "${DRIVER_PATH}/${ko}.ko" ]; then
        insmod "${DRIVER_PATH}/${ko}.ko" $args && echo "[OK]   $ko" || echo "[FAIL] $ko"
    else
        echo "[WARN] $ko not found"
    fi
}

echo "=== Hi3516CV300 Driver Init ==="

load_driver sys
load_driver mmz    mmz=anonymous,0,0xC0000000,256M

load_driver hi_osal
load_driver hi3516cv300_base
load_driver hi3516cv300_sys
load_driver hi3516cv300_tde

load_driver hi3516cv300_vi
load_driver hi3516cv300_vpss
load_driver hi3516cv300_venc
load_driver hi3516cv300_vda
load_driver hi3516cv300_isp
load_driver hi3516cv300_mipi_rx

# Sensor driver
load_driver sony_imx335  sensor_type=0

echo "=== Driver Init Done ==="

# Setup device nodes
if [ ! -c /dev/hi_mipi ]; then
    mknod /dev/hi_mipi c 0 0 2>/dev/null || true
fi

# Set CPU affinity and governor
if [ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]; then
    echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
fi

# Mount SD card if available
if [ -b /dev/mmcblk0p1 ]; then
    mkdir -p /mnt/sdcard
    mount -t vfat /dev/mmcblk0p1 /mnt/sdcard -o noatime 2>/dev/null && \
        echo "[OK]   SD card mounted" || echo "[WARN] SD card mount failed"
elif [ -b /dev/mmcblk0 ]; then
    mkdir -p /mnt/sdcard
    mount -t vfat /dev/mmcblk0 /mnt/sdcard -o noatime 2>/dev/null && \
        echo "[OK]   SD card mounted" || echo "[WARN] SD card mount failed"
fi
