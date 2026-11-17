USE [master];
GO

IF DB_ID(N'FschoolIoT') IS NULL
BEGIN
    CREATE DATABASE [FschoolIoT];
END;
GO

ALTER DATABASE [FschoolIoT] SET RECOVERY SIMPLE;
GO

USE [FschoolIoT];
GO

IF NOT EXISTS (
    SELECT 1
    FROM sys.extended_properties
    WHERE name = N'FschoolIoTDescription'
)
BEGIN
    EXEC sys.sp_addextendedproperty
        @name = N'FschoolIoTDescription',
        @value = N'Database lưu dữ liệu telemetry từ các node cảm biến Fschool IoT';
END;
GO
