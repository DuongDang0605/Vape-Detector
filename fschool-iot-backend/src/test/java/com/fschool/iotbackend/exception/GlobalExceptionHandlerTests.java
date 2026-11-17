package com.fschool.iotbackend.exception;

import org.junit.jupiter.api.Test;
import org.springframework.mock.web.MockHttpServletRequest;

import java.time.Clock;
import java.time.Instant;
import java.time.ZoneOffset;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

class GlobalExceptionHandlerTests {

    private final GlobalExceptionHandler handler = new GlobalExceptionHandler(
            Clock.fixed(Instant.parse("2026-08-17T08:00:00Z"), ZoneOffset.UTC));

    @Test
    void returnsFriendlyNotFoundResponseWithoutLeakingInternalMessage() {
        var request = new MockHttpServletRequest("GET", "/api/v1/telemetry/new-device/latest");

        var response = handler.notFound(
                new ResourceNotFoundException("database row 42 was not found"), request);

        assertEquals("RESOURCE_NOT_FOUND", response.code());
        assertEquals(ApiErrorCode.RESOURCE_NOT_FOUND.getUserMessage(), response.message());
        assertFalse(response.message().contains("row 42"));
        assertFalse(response.retryable());
        assertNotNull(response.requestId());
    }

    @Test
    void returnsSafeFallbackAndRequestIdForUnexpectedFailures() {
        var request = new MockHttpServletRequest("GET", "/api/v1/telemetry/devices");

        var response = handler.unexpected(
                new IllegalStateException("secret connection details"), request);

        assertEquals("INTERNAL_ERROR", response.code());
        assertEquals(ApiErrorCode.INTERNAL_ERROR.getUserMessage(), response.message());
        assertFalse(response.message().contains("secret"));
        assertTrue(response.retryable());
        assertNotNull(response.requestId());
    }
}
