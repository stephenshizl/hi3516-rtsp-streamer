# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile

# Keep DeviceAdminReceiver
-keep public class * extends android.app.admin.DeviceAdminReceiver
-keepclassmembers class * extends android.app.admin.DeviceAdminReceiver {
    public <init>();
}

# Keep MainActivity
-keep public class com.hi3516.deviceadmin.MainActivity
-keepclassmembers class com.hi3516.deviceadmin.MainActivity {
    public void *(android.view.View);
}
