# Fschool IoT Backend

Spring Boot backend nay bam truc tiep payload BLE cua firmware
`Code_vape_detector`:

```json
{"temp":28.5,"humid":55.2,"pm25":120,"voc":27450,"status":1}
```

Web gateway bo sung `deviceId`, `bootId`, `sequence`, `timestamp` va `buffered`.
Endpoint ghi du lieu la `POST /api/v1/telemetry/ble`.

## Cac gia tri duoc luu

- `temperature`, `humidity`: SHT3x
- `pm25`: PMS7003
- `sraw_voc`: raw ticks cua SGP41
- `edge_status`: ket qua `0 = CLEAN`, `1 = VAPE_DETECTED` tu ESP32-S3

Tat ca gia tri cam bien tren la bat buoc trong request, entity va database. Backend
khong suy dien them du lieu phan cung ma firmware khong gui.

## Chay local

```powershell
.\mvnw.cmd clean package
java -jar target\fschool-iot-backend-0.0.1-SNAPSHOT.jar --spring.profiles.active=h2
```

H2 local dung file `data/fschool-iot-firmware.mv.db`; file database cu
`data/fschool-iot.mv.db` duoc giu nguyen de co the khoi phuc du lieu legacy.

## API

- `POST /api/v1/telemetry/ble`
- `GET /api/v1/telemetry/devices`
- `GET /api/v1/telemetry/{deviceId}/latest`
- `GET /api/v1/telemetry/{deviceId}/history?limit=60`
- `GET /api/v1/telemetry/{deviceId}/stream`

Schema H2 nam trong `src/main/resources/h2-schema.sql`. Schema va migration
SQL Server nam trong `src/main/resources/sqlserver-schema.sql`.
