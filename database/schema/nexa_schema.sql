
USE nexa_db;

CREATE TABLE incidents (
    incident_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    incident_code VARCHAR(30) NOT NULL UNIQUE,
    title VARCHAR(150) NOT NULL,
    description TEXT,
    incident_type VARCHAR(50) NOT NULL,
    severity ENUM('LOW','MEDIUM','HIGH','CRITICAL') NOT NULL DEFAULT 'MEDIUM',
    status ENUM('REPORTED','VERIFIED','ACTIVE','RESOLVED','CLOSED') NOT NULL DEFAULT 'REPORTED',
    latitude DECIMAL(10,7),
    longitude DECIMAL(10,7),
    address VARCHAR(255),
    reported_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    resolved_at DATETIME,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_incident_status (status),
    INDEX idx_incident_severity (severity),
    INDEX idx_incident_location (latitude, longitude)
) ENGINE=InnoDB;

CREATE TABLE resources (
    resource_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    resource_code VARCHAR(30) NOT NULL UNIQUE,
    resource_name VARCHAR(150) NOT NULL,
    resource_type ENUM('AMBULANCE','FIRE_TRUCK','RESCUE_TEAM','MEDICAL_TEAM','SUPPLIES','SHELTER','HOSPITAL','OTHER') NOT NULL,
    status ENUM('AVAILABLE','DEPLOYED','MAINTENANCE','UNAVAILABLE') NOT NULL DEFAULT 'AVAILABLE',
    total_units INT UNSIGNED NOT NULL DEFAULT 1,
    available_units INT UNSIGNED NOT NULL DEFAULT 1,
    latitude DECIMAL(10,7),
    longitude DECIMAL(10,7),
    address VARCHAR(255),
    last_updated DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_resource_units CHECK (available_units <= total_units),
    INDEX idx_resource_type_status (resource_type, status),
    INDEX idx_resource_location (latitude, longitude)
) ENGINE=InnoDB;

CREATE TABLE emergency_requests (
    request_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    incident_id BIGINT UNSIGNED NOT NULL,
    request_type VARCHAR(80) NOT NULL,
    description TEXT,
    priority ENUM('LOW','MEDIUM','HIGH','CRITICAL') NOT NULL DEFAULT 'MEDIUM',
    quantity_required INT UNSIGNED NOT NULL DEFAULT 1,
    status ENUM('PENDING','PROCESSING','FULFILLED','CANCELLED') NOT NULL DEFAULT 'PENDING',
    requested_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (incident_id) REFERENCES incidents(incident_id),
    INDEX idx_request_priority_status (priority, status),
    INDEX idx_request_incident (incident_id)
) ENGINE=InnoDB;

CREATE TABLE resource_allocations (
    allocation_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    incident_id BIGINT UNSIGNED NOT NULL,
    resource_id BIGINT UNSIGNED NOT NULL,
    request_id BIGINT UNSIGNED,
    allocated_units INT UNSIGNED NOT NULL,
    status ENUM('ASSIGNED','DISPATCHED','ON_SITE','COMPLETED','CANCELLED') NOT NULL DEFAULT 'ASSIGNED',
    assigned_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    released_at DATETIME,
    FOREIGN KEY (incident_id) REFERENCES incidents(incident_id),
    FOREIGN KEY (resource_id) REFERENCES resources(resource_id),
    FOREIGN KEY (request_id) REFERENCES emergency_requests(request_id),
    INDEX idx_allocation_incident (incident_id),
    INDEX idx_allocation_resource (resource_id),
    INDEX idx_allocation_status (status)
) ENGINE=InnoDB;

CREATE TABLE incident_events (
    event_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    incident_id BIGINT UNSIGNED NOT NULL,
    event_type VARCHAR(80) NOT NULL,
    description TEXT NOT NULL,
    event_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (incident_id) REFERENCES incidents(incident_id),
    INDEX idx_event_incident_time (incident_id, event_time)
) ENGINE=InnoDB;

CREATE TABLE network_nodes (
    node_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    node_code VARCHAR(30) NOT NULL UNIQUE,
    node_name VARCHAR(150) NOT NULL,
    latitude DECIMAL(10,7) NOT NULL,
    longitude DECIMAL(10,7) NOT NULL,
    node_type VARCHAR(50) NOT NULL DEFAULT 'ROAD',
    INDEX idx_node_location (latitude, longitude)
) ENGINE=InnoDB;

CREATE TABLE network_edges (
    edge_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    source_node_id BIGINT UNSIGNED NOT NULL,
    destination_node_id BIGINT UNSIGNED NOT NULL,
    distance_km DECIMAL(10,3) NOT NULL,
    travel_time_minutes DECIMAL(10,2),
    is_blocked BOOLEAN NOT NULL DEFAULT FALSE,
    FOREIGN KEY (source_node_id) REFERENCES network_nodes(node_id),
    FOREIGN KEY (destination_node_id) REFERENCES network_nodes(node_id),
    CHECK (distance_km > 0),
    CHECK (source_node_id <> destination_node_id),
    INDEX idx_edge_source (source_node_id),
    INDEX idx_edge_destination (destination_node_id)
) ENGINE=InnoDB;
