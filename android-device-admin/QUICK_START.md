# 快速开始指南

本指南帮助您快速上手 Hi3516 Android 设备管理员应用。

## 前置条件

### 必需软件

1. **Java Development Kit (JDK)**
   ```bash
   # 检查 Java 版本
   java -version
   # 需要 JDK 8 或更高版本
   ```

2. **Android SDK**
   ```bash
   # 检查 Android SDK
   $ANDROID_HOME/tools/bin/sdkmanager --version
   ```

3. **Android Studio** (推荐，但可选)
   - 下载: https://developer.android.com/studio
   - 或仅使用命令行工具

### 环境配置

```bash
# 设置环境变量（添加到 ~/.bashrc 或 ~/.zshrc）
export ANDROID_HOME=/path/to/android/sdk
export PATH=$PATH:$ANDROID_HOME/tools
export PATH=$PATH:$ANDROID_HOME/platform-tools
export PATH=$PATH:$ANDROID_HOME/tools/bin

# 使配置生效
source ~/.bashrc  # 或 source ~/.zshrc
```

## 5 分钟快速开始

### 步骤 1: 配置项目

```bash
cd android-device-admin

# 创建本地配置文件
cp local.properties.template local.properties

# 编辑 local.properties，设置 SDK 路径
# 示例内容：
# sdk.dir=/home/yourname/Android/Sdk
```

### 步骤 2: 构建应用

```bash
# 构建 Debug 版本
./gradlew assembleDebug

# 首次构建会下载 Gradle 和依赖，可能需要几分钟
```

### 步骤 3: 安装到设备

```bash
# 确保设备已连接并启用 USB 调试
adb devices

# 安装应用
adb install app/build/outputs/apk/debug/app-debug.apk
```

### 步骤 4: 激活设备管理员

```bash
# 启动应用
adb shell am start -n com.hi3516.deviceadmin/.MainActivity

# 或在设备上手动打开 "Hi3516 设备管理员" 应用

# 点击 "激活设备管理员" 按钮
# 在系统提示中确认激活
```

### 步骤 5: 测试功能

- 点击 "锁定设备" 按钮测试锁定功能
- 点击 "设置密码策略" 测试密码设置
- 点击 "禁用相机" 测试相机控制

## 常用 Gradle 命令

```bash
# 清理构建
./gradlew clean

# 构建 Debug 版本
./gradlew assembleDebug

# 构建 Release 版本
./gradlew assembleRelease

# 运行单元测试
./gradlew test

# 运行检查（lint）
./gradlew lint

# 生成 APK
./gradlew assembleDebug

# 查看依赖树
./gradlew app:dependencies

# 生成调试报告
./gradlew build --info
```

## 开发工具

### Android Studio

如果使用 Android Studio：

1. **打开项目**
   ```
   File -> Open -> 选择 android-device-admin 目录
   ```

2. **同步 Gradle**
   - 点击 "Sync Project with Gradle Files"
   - 或点击顶部的 "Sync Now" 按钮

3. **运行应用**
   - 点击绿色运行按钮 ▶
   - 或按 Shift+F10

### 命令行

```bash
# 实时查看日志
adb logcat | grep Hi3516

# 查看应用日志
adb logcat -s "DeviceAdmin:*"

# 查看崩溃日志
adb logcat -b crash
```

## 目录说明

```
android-device-admin/
├── app/src/main/
│   ├── java/           # Java 源代码
│   ├── res/            # 资源文件（布局、字符串、图标等）
│   └── AndroidManifest.xml  # 应用清单
├── app/build.gradle    # 应用构建配置
├── build.gradle        # 项目构建配置
└── gradlew            # Gradle 包装器脚本
```

## 常见问题

### 1. Gradle 构建失败

**问题**: 找不到 SDK 或 Gradle 版本不匹配

**解决方案**:
```bash
# 确认 SDK 路径正确
cat local.properties

# 清理并重新构建
./gradlew clean
./gradlew assembleDebug
```

### 2. 无法安装 APK

**问题**: INSTALL_FAILED_UPDATE_INCOMPATIBLE

**解决方案**:
```bash
# 先卸载旧版本
adb uninstall com.hi3516.deviceadmin

# 再安装新版本
adb install app/build/outputs/apk/debug/app-debug.apk
```

### 3. 设备管理员无法激活

**问题**: 点击激活按钮无反应

**解决方案**:
```bash
# 检查是否已被激活
adb shell dumpsys device_policy | grep hi3516

# 手动在设置中激活
# 设置 -> 安全 -> 设备管理应用
```

### 4. 应用崩溃

**问题**: 应用意外停止

**解决方案**:
```bash
# 查看详细日志
adb logcat -b crash

# 查看应用日志
adb logcat | grep -E "AndroidRuntime|FATAL"

# 常见原因：
# - 缺少权限（检查 AndroidManifest.xml）
# - 设备管理员未激活
# - Android 版本不兼容（需要 API 33+）
```

## 调试技巧

### 1. 启用详细日志

在 `MainActivity.java` 中添加：

```java
import android.util.Log;

// 添加日志
Log.d("DeviceAdmin", "Device admin status: " + isActive);
```

### 2. 使用 Android Studio Debugger

1. 在代码行号左侧点击设置断点
2. 点击虫子图标 🐛
3. 选择设备并开始调试

### 3. 检查设备管理员状态

```bash
# 查看所有设备管理员
adb shell dumpsys device_policy

# 查看特定应用状态
adb shell dumpsys device_policy | grep -A 10 hi3516
```

## 自定义配置

### 修改应用名称

编辑 `app/src/main/res/values/strings.xml`:

```xml
<string name="app_name">您的应用名称</string>
```

### 修改包名

需要更新以下文件：

1. `app/build.gradle`:
   ```gradle
   applicationId "your.new.package.name"
   ```

2. 移动所有 Java 文件到新包路径

3. 更新 `AndroidManifest.xml` 中的包名

### 添加新功能

在 `device_admin.xml` 中添加策略：

```xml
<uses-policies>
    <!-- 现有策略 -->
    <limit-password />
    
    <!-- 新策略 -->
    <reset-password />
    <expire-password />
</uses-policies>
```

在 `MainActivity.java` 中添加控制方法：

```java
private void newPasswordPolicy() {
    if (devicePolicyManager.isAdminActive(deviceAdminComponent)) {
        // 您的策略代码
        devicePolicyManager.setPasswordExpirationTimeout(
            deviceAdminComponent, 
            86400000L  // 24小时
        );
    }
}
```

## 下一步

- 📖 阅读完整文档: [README.md](README.md)
- 🔧 查看集成指南: [docs/ANDROID_INTEGRATION.md](../docs/ANDROID_INTEGRATION.md)
- 📊 了解项目细节: [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)

## 获取帮助

- 查看文档
- 检查日志
- 提交 GitHub Issue
- 阅读 Android 官方文档: https://developer.android.com/guide/topics/admin/device-admin

## 版本信息

- **版本**: 1.0.0
- **最小 SDK**: 33 (Android 13)
- **目标 SDK**: 33 (Android 13)
- **构建工具**: Gradle 8.0
- **语言**: Java 8

---

祝您使用愉快！🚀
