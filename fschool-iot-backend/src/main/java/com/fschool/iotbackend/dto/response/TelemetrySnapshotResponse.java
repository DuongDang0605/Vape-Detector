package com.fschool.iotbackend.dto.response;

import com.fschool.iotbackend.domain.VapeEdgeStatus;

import java.time.Instant;

public record TelemetrySnapshotResponse(
        long measurementId,
        String deviceId,
        Instant measuredAt,
        Instant receivedAt,
        long sequence,
        double temperature,
        double humidity,
        int pm25,
        int srawVoc,
        VapeEdgeStatus edgeStatus,
        boolean buffered
) {
}
