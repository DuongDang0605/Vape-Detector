package com.fschool.iotbackend.service;

import org.springframework.stereotype.Service;
import org.springframework.transaction.support.TransactionSynchronization;
import org.springframework.transaction.support.TransactionSynchronizationManager;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

/** Fan-out nhe cho dashboard; REST polling van la fallback khi SSE bi ngat. */
@Service
public class TelemetryStreamService {

    private static final long STREAM_TIMEOUT_MS = 30L * 60L * 1000L;
    private final ConcurrentHashMap<String, CopyOnWriteArrayList<SseEmitter>> emitters =
            new ConcurrentHashMap<>();

    public SseEmitter subscribe(String deviceId) {
        var emitter = new SseEmitter(STREAM_TIMEOUT_MS);
        var deviceEmitters = emitters.computeIfAbsent(deviceId, ignored -> new CopyOnWriteArrayList<>());
        deviceEmitters.add(emitter);
        Runnable cleanup = () -> remove(deviceId, emitter);
        emitter.onCompletion(cleanup);
        emitter.onTimeout(cleanup);
        emitter.onError(ignored -> cleanup.run());
        try {
            emitter.send(SseEmitter.event().comment("connected").reconnectTime(3_000));
        } catch (IOException exception) {
            cleanup.run();
        }
        return emitter;
    }

    public void publish(String deviceId, Object payload) {
        var deviceEmitters = emitters.get(deviceId);
        if (deviceEmitters == null) return;
        for (var emitter : deviceEmitters) {
            try {
                emitter.send(SseEmitter.event().data(payload).reconnectTime(3_000));
            } catch (IOException | IllegalStateException exception) {
                remove(deviceId, emitter);
            }
        }
    }

    /** Chi phat khi ban ghi da commit de dashboard khong thay du lieu "ma". */
    public void publishAfterCommit(String deviceId, Object payload) {
        if (!TransactionSynchronizationManager.isSynchronizationActive()) {
            publish(deviceId, payload);
            return;
        }
        TransactionSynchronizationManager.registerSynchronization(new TransactionSynchronization() {
            @Override
            public void afterCommit() {
                publish(deviceId, payload);
            }
        });
    }

    private void remove(String deviceId, SseEmitter emitter) {
        var deviceEmitters = emitters.get(deviceId);
        if (deviceEmitters == null) return;
        deviceEmitters.remove(emitter);
        if (deviceEmitters.isEmpty()) emitters.remove(deviceId, deviceEmitters);
    }
}
