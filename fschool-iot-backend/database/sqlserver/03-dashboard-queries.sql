USE [FschoolIoT];
GO

SELECT
    device_id,
    COUNT(*) AS measurement_count,
    MIN(measured_at) AS first_measurement,
    MAX(measured_at) AS latest_measurement,
    MAX(pm25) AS peak_pm25,
    MIN(sraw_voc) AS lowest_sraw_voc,
    SUM(CASE WHEN edge_status = 'VAPE_DETECTED' THEN 1 ELSE 0 END) AS vape_alert_count
FROM dbo.sensor_measurement
GROUP BY device_id
ORDER BY device_id;
GO

SELECT TOP (100)
    id,
    device_id,
    measured_at,
    temperature,
    humidity,
    pm25,
    sraw_voc,
    edge_status,
    buffered_message
FROM dbo.sensor_measurement
ORDER BY measured_at DESC;
GO
