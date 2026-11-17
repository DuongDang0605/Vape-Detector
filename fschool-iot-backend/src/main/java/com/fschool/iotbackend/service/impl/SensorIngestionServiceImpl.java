package com.fschool.iotbackend.service.impl;

import com.fschool.iotbackend.dto.response.TelemetryChartPointResponse;
import com.fschool.iotbackend.dto.response.TelemetrySnapshotResponse;
import com.fschool.iotbackend.exception.ResourceNotFoundException;
import com.fschool.iotbackend.mapper.SensorTelemetryMapper;
import com.fschool.iotbackend.repository.SensorMeasurementRepository;
import com.fschool.iotbackend.service.SensorIngestionService;
import org.springframework.data.domain.PageRequest;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.Collections;
import java.util.List;

@Service
public class SensorIngestionServiceImpl implements SensorIngestionService {

    private final SensorMeasurementRepository repository;
    private final SensorTelemetryMapper mapper;

    public SensorIngestionServiceImpl(
            SensorMeasurementRepository repository,
            SensorTelemetryMapper mapper
    ) {
        this.repository = repository;
        this.mapper = mapper;
    }

    @Override
    @Transactional(readOnly = true)
    public TelemetrySnapshotResponse getLatest(String deviceId) {
        return repository.findFirstByDeviceIdOrderByMeasuredAtDesc(deviceId)
                .map(mapper::toSnapshot)
                .orElseThrow(() -> new ResourceNotFoundException(
                        "No telemetry found for device: " + deviceId));
    }

    @Override
    @Transactional(readOnly = true)
    public List<TelemetryChartPointResponse> getHistory(String deviceId, int limit) {
        int safeLimit = Math.max(1, Math.min(limit, 500));
        var newestFirst = repository.findByDeviceIdOrderByMeasuredAtDesc(
                deviceId, PageRequest.of(0, safeLimit));
        Collections.reverse(newestFirst);
        return newestFirst.stream().map(mapper::toChartPoint).toList();
    }

    @Override
    @Transactional(readOnly = true)
    public List<String> getDeviceIds() {
        return repository.findDistinctDeviceIds();
    }
}
