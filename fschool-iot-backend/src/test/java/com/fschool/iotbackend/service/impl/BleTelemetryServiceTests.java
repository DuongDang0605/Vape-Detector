package com.fschool.iotbackend.service.impl;

import com.fschool.iotbackend.domain.VapeEdgeStatus;
import com.fschool.iotbackend.dto.request.BleTelemetryRequest;
import com.fschool.iotbackend.mapper.SensorTelemetryMapper;
import com.fschool.iotbackend.repository.SensorMeasurementRepository;
import com.fschool.iotbackend.service.BleTelemetryService;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.Locale;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest
@Transactional
class BleTelemetryServiceTests {

    @Autowired
    private BleTelemetryService service;

    @Autowired
    private SensorMeasurementRepository repository;

    @Autowired
    private SensorTelemetryMapper mapper;

    @Autowired
    private JdbcTemplate jdbcTemplate;

    @Test
    void storesExactlyTheFiveFirmwareValues() {
        var request = packet("ESP32S3-VAPE-A101", "browser-session-01", 7L, 1);

        var response = service.ingest(request);
        var stored = repository.findById(response.measurementId()).orElseThrow();
        var point = mapper.toChartPoint(stored);

        assertEquals(VapeEdgeStatus.VAPE_DETECTED, response.edgeStatus());
        assertEquals(28.5, point.temperature());
        assertEquals(55.2, point.humidity());
        assertEquals(120, point.pm25());
        assertEquals(27_450, point.srawVoc());
        assertEquals(VapeEdgeStatus.VAPE_DETECTED, point.edgeStatus());
        assertFalse(response.duplicate());
    }

    @Test
    void keepsIdempotencyForTheBrowserGateway() {
        var request = packet("ESP32S3-VAPE-A102", "browser-session-02", 0L, 0);

        var first = service.ingest(request);
        var duplicate = service.ingest(request);

        assertEquals(VapeEdgeStatus.CLEAN, first.edgeStatus());
        assertTrue(duplicate.duplicate());
        assertEquals(first.measurementId(), duplicate.measurementId());
    }

    @Test
    void databaseSchemaContainsNoAbsentSensorColumns() {
        var columns = jdbcTemplate.queryForList(
                        "select column_name from information_schema.columns "
                                + "where upper(table_name) = 'SENSOR_MEASUREMENT'",
                        String.class)
                .stream()
                .map(value -> value.toLowerCase(Locale.ROOT))
                .toList();

        assertTrue(columns.containsAll(java.util.List.of(
                "temperature", "humidity", "pm25", "sraw_voc", "edge_status")));
        assertFalse(columns.stream().anyMatch(name ->
                name.contains("audio") || name.contains("mfcc")
                        || name.equals("voc_index") || name.equals("delta_voc")));
    }

    private static BleTelemetryRequest packet(
            String deviceId,
            String bootId,
            long sequence,
            int status
    ) {
        return new BleTelemetryRequest(
                deviceId, bootId, sequence, Instant.parse("2026-08-21T08:00:00Z"),
                28.5, 55.2, 120, 27_450, status, false
        );
    }
}
