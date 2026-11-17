package com.fschool.iotbackend.service.impl;

import com.fschool.iotbackend.dto.request.BleTelemetryRequest;
import com.fschool.iotbackend.dto.response.IngestionResponse;
import com.fschool.iotbackend.mapper.SensorTelemetryMapper;
import com.fschool.iotbackend.repository.SensorMeasurementRepository;
import com.fschool.iotbackend.service.BleTelemetryService;
import com.fschool.iotbackend.service.TelemetryStreamService;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import tools.jackson.core.JacksonException;
import tools.jackson.databind.ObjectMapper;

import java.time.Clock;

/** Luu nguyen payload firmware; khong suy dien du lieu cua cam bien khac. */
@Service
public class BleTelemetryServiceImpl implements BleTelemetryService {

    private static final Logger LOGGER = LoggerFactory.getLogger(BleTelemetryServiceImpl.class);

    private final SensorMeasurementRepository repository;
    private final SensorTelemetryMapper mapper;
    private final TelemetryStreamService streamService;
    private final ObjectMapper objectMapper;
    private final Clock clock;

    public BleTelemetryServiceImpl(
            SensorMeasurementRepository repository,
            SensorTelemetryMapper mapper,
            TelemetryStreamService streamService,
            ObjectMapper objectMapper,
            Clock clock
    ) {
        this.repository = repository;
        this.mapper = mapper;
        this.streamService = streamService;
        this.objectMapper = objectMapper;
        this.clock = clock;
    }

    @Override
    @Transactional
    public IngestionResponse ingest(BleTelemetryRequest request) {
        var duplicate = repository.findByDeviceIdAndBootIdAndSequence(
                request.deviceId(), request.bootId(), request.sequence());
        if (duplicate.isPresent()) {
            LOGGER.warn("[BLE-INGEST] duplicate device={} connection={} sequence={} buffered={}",
                    request.deviceId(), request.bootId(), request.sequence(), request.buffered());
            return mapper.toIngestionResponse(duplicate.get(), true);
        }

        var entity = mapper.toEntity(request, clock.instant(), serialize(request));
        var saved = repository.save(entity);
        if (request.sequence() == 0 || request.sequence() % 30 == 0 || request.buffered()) {
            LOGGER.info("[BLE-INGEST] stored device={} connection={} sequence={} buffered={} "
                            + "status={} pm25={} voc={} measuredAt={}",
                    request.deviceId(), request.bootId(), request.sequence(), request.buffered(),
                    request.status(), request.pm25(), request.voc(), request.timestamp());
        }
        streamService.publishAfterCommit(saved.getDeviceId(), mapper.toChartPoint(saved));
        return mapper.toIngestionResponse(saved, false);
    }

    private String serialize(BleTelemetryRequest request) {
        try {
            return objectMapper.writeValueAsString(request);
        } catch (JacksonException exception) {
            throw new IllegalStateException("Cannot serialize BLE telemetry payload", exception);
        }
    }
}
