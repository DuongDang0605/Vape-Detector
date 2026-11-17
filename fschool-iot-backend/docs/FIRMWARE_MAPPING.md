# Firmware mapping

Nguon su that la payload trong `Code_vape_detector/main/main.c`:

```json
{"temp":28.5,"humid":55.2,"pm25":120,"voc":27450,"status":1}
```

| Firmware | Phan cung | Database | API response |
|---|---|---|---|
| `temp` | SHT3x | `temperature` | `temperature` |
| `humid` | SHT3x | `humidity` | `humidity` |
| `pm25` | PMS7003 | `pm25` | `pm25` |
| `voc` | SGP41 raw ticks | `sraw_voc` | `srawVoc` |
| `status` (`0`/`1`) | Vape logic PM2.5 tren ESP32-S3 | `edge_status` | `edgeStatus` |

Tat ca cot tren co rang buoc `NOT NULL`. Metadata gateway cung bat buoc gom
`device_id`, `boot_id`, `sequence_number`, `measured_at` va `received_at`.
