package com.fschool.iotbackend.dto.response;

import com.fschool.iotbackend.domain.VapeEdgeStatus;

import java.time.Instant;

public record IngestionResponse(
        long measurementId,
        String deviceId,
        long sequence,
        VapeEdgeStatus edgeStatus,
        Instant receivedAt,
        boolean duplicate
) {
}
