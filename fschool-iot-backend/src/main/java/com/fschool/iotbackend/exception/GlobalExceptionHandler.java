package com.fschool.iotbackend.exception;

import jakarta.servlet.http.HttpServletRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.http.HttpStatus;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.web.HttpMediaTypeNotAcceptableException;
import org.springframework.web.HttpMediaTypeNotSupportedException;
import org.springframework.web.HttpRequestMethodNotSupportedException;
import org.springframework.web.bind.MissingServletRequestParameterException;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestControllerAdvice;
import org.springframework.web.context.request.async.AsyncRequestTimeoutException;
import org.springframework.web.method.annotation.MethodArgumentTypeMismatchException;
import org.springframework.web.servlet.resource.NoResourceFoundException;

import java.time.Clock;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;

@RestControllerAdvice
public class GlobalExceptionHandler {

    private static final Logger LOGGER = LoggerFactory.getLogger(GlobalExceptionHandler.class);

    private final Clock clock;

    public GlobalExceptionHandler(Clock clock) {
        this.clock = clock;
    }

    @ExceptionHandler(MethodArgumentNotValidException.class)
    @ResponseStatus(HttpStatus.BAD_REQUEST)
    public ApiErrorResponse validation(
            MethodArgumentNotValidException exception,
            HttpServletRequest request
    ) {
        Map<String, String> fieldErrors = new LinkedHashMap<>();
        exception.getBindingResult().getFieldErrors().forEach(error ->
                fieldErrors.putIfAbsent(error.getField(), error.getDefaultMessage()));
        exception.getBindingResult().getGlobalErrors().forEach(error ->
                fieldErrors.putIfAbsent(error.getObjectName(), error.getDefaultMessage()));
        return error(HttpStatus.BAD_REQUEST, ApiErrorCode.PAYLOAD_VALIDATION_FAILED,
                request, fieldErrors, exception);
    }

    @ExceptionHandler(HttpMessageNotReadableException.class)
    @ResponseStatus(HttpStatus.BAD_REQUEST)
    public ApiErrorResponse unreadable(
            HttpMessageNotReadableException exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.BAD_REQUEST, ApiErrorCode.MALFORMED_REQUEST,
                request, Map.of(), exception);
    }

    @ExceptionHandler(ResourceNotFoundException.class)
    @ResponseStatus(HttpStatus.NOT_FOUND)
    public ApiErrorResponse notFound(
            ResourceNotFoundException exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.NOT_FOUND, ApiErrorCode.RESOURCE_NOT_FOUND,
                request, Map.of(), exception);
    }

    @ExceptionHandler(NoResourceFoundException.class)
    @ResponseStatus(HttpStatus.NOT_FOUND)
    public ApiErrorResponse routeNotFound(
            NoResourceFoundException exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.NOT_FOUND, ApiErrorCode.RESOURCE_NOT_FOUND,
                request, Map.of(), exception);
    }

    @ExceptionHandler({
            MissingServletRequestParameterException.class,
            MethodArgumentTypeMismatchException.class
    })
    @ResponseStatus(HttpStatus.BAD_REQUEST)
    public ApiErrorResponse invalidParameter(
            Exception exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.BAD_REQUEST, ApiErrorCode.REQUEST_PARAMETER_INVALID,
                request, Map.of(), exception);
    }

    @ExceptionHandler(HttpRequestMethodNotSupportedException.class)
    @ResponseStatus(HttpStatus.METHOD_NOT_ALLOWED)
    public ApiErrorResponse methodNotAllowed(
            HttpRequestMethodNotSupportedException exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.METHOD_NOT_ALLOWED, ApiErrorCode.OPERATION_NOT_SUPPORTED,
                request, Map.of(), exception);
    }

    @ExceptionHandler({
            HttpMediaTypeNotSupportedException.class,
            HttpMediaTypeNotAcceptableException.class
    })
    @ResponseStatus(HttpStatus.UNSUPPORTED_MEDIA_TYPE)
    public ApiErrorResponse mediaTypeNotSupported(
            Exception exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.UNSUPPORTED_MEDIA_TYPE, ApiErrorCode.CONTENT_TYPE_NOT_SUPPORTED,
                request, Map.of(), exception);
    }

    @ExceptionHandler(DataIntegrityViolationException.class)
    @ResponseStatus(HttpStatus.CONFLICT)
    public ApiErrorResponse dataConflict(
            DataIntegrityViolationException exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.CONFLICT, ApiErrorCode.DATA_CONFLICT,
                request, Map.of(), exception);
    }

    @ExceptionHandler(DataAccessException.class)
    @ResponseStatus(HttpStatus.SERVICE_UNAVAILABLE)
    public ApiErrorResponse storageUnavailable(
            DataAccessException exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.SERVICE_UNAVAILABLE, ApiErrorCode.STORAGE_TEMPORARILY_UNAVAILABLE,
                request, Map.of(), exception);
    }

    @ExceptionHandler(AsyncRequestTimeoutException.class)
    @ResponseStatus(HttpStatus.SERVICE_UNAVAILABLE)
    public ApiErrorResponse timeout(
            AsyncRequestTimeoutException exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.SERVICE_UNAVAILABLE, ApiErrorCode.REQUEST_TIMEOUT,
                request, Map.of(), exception);
    }

    @ExceptionHandler(Exception.class)
    @ResponseStatus(HttpStatus.INTERNAL_SERVER_ERROR)
    public ApiErrorResponse unexpected(
            Exception exception,
            HttpServletRequest request
    ) {
        return error(HttpStatus.INTERNAL_SERVER_ERROR, ApiErrorCode.INTERNAL_ERROR,
                request, Map.of(), exception);
    }

    private ApiErrorResponse error(
            HttpStatus status,
            ApiErrorCode code,
            HttpServletRequest request,
            Map<String, String> fieldErrors,
            Exception exception
    ) {
        String requestId = UUID.randomUUID().toString();
        String path = request.getRequestURI();
        if (status.is5xxServerError()) {
            LOGGER.error("API failure requestId={} code={} method={} path={}",
                    requestId, code.name(), request.getMethod(), path, exception);
        } else {
            LOGGER.warn("API request rejected requestId={} code={} method={} path={} cause={}",
                    requestId, code.name(), request.getMethod(), path,
                    exception.getClass().getSimpleName());
        }
        return new ApiErrorResponse(
                clock.instant(),
                code.name(),
                code.getUserMessage(),
                requestId,
                path,
                code.isRetryable(),
                fieldErrors
        );
    }
}
