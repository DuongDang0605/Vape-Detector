package com.fschool.iotbackend.dto.request;

import jakarta.validation.constraints.DecimalMax;
import jakarta.validation.constraints.DecimalMin;
import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.PositiveOrZero;

import java.time.Instant;

/**
 * Goi tin do Web Bluetooth gateway bo sung metadata quanh JSON co dinh cua
 * firmware: temp, humid, pm25, voc va status.
 */
public record BleTelemetryRequest(
        @NotBlank String deviceId,
        @NotBlank String bootId,
        @NotNull @PositiveOrZero Long sequence,
        @NotNull Instant timestamp,
        @NotNull @DecimalMin("-45.0") @DecimalMax("130.0") Double temp,
        @NotNull @DecimalMin("0.0") @DecimalMax("100.0") Double humid,
        @NotNull @Min(0) @Max(65535) Integer pm25,
        @NotNull @Min(0) @Max(65535) Integer voc,
        @NotNull @Min(0) @Max(1) Integer status,
        boolean buffered
) {
}
