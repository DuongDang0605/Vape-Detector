package com.fschool.iotbackend.mapper;

import com.fschool.iotbackend.domain.VapeEdgeStatus;
import com.fschool.iotbackend.dto.request.BleTelemetryRequest;
import com.fschool.iotbackend.dto.response.IngestionResponse;
import com.fschool.iotbackend.dto.response.TelemetryChartPointResponse;
import com.fschool.iotbackend.dto.response.TelemetrySnapshotResponse;
import com.fschool.iotbackend.entity.SensorMeasurementEntity;
import org.springframework.stereotype.Component;

import java.time.Instant;

@Component
public class SensorTelemetryMapper {

    public SensorMeasurementEntity toEntity(
            BleTelemetryRequest request,
            Instant receivedAt,
            String rawPayload
    ) {
        return new SensorMeasurementEntity(
                request.deviceId(), request.bootId(), request.sequence(), request.timestamp(),
                receivedAt, request.temp(), request.humid(), request.pm25(), request.voc(),
                VapeEdgeStatus.fromCode(request.status()), request.buffered(), rawPayload
        );
    }

    public IngestionResponse toIngestionResponse(SensorMeasurementEntity entity, boolean duplicate) {
        return new IngestionResponse(
                entity.getId(), entity.getDeviceId(), entity.getSequence(),
                entity.getEdgeStatus(), entity.getReceivedAt(), duplicate
        );
    }

    public TelemetrySnapshotResponse toSnapshot(SensorMeasurementEntity entity) {
        return new TelemetrySnapshotResponse(
                entity.getId(), entity.getDeviceId(), entity.getMeasuredAt(),
                entity.getReceivedAt(), entity.getSequence(), entity.getTemperature(),
                entity.getHumidity(), entity.getPm25(), entity.getSrawVoc(),
                entity.getEdgeStatus(), entity.isBuffered()
        );
    }

    public TelemetryChartPointResponse toChartPoint(SensorMeasurementEntity entity) {
        return new TelemetryChartPointResponse(
                entity.getId(), entity.getDeviceId(), entity.getMeasuredAt(),
                entity.getReceivedAt(), entity.getSequence(), entity.getTemperature(),
                entity.getHumidity(), entity.getPm25(), entity.getSrawVoc(),
                entity.getEdgeStatus(), entity.isBuffered()
        );
    }
}
