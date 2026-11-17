param(
    [string]$ApiBase = "http://localhost:8080/api/v1/telemetry",
    [string]$DeviceId = "ESP32S3-VAPE-DEMO"
)

$bootId = "local-demo-$([DateTimeOffset]::UtcNow.ToUnixTimeSeconds())"
$samples = @(
    @{ temp = 27.8; humid = 54.2; pm25 = 12; voc = 28100; status = 0 },
    @{ temp = 27.9; humid = 54.5; pm25 = 88; voc = 26920; status = 1 },
    @{ temp = 28.0; humid = 92.0; pm25 = 32; voc = 28040; status = 0 }
)

for ($index = 0; $index -lt $samples.Count; $index++) {
    $sample = $samples[$index]
    $payload = [ordered]@{
        deviceId = $DeviceId
        bootId = $bootId
        sequence = $index
        timestamp = [DateTime]::UtcNow.AddSeconds($index).ToString("o")
        temp = $sample.temp
        humid = $sample.humid
        pm25 = $sample.pm25
        voc = $sample.voc
        status = $sample.status
        buffered = $false
    }

    Invoke-RestMethod -Method Post -Uri "$ApiBase/ble" `
        -ContentType "application/json" -Body ($payload | ConvertTo-Json)
}
