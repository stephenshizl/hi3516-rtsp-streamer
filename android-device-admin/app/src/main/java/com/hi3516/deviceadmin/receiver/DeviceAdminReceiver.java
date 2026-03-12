package com.hi3516.deviceadmin.receiver;

import android.app.admin.DeviceAdminReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.UserManager;
import android.widget.Toast;

public class DeviceAdminReceiver extends DeviceAdminReceiver {

    @Override
    public void onEnabled(Context context, Intent intent) {
        super.onEnabled(context, intent);
        showToast(context, "设备管理员已激活");
    }

    @Override
    public void onDisabled(Context context, Intent intent) {
        super.onDisabled(context, intent);
        showToast(context, "设备管理员已停用");
    }

    @Override
    public void onDisableRequested(Context context, Intent intent) {
        super.onDisableRequested(context, intent);
        showToast(context, "请求停用设备管理员");
    }

    @Override
    public void onPasswordChanged(Context context, Intent intent) {
        super.onPasswordChanged(context, intent);
        showToast(context, "密码已更改");
    }

    @Override
    public void onPasswordFailed(Context context, Intent intent) {
        super.onPasswordFailed(context, intent);
        UserManager userManager = (UserManager) context.getSystemService(Context.USER_SERVICE);
        int failedAttempts = userManager.getUserHandle();
        
        DeviceAdminHelper.notifyPasswordFailed(context, failedAttempts);
        showToast(context, "密码错误，第 " + failedAttempts + " 次尝试");
    }

    @Override
    public void onPasswordSucceeded(Context context, Intent intent) {
        super.onPasswordSucceeded(context, intent);
        DeviceAdminHelper.notifyPasswordSucceeded(context);
        showToast(context, "密码验证成功");
    }

    @Override
    public void onLockTaskModeEntering(Context context, Intent intent, String pkg) {
        super.onLockTaskModeEntering(context, intent, pkg);
        showToast(context, "进入固定应用模式: " + pkg);
    }

    @Override
    public void onLockTaskModeExiting(Context context, Intent intent) {
        super.onLockTaskModeExiting(context, intent);
        showToast(context, "退出固定应用模式");
    }

    @Override
    public CharSequence onDisableRequested(Context context, Intent intent) {
        return "停用设备管理员将导致某些安全功能无法使用。确定要继续吗？";
    }

    private void showToast(Context context, String message) {
        Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
    }
}
