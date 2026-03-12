package com.hi3516.deviceadmin.receiver;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import androidx.core.app.NotificationCompat;

public class DeviceAdminHelper {
    
    private static final String CHANNEL_ID = "device_admin_channel";
    private static final String CHANNEL_NAME = "设备管理员通知";
    private static final int NOTIFICATION_ID = 1001;
    private static final int MAX_FAILED_ATTEMPTS = 5;

    public static void notifyPasswordFailed(Context context, int failedAttempts) {
        if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
            sendNotification(context, "安全警告", 
                    "密码错误次数过多，请立即检查设备安全");
        }
    }

    public static void notifyPasswordSucceeded(Context context) {
        sendNotification(context, "安全提示", "设备已成功解锁");
    }

    private static void sendNotification(Context context, String title, String message) {
        NotificationManager notificationManager = 
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    CHANNEL_NAME,
                    NotificationManager.IMPORTANCE_HIGH
            );
            channel.setDescription("设备管理员安全通知");
            notificationManager.createNotificationChannel(channel);
        }

        NotificationCompat.Builder builder = new NotificationCompat.Builder(context, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.ic_lock_lock)
                .setContentTitle(title)
                .setContentText(message)
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .setAutoCancel(true);

        notificationManager.notify(NOTIFICATION_ID, builder.build());
    }
}
