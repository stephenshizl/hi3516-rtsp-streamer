package com.hi3516.deviceadmin;

import android.app.Activity;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.hi3516.deviceadmin.receiver.DeviceAdminReceiver;

public class MainActivity extends AppCompatActivity {

    private DevicePolicyManager devicePolicyManager;
    private ComponentName deviceAdminComponent;
    private TextView adminStatusText;
    private Button activateButton;
    private Button lockDeviceButton;
    private Button wipeDataButton;
    private Button setPasswordButton;
    private Button disableCameraButton;
    private Button setLockTimeoutButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initializeComponents();
        setupButtons();
        updateAdminStatus();
    }

    private void initializeComponents() {
        devicePolicyManager = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
        deviceAdminComponent = new ComponentName(this, DeviceAdminReceiver.class);

        adminStatusText = findViewById(R.id.admin_status_text);
        activateButton = findViewById(R.id.activate_button);
        lockDeviceButton = findViewById(R.id.lock_device_button);
        wipeDataButton = findViewById(R.id.wipe_data_button);
        setPasswordButton = findViewById(R.id.set_password_button);
        disableCameraButton = findViewById(R.id.disable_camera_button);
        setLockTimeoutButton = findViewById(R.id.set_lock_timeout_button);
    }

    private void setupButtons() {
        activateButton.setOnClickListener(v -> requestDeviceAdminActivation());
        lockDeviceButton.setOnClickListener(v -> lockDevice());
        wipeDataButton.setOnClickListener(v -> wipeData());
        setPasswordButton.setOnClickListener(v -> setPasswordPolicy());
        disableCameraButton.setOnClickListener(v -> toggleCamera());
        setLockTimeoutButton.setOnClickListener(v -> setLockTimeout());
    }

    private void requestDeviceAdminActivation() {
        if (!devicePolicyManager.isAdminActive(deviceAdminComponent)) {
            Intent intent = new Intent(DevicePolicyManager.ACTION_ADD_DEVICE_ADMIN);
            intent.putExtra(DevicePolicyManager.EXTRA_DEVICE_ADMIN, deviceAdminComponent);
            intent.putExtra(DevicePolicyManager.EXTRA_ADD_EXPLANATION, 
                    getString(R.string.device_admin_explanation));
            startActivityForResult(intent, 1);
        } else {
            deactivateDeviceAdmin();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 1) {
            updateAdminStatus();
        }
    }

    private void deactivateDeviceAdmin() {
        if (devicePolicyManager.isAdminActive(deviceAdminComponent)) {
            devicePolicyManager.removeActiveAdmin(deviceAdminComponent);
            updateAdminStatus();
            Toast.makeText(this, R.string.device_admin_deactivated, Toast.LENGTH_SHORT).show();
        }
    }

    private void updateAdminStatus() {
        boolean isActive = devicePolicyManager.isAdminActive(deviceAdminComponent);
        if (isActive) {
            adminStatusText.setText(R.string.device_admin_active);
            adminStatusText.setTextColor(getResources().getColor(android.R.color.holo_green_dark));
            activateButton.setText(R.string.deactivate_device_admin);
            enableControlButtons(true);
        } else {
            adminStatusText.setText(R.string.device_admin_inactive);
            adminStatusText.setTextColor(getResources().getColor(android.R.color.holo_red_dark));
            activateButton.setText(R.string.activate_device_admin);
            enableControlButtons(false);
        }
    }

    private void enableControlButtons(boolean enabled) {
        lockDeviceButton.setEnabled(enabled);
        wipeDataButton.setEnabled(enabled);
        setPasswordButton.setEnabled(enabled);
        disableCameraButton.setEnabled(enabled);
        setLockTimeoutButton.setEnabled(enabled);
    }

    private void lockDevice() {
        if (devicePolicyManager.isAdminActive(deviceAdminComponent)) {
            devicePolicyManager.lockNow();
            Toast.makeText(this, R.string.device_locked, Toast.LENGTH_SHORT).show();
        }
    }

    private void wipeData() {
        if (devicePolicyManager.isAdminActive(deviceAdminComponent)) {
            devicePolicyManager.wipeData(0);
            Toast.makeText(this, R.string.data_wiping, Toast.LENGTH_SHORT).show();
        }
    }

    private void setPasswordPolicy() {
        if (devicePolicyManager.isAdminActive(deviceAdminComponent)) {
            devicePolicyManager.setPasswordQuality(deviceAdminComponent, 
                    DevicePolicyManager.PASSWORD_QUALITY_NUMERIC);
            devicePolicyManager.setPasswordMinimumLength(deviceAdminComponent, 6);
            Toast.makeText(this, R.string.password_policy_set, Toast.LENGTH_SHORT).show();
        }
    }

    private void toggleCamera() {
        if (devicePolicyManager.isAdminActive(deviceAdminComponent)) {
            boolean currentStatus = devicePolicyManager.getCameraDisabled(deviceAdminComponent);
            devicePolicyManager.setCameraDisabled(deviceAdminComponent, !currentStatus);
            
            String message = !currentStatus ? 
                    R.string.camera_disabled : R.string.camera_enabled;
            Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
            
            disableCameraButton.setText(!currentStatus ? 
                    R.string.enable_camera : R.string.disable_camera);
        }
    }

    private void setLockTimeout() {
        if (devicePolicyManager.isAdminActive(deviceAdminComponent)) {
            devicePolicyManager.setMaximumTimeToLock(deviceAdminComponent, 60000);
            Toast.makeText(this, R.string.lock_timeout_set, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        updateAdminStatus();
    }
}
