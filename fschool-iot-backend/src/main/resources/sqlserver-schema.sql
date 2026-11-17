IF OBJECT_ID('dbo.sensor_measurement', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.sensor_measurement (
        id BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY,
        device_id VARCHAR(100) NOT NULL,
        boot_id VARCHAR(100) NOT NULL,
        sequence_number BIGINT NOT NULL,
        measured_at DATETIME2 NOT NULL,
        received_at DATETIME2 NOT NULL,
        temperature FLOAT NOT NULL,
        humidity FLOAT NOT NULL,
        pm25 INT NOT NULL,
        sraw_voc INT NOT NULL,
        edge_status VARCHAR(40) NOT NULL
            CONSTRAINT ck_measurement_edge_status CHECK (edge_status IN ('CLEAN', 'VAPE_DETECTED')),
        buffered_message BIT NOT NULL,
        raw_payload NVARCHAR(MAX) NOT NULL,
        CONSTRAINT uk_measurement_device_boot_sequence
            UNIQUE (device_id, boot_id, sequence_number)
    );
END;

-- Schema cu co pm25_raw, VOC Index va audio. Chi giu lai cac dong BLE du truong.
IF COL_LENGTH('dbo.sensor_measurement', 'pm25_raw') IS NOT NULL
BEGIN
    IF OBJECT_ID('dbo.sensor_measurement_firmware', 'U') IS NOT NULL
        DROP TABLE dbo.sensor_measurement_firmware;

    CREATE TABLE dbo.sensor_measurement_firmware (
        id BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY,
        device_id VARCHAR(100) NOT NULL,
        boot_id VARCHAR(100) NOT NULL,
        sequence_number BIGINT NOT NULL,
        measured_at DATETIME2 NOT NULL,
        received_at DATETIME2 NOT NULL,
        temperature FLOAT NOT NULL,
        humidity FLOAT NOT NULL,
        pm25 INT NOT NULL,
        sraw_voc INT NOT NULL,
        edge_status VARCHAR(40) NOT NULL,
        buffered_message BIT NOT NULL,
        raw_payload NVARCHAR(MAX) NOT NULL,
        CONSTRAINT uk_measurement_firmware_device_boot_sequence
            UNIQUE (device_id, boot_id, sequence_number)
    );

    SET IDENTITY_INSERT dbo.sensor_measurement_firmware ON;
    INSERT INTO dbo.sensor_measurement_firmware (
        id, device_id, boot_id, sequence_number, measured_at, received_at,
        temperature, humidity, pm25, sraw_voc, edge_status,
        buffered_message, raw_payload
    )
    SELECT
        id, device_id, boot_id, sequence_number, measured_at, received_at,
        temperature, humidity, pm25_raw, sraw_voc, edge_status,
        buffered_message, raw_payload
    FROM dbo.sensor_measurement
    WHERE pm25_raw IS NOT NULL
      AND sraw_voc IS NOT NULL
      AND edge_status IS NOT NULL;
    SET IDENTITY_INSERT dbo.sensor_measurement_firmware OFF;

    DROP TABLE dbo.sensor_measurement;
    EXEC sp_rename 'dbo.sensor_measurement_firmware', 'sensor_measurement';
END;

-- Firmware Final chi con hai trang thai. Chuan hoa du lieu cu truoc khi doc.
UPDATE dbo.sensor_measurement
SET edge_status = 'CLEAN'
WHERE edge_status IN ('STEAM_EXCLUDED', 'CHEMICAL_EXCLUDED');

IF NOT EXISTS (
    SELECT 1 FROM sys.indexes
    WHERE name = 'ix_measurement_device_measured_at'
      AND object_id = OBJECT_ID('dbo.sensor_measurement')
)
    CREATE INDEX ix_measurement_device_measured_at
        ON dbo.sensor_measurement(device_id, measured_at DESC);
