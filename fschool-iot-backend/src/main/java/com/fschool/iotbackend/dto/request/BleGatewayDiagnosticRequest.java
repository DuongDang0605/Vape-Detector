package com.fschool.iotbackend.dto.request;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;

import java.time.Instant;
import java.util.Map;

/**
 * Su kien chan doan tu Web Bluetooth gateway. Chi ghi log, khong luu vao bang
 * sensor_measurement va khong lam thay doi schema cam bien.
 */
public record BleGatewayDiagnosticRequest(
        @NotNull Instant timestamp,
        @NotBlank @Pattern(regexp = "info|warn|error") String level,
        @NotBlank @Size(max = 80) String event,
        @NotBlank @Size(max = 500) String message,
        @NotBlank @Size(max = 100) String browserSessionId,
        @NotNull @Size(max = 100) String connectionId,
        @NotNull @Size(max = 128) String deviceId,
        @NotNull @Size(max = 20) Map<String, Object> details
) {
}
