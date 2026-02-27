# Configuration Reference

The default configuration file is `/etc/ipcamera.conf`.

## [video] Section

| Key            | Type    | Default | Description                              |
|----------------|---------|---------|------------------------------------------|
| `main_width`   | integer | 1920    | Main stream width (pixels)               |
| `main_height`  | integer | 1080    | Main stream height (pixels)              |
| `main_bitrate` | integer | 4096    | Main stream bitrate (Kbps)               |
| `sub_width`    | integer | 1280    | Sub stream width (pixels)                |
| `sub_height`   | integer | 720     | Sub stream height (pixels)               |
| `sub_bitrate`  | integer | 1024    | Sub stream bitrate (Kbps)                |
| `frame_rate`   | integer | 25      | Frame rate (fps)                         |
| `gop`          | integer | 50      | GOP size (keyframe interval)             |
| `sensor_fps`   | integer | 30      | Sensor input frame rate                  |
| `codec`        | string  | h264    | Codec: `h264` or `h265`                  |
| `rc_mode`      | string  | cbr     | Rate control: `cbr` or `vbr`             |

## [network] Section

| Key          | Type    | Default       | Description                    |
|--------------|---------|---------------|--------------------------------|
| `ip`         | string  | 192.168.1.100 | Device IP address               |
| `netmask`    | string  | 255.255.255.0 | Network mask                    |
| `gateway`    | string  | 192.168.1.1   | Default gateway                 |
| `rtsp_port`  | integer | 554           | RTSP server port                |
| `rtsp_auth`  | integer | 0             | Enable RTSP authentication      |
| `rtsp_user`  | string  |               | RTSP username (if auth enabled) |
| `rtsp_pass`  | string  |               | RTSP password (if auth enabled) |

## [motion] Section

| Key              | Type    | Default | Description                    |
|------------------|---------|---------|--------------------------------|
| `enable`         | integer | 1       | Enable motion detection        |
| `region_count`   | integer | 1       | Number of detection regions (1-4) |
| `region_N_x`     | integer | 0       | Region N left edge (pixels)    |
| `region_N_y`     | integer | 0       | Region N top edge (pixels)     |
| `region_N_w`     | integer | 1920    | Region N width                 |
| `region_N_h`     | integer | 1080    | Region N height                |
| `region_N_sens`  | integer | 5       | Sensitivity (1-10, higher=more sensitive) |
| `region_N_thresh`| integer | 40      | Alarm threshold (active blocks %) |

## [record] Section

| Key              | Type    | Default           | Description                           |
|------------------|---------|-------------------|---------------------------------------|
| `enable`         | integer | 1                 | Enable recording                      |
| `mode`           | integer | 3                 | 0=manual, 1=motion, 2=schedule, 3=continuous |
| `slice_time`     | integer | 10                | Minutes per file segment              |
| `max_days`       | integer | 7                 | Maximum days to retain recordings     |
| `pre_record`     | integer | 5                 | Pre-alarm buffer (seconds)            |
| `post_record`    | integer | 30                | Post-alarm recording (seconds)        |
| `min_free_mb`    | integer | 256               | Minimum free space before deleting old files (MB) |
| `mount_point`    | string  | /mnt/sdcard/record | Recording directory                  |
