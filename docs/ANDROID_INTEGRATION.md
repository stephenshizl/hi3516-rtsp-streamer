# Android 设备管理员集成指南

本文档说明如何使用 Android 设备管理员应用与 Hi3516 RTSP 流媒体项目集成。

## 概述

Hi3516-RTSP-Streamer 项目包含两个独立的部分：

1. **C/C++ RTSP 流媒体服务器** - 运行在 Hi3516CV300 嵌入式平台
2. **Android 设备管理员应用** - 运行在 Android 13+ 设备上

这两个组件可以独立使用，也可以配合使用来构建完整的 IP 摄像头解决方案。

## 架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Hi3516 IP 摄像头设备                      │
│                                                             │
│  ┌──────────────────────┐         ┌──────────────────────┐ │
│  │  RTSP Streamer       │         │  Android 系统        │ │
│  │  (C/C++)             │         │                      │ │
│  │                      │         │  ┌────────────────┐  │ │
│  │  • 视频编码          │         │  │ 设备管理员应用  │  │ │
│  │  • RTSP 服务器       │         │  │ (Java)         │  │ │
│  │  • SD 卡录制         │         │  │                │  │ │
│  │  • 运动检测          │         │  │ • 安全管理      │  │ │
│  │                      │         │  │ • 设备控制      │  │ │
│  └──────────┬───────────┘         │  │ • 远程擦除      │  │ │
│             │                     │  └────────────────┘  │ │
│             │                     └──────────────────────┘ │
│             │                                            │
└─────────────┼────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────┐
│                    网络客户端                                │
│                                                             │
│  • VLC Player                                              │
│  • FFplay                                                  │
│  • 自定义播放器应用                                         │
│  • Web 浏览器                                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景 1：独立使用

**Android 设备管理员应用** 可用于：
- 企业设备管理
- 教育设备管理
- 个人设备安全增强
- 家长控制应用

**RTSP 流媒体服务器** 可用于：
- 独立 IP 摄像头
- 视频监控系统
- 直播推流应用

### 场景 2：配合使用

在 Hi3516 平台上，可以：

1. 在 Android 设备上安装设备管理员应用
2. 在同一设备或其他设备上运行 RTSP 流媒体服务器
3. 通过设备管理员应用：
   - 远程管理摄像头设备
   - 控制 SD 卡录制功能
   - 管理网络访问
   - 监控设备状态

## 部署方案

### 方案 A：单设备部署

在同一台 Hi3516 Android 设备上：

```
Hi3516 Android 设备
├── Android 系统 (API 33+)
│   └── 设备管理员应用 (APK)
└── RTSP Streamer (原生二进制)
```

**优点**:
- 设备整合度高
- 管理方便
- 本地控制能力强

**挑战**:
- 需要同时支持 Android 和原生代码
- 资源分配需要优化

### 方案 B：双设备部署

设备 1: Hi3516 RTSP 流媒体服务器
设备 2: Android 设备（运行设备管理员应用）

**优点**:
- 每个设备专注其功能
- 更容易部署和维护
- 可以远程管理

**挑战**:
- 需要网络连接
- 延迟可能较高

## 开发集成

### Android 应用调用 RTSP 服务

如果需要在 Android 应用中集成 RTSP 播放：

```java
// 使用 MediaPlayer 播放 RTSP 流
MediaPlayer mediaPlayer = new MediaPlayer();
try {
    mediaPlayer.setDataSource("rtsp://192.168.1.100:554/stream0");
    mediaPlayer.setSurface(surfaceView.getHolder().getSurface());
    mediaPlayer.prepareAsync();
    mediaPlayer.start();
} catch (IOException e) {
    e.printStackTrace();
}
```

### 使用 ExoPlayer（推荐）

```gradle
implementation 'com.google.android.exoplayer:exoplayer:2.18.7'
```

```java
// 创建播放器
SimpleExoPlayer player = new SimpleExoPlayer.Builder(context).build();
playerView.setPlayer(player);

// 准备媒体项
MediaItem mediaItem = MediaItem.fromUri("rtsp://192.168.1.100:554/stream0");
player.setMediaItem(mediaItem);
player.prepare();
player.play();
```

## 安全考虑

### 设备管理员权限

设备管理员应用拥有高权限，需要：

1. **用户明确同意**: 用户必须手动激活设备管理员
2. **透明度**: 清楚说明应用的功能和权限用途
3. **可控性**: 提供停用选项
4. **审计日志**: 记录所有管理操作

### RTSP 安全

RTSP 流媒体服务器安全建议：

1. **使用 HTTPS/WSS**: 如果可能，加密传输
2. **访问控制**: 设置认证（用户名/密码）
3. **防火墙规则**: 限制访问 IP 范围
4. **定期更新**: 保持固件和应用更新

## 故障排除

### 常见问题

#### 1. Android 应用无法连接 RTSP 流

**可能原因**:
- 网络不通
- 防火墙阻止
- RTSP 服务未启动

**解决方案**:
```bash
# 检查 RTSP 服务状态
ps aux | grep rtsp

# 检查端口监听
netstat -tuln | grep 554

# 测试连接
curl -v rtsp://192.168.1.100:554/stream0
```

#### 2. 设备管理员无法激活

**可能原因**:
- 已有其他设备管理员限制
- 设备被企业管理
- Android 版本不兼容

**解决方案**:
- 检查设备管理员列表
- 联系设备管理员
- 确认 Android 版本（需要 API 33+）

#### 3. 性能问题

**优化建议**:
- 调整视频分辨率和码率
- 使用硬件加速解码
- 优化网络缓冲区大小

## 扩展功能

### 可以添加的功能

1. **双向通信**: 通过 WebSocket 或其他协议实现控制通道
2. **云存储集成**: 将录制的视频上传到云端
3. **AI 分析**: 集成人脸识别、物体检测等 AI 功能
4. **推送通知**: 运动检测时发送通知到 Android 设备
5. **多设备管理**: 通过 Android 应用管理多个摄像头设备

### 示例：运动检测通知

在 RTSP 服务器端添加：

```c
// 检测到运动时发送通知
void send_motion_notification() {
    // 通过 HTTP 或其他协议发送通知到 Android 设备
    http_post("https://android-device-api/motion", 
              "{\"timestamp\": %ld, \"confidence\": %f}",
              time(NULL), confidence);
}
```

Android 端接收：

```java
// 使用 WorkManager 或 Firebase FCM 接收通知
public class MotionWorker extends Worker {
    @Override
    public Result doWork() {
        // 处理运动检测通知
        showNotification("检测到运动", "摄像头 " + deviceId + " 检测到移动");
        return Result.success();
    }
}
```

## 资源链接

### 文档

- [Android 设备管理 API](https://developer.android.com/guide/topics/admin/device-admin)
- [Live555 RTSP 文档](http://www.live555.com/liveMedia/)
- [Hi3516 SDK 文档](https://www.hisilicon.com/)

### 工具

- [Android Studio](https://developer.android.com/studio)
- [VLC Media Player](https://www.videolan.org/vlc/)
- [FFmpeg](https://ffmpeg.org/)

## 许可证

本项目的两个组件遵循相同的开源许可证。请查看主 README 或 LICENSE 文件。

## 贡献

欢迎贡献代码、报告问题或提出改进建议。请通过 GitHub Issues 和 Pull Requests 参与贡献。
