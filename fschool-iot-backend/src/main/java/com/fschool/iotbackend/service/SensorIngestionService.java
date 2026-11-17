package com.fschool.iotbackend.service;

import com.fschool.iotbackend.dto.response.TelemetryChartPointResponse;
import com.fschool.iotbackend.dto.response.TelemetrySnapshotResponse;

import java.util.List;

public interface SensorIngestionService {
    TelemetrySnapshotResponse getLatest(String deviceId);
    List<TelemetryChartPointResponse> getHistory(String deviceId, int limit);
    List<String> getDeviceIds();
}
