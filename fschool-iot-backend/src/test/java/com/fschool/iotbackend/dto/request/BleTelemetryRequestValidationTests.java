package com.fschool.iotbackend.dto.request;

import jakarta.validation.Validation;
import org.junit.jupiter.api.Test;

import java.time.Instant;

import static org.junit.jupiter.api.Assertions.assertTrue;

class BleTelemetryRequestValidationTests {

    @Test
    void rejectsLegacyStatusOutsideFinalFirmwareContract() {
        try (var factory = Validation.buildDefaultValidatorFactory()) {
            var validator = factory.getValidator();
            var request = new BleTelemetryRequest(
                    "ESP32S3-VAPE-TEST", "boot-test", 1L, Instant.now(),
                    28.5, 55.2, 120, 27_450, 2, false
            );

            assertTrue(validator.validate(request).stream()
                    .anyMatch(violation -> violation.getPropertyPath().toString().equals("status")));
        }
    }
}
