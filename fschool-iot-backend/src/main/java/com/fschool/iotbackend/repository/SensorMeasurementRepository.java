package com.fschool.iotbackend.repository;

import com.fschool.iotbackend.entity.SensorMeasurementEntity;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;

import java.util.List;
import java.util.Optional;

public interface SensorMeasurementRepository extends JpaRepository<SensorMeasurementEntity, Long> {

    Optional<SensorMeasurementEntity> findByDeviceIdAndBootIdAndSequence(
            String deviceId,
            String bootId,
            Long sequence
    );

    Optional<SensorMeasurementEntity> findFirstByDeviceIdOrderByMeasuredAtDesc(String deviceId);

    List<SensorMeasurementEntity> findByDeviceIdOrderByMeasuredAtDesc(
            String deviceId,
            Pageable pageable
    );

    @Query("select distinct measurement.deviceId from SensorMeasurementEntity measurement "
            + "order by measurement.deviceId")
    List<String> findDistinctDeviceIds();
}
