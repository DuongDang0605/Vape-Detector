package com.fschool.iotbackend.exception;

/**
 * Danh muc loi cong khai cua API. Message o day an toan de hien thi cho nguoi dung;
 * chi tiet ky thuat va stack trace chi duoc ghi trong log cua backend.
 */
public enum ApiErrorCode {
    PAYLOAD_VALIDATION_FAILED(
            "Dữ liệu thiết bị gửi lên chưa đầy đủ hoặc chưa hợp lệ.", false),
    MALFORMED_REQUEST(
            "Hệ thống chưa thể đọc gói dữ liệu này. Vui lòng kiểm tra lại dữ liệu gửi lên.", false),
    REQUEST_PARAMETER_INVALID(
            "Thông tin yêu cầu chưa hợp lệ. Vui lòng kiểm tra và thử lại.", false),
    RESOURCE_NOT_FOUND(
            "Chưa tìm thấy dữ liệu phù hợp. Thiết bị có thể chưa gửi bản ghi đầu tiên.", false),
    OPERATION_NOT_SUPPORTED(
            "Thao tác này chưa được hệ thống hỗ trợ.", false),
    CONTENT_TYPE_NOT_SUPPORTED(
            "Định dạng dữ liệu gửi lên chưa được hệ thống hỗ trợ.", false),
    DATA_CONFLICT(
            "Dữ liệu vừa thay đổi hoặc đã tồn tại. Vui lòng tải lại và thử lần nữa.", true),
    STORAGE_TEMPORARILY_UNAVAILABLE(
            "Hệ thống lưu trữ đang tạm thời gián đoạn. Vui lòng thử lại sau ít phút.", true),
    REQUEST_TIMEOUT(
            "Yêu cầu mất nhiều thời gian hơn dự kiến. Vui lòng thử lại.", true),
    INTERNAL_ERROR(
            "Hệ thống đang gặp sự cố tạm thời. Vui lòng thử lại sau ít phút.", true);

    private final String userMessage;
    private final boolean retryable;

    ApiErrorCode(String userMessage, boolean retryable) {
        this.userMessage = userMessage;
        this.retryable = retryable;
    }

    public String getUserMessage() {
        return userMessage;
    }

    public boolean isRetryable() {
        return retryable;
    }
}
