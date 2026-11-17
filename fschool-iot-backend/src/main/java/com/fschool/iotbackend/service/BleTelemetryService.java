package com.fschool.iotbackend.service;

import com.fschool.iotbackend.dto.request.BleTelemetryRequest;
import com.fschool.iotbackend.dto.response.IngestionResponse;

public interface BleTelemetryService {
    IngestionResponse ingest(BleTelemetryRequest request);
}
