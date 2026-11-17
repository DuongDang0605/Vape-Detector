package com.fschool.iotbackend.entity;

import com.fschool.iotbackend.domain.VapeEdgeStatus;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.Lob;
import jakarta.persistence.Table;
import jakarta.persistence.UniqueConstraint;

import java.time.Instant;

/** Ban ghi 1:1 voi payload ma firmware ESP32-S3 hien tai phat qua BLE. */
@Entity
@Table(
        name = "sensor_measurement",
        uniqueConstraints = @UniqueConstraint(
                name = "uk_measurement_device_boot_sequence",
                columnNames = {"device_id", "boot_id", "sequence_number"}
        )
)
public class SensorMeasurementEntity {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "device_id", nullable = false, length = 100)
    private String deviceId;

    @Column(name = "boot_id", nullable = false, length = 100)
    private String bootId;

    @Column(name = "sequence_number", nullable = false)
    private long sequence;

    @Column(name = "measured_at", nullable = false)
    private Instant measuredAt;

    @Column(name = "received_at", nullable = false)
    private Instant receivedAt;

    @Column(name = "temperature", nullable = false)
    private double temperature;

    @Column(name = "humidity", nullable = false)
    private double humidity;

    @Column(name = "pm25", nullable = false)
    private int pm25;

    /** SRAW VOC raw ticks tu SGP41, khong phai VOC Index. */
    @Column(name = "sraw_voc", nullable = false)
    private int srawVoc;

    @Enumerated(EnumType.STRING)
    @Column(name = "edge_status", nullable = false, length = 40)
    private VapeEdgeStatus edgeStatus;

    @Column(name = "buffered_message", nullable = false)
    private boolean buffered;

    @Lob
    @Column(name = "raw_payload", nullable = false)
    private String rawPayload;

    protected SensorMeasurementEntity() {
    }

    public SensorMeasurementEntity(
            String deviceId,
            String bootId,
            long sequence,
            Instant measuredAt,
            Instant receivedAt,
            double temperature,
            double humidity,
            int pm25,
            int srawVoc,
            VapeEdgeStatus edgeStatus,
            boolean buffered,
            String rawPayload
    ) {
        this.deviceId = deviceId;
        this.bootId = bootId;
        this.sequence = sequence;
        this.measuredAt = measuredAt;
        this.receivedAt = receivedAt;
        this.temperature = temperature;
        this.humidity = humidity;
        this.pm25 = pm25;
        this.srawVoc = srawVoc;
        this.edgeStatus = edgeStatus;
        this.buffered = buffered;
        this.rawPayload = rawPayload;
    }

    public Long getId() { return id; }
    public String getDeviceId() { return deviceId; }
    public String getBootId() { return bootId; }
    public long getSequence() { return sequence; }
    public Instant getMeasuredAt() { return measuredAt; }
    public Instant getReceivedAt() { return receivedAt; }
    public double getTemperature() { return temperature; }
    public double getHumidity() { return humidity; }
    public int getPm25() { return pm25; }
    public int getSrawVoc() { return srawVoc; }
    public VapeEdgeStatus getEdgeStatus() { return edgeStatus; }
    public boolean isBuffered() { return buffered; }
    public String getRawPayload() { return rawPayload; }
}
