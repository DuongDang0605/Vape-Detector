package com.fschool.iotbackend.exception;

import java.time.Instant;
import java.util.Map;

public record ApiErrorResponse(
        Instant timestamp,
        String code,
        String message,
        String requestId,
        String path,
        boolean retryable,
        Map<String, String> fieldErrors
) {
}
