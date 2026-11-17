package com.fschool.iotbackend.domain;

/** Trang thai 0..1 do firmware Final phat qua BLE. */
public enum VapeEdgeStatus {
    CLEAN(0),
    VAPE_DETECTED(1);

    private final int code;

    VapeEdgeStatus(int code) {
        this.code = code;
    }

    public int getCode() {
        return code;
    }

    public static VapeEdgeStatus fromCode(int code) {
        for (var status : values()) {
            if (status.code == code) return status;
        }
        throw new IllegalArgumentException("Unsupported vape edge status: " + code);
    }
}
