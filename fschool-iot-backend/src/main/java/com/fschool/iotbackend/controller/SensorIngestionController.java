package com.fschool.iotbackend.controller;

import com.fschool.iotbackend.dto.request.BleGatewayDiagnosticRequest;
import com.fschool.iotbackend.dto.request.BleTelemetryRequest;
import com.fschool.iotbackend.dto.response.IngestionResponse;
import com.fschool.iotbackend.dto.response.TelemetryChartPointResponse;
import com.fschool.iotbackend.dto.response.TelemetrySnapshotResponse;
import com.fschool.iotbackend.service.BleTelemetryService;
import com.fschool.iotbackend.service.SensorIngestionService;
import com.fschool.iotbackend.service.TelemetryStreamService;
import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;

@RestController
@RequestMapping("/api/v1/telemetry")
public class SensorIngestionController {

    private static final Logger LOGGER = LoggerFactory.getLogger("BLE_GATEWAY");

    private final SensorIngestionService queryService;
    private final BleTelemetryService bleTelemetryService;
    private final TelemetryStreamService streamService;

    public SensorIngestionController(
            SensorIngestionService queryService,
            BleTelemetryService bleTelemetryService,
            TelemetryStreamService streamService
    ) {
        this.queryService = queryService;
        this.bleTelemetryService = bleTelemetryService;
        this.streamService = streamService;
    }

    /** Nhan dung 5 truong firmware sau khi FE bo sung metadata gateway. */
    @PostMapping("/ble")
    @ResponseStatus(HttpStatus.CREATED)
    public IngestionResponse ingestBle(@Valid @RequestBody BleTelemetryRequest request) {
        return bleTelemetryService.ingest(request);
    }

    /** Dua log cua Web Bluetooth vao console BE de doi chieu cung telemetry. */
    @PostMapping("/ble-diagnostics")
    @ResponseStatus(HttpStatus.NO_CONTENT)
    public void logBleDiagnostic(@Valid @RequestBody BleGatewayDiagnosticRequest request) {
        var template = "[BLE-GATEWAY] clientAt={} event={} browserSession={} connection={} "
                + "device={} message={} details={}";
        var arguments = new Object[]{
                request.timestamp(), request.event(), request.browserSessionId(),
                request.connectionId(), request.deviceId(), request.message(), request.details()
        };
        switch (request.level()) {
            case "error" -> LOGGER.error(template, arguments);
            case "warn" -> LOGGER.warn(template, arguments);
            default -> LOGGER.info(template, arguments);
        }
    }

    @GetMapping("/{deviceId}/latest")
    public TelemetrySnapshotResponse latest(@PathVariable String deviceId) {
        return queryService.getLatest(deviceId);
    }

    @GetMapping("/{deviceId}/history")
    public List<TelemetryChartPointResponse> history(
            @PathVariable String deviceId,
            @RequestParam(defaultValue = "60") int limit
    ) {
        return queryService.getHistory(deviceId, limit);
    }

    @GetMapping("/devices")
    public List<String> devices() {
        return queryService.getDeviceIds();
    }

    @GetMapping(value = "/{deviceId}/stream", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
    public SseEmitter stream(@PathVariable String deviceId) {
        return streamService.subscribe(deviceId);
    }
}
