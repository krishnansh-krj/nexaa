
USE nexa_db;

-- ==========================================
-- NEXA VERSION 1.0 - SAMPLE DATA
-- ==========================================

-- 1. INCIDENTS
INSERT INTO incidents
(incident_code, title, description, incident_type, severity,
 status, latitude, longitude, address)
VALUES
('NEXA-INC-001',
 'Urban Building Fire',
 'Fire reported in a multi-storey residential building.',
 'FIRE', 'CRITICAL', 'ACTIVE',
 28.6139000, 77.2090000, 'Central Delhi - Simulation Zone A'),

('NEXA-INC-002',
 'Urban Flooding',
 'Heavy rainfall causing waterlogging and evacuation requirements.',
 'FLOOD', 'HIGH', 'VERIFIED',
 28.6304000, 77.2177000, 'Central Delhi - Simulation Zone B');

-- 2. RESOURCES
INSERT INTO resources
(resource_code, resource_name, resource_type, status,
 total_units, available_units, latitude, longitude, address)
VALUES
('NEXA-RES-001', 'Ambulance Unit A', 'AMBULANCE',
 'AVAILABLE', 3, 3, 28.6150000, 77.2100000,
 'Emergency Base A'),

('NEXA-RES-002', 'Fire Response Truck A', 'FIRE_TRUCK',
 'AVAILABLE', 2, 2, 28.6170000, 77.2110000,
 'Fire Station A'),

('NEXA-RES-003', 'Medical Response Team A', 'MEDICAL_TEAM',
 'AVAILABLE', 5, 5, 28.6200000, 77.2140000,
 'Medical Centre A'),

('NEXA-RES-004', 'Rescue Team A', 'RESCUE_TEAM',
 'AVAILABLE', 4, 4, 28.6250000, 77.2160000,
 'Rescue Base A');

-- 3. EMERGENCY REQUESTS
INSERT INTO emergency_requests
(incident_id, request_type, description, priority, quantity_required)
VALUES
(1, 'FIRE_SUPPRESSION',
 'Immediate fire suppression support required.',
 'CRITICAL', 2),

(1, 'MEDICAL_ASSISTANCE',
 'Medical team required for possible casualties.',
 'HIGH', 1),

(2, 'RESCUE_SUPPORT',
 'Rescue personnel required for flood evacuation.',
 'HIGH', 2);

-- 4. RESOURCE ALLOCATIONS
INSERT INTO resource_allocations
(incident_id, resource_id, request_id, allocated_units, status)
VALUES
(1, 2, 1, 1, 'DISPATCHED'),
(1, 3, 2, 1, 'ASSIGNED'),
(2, 4, 3, 2, 'ASSIGNED');

-- ============================================================
-- SYNCHRONIZE RESOURCE INVENTORY WITH SEEDED ACTIVE ALLOCATIONS
-- ============================================================

UPDATE resources r
LEFT JOIN
(
    SELECT
        resource_id,
        SUM(allocated_units) AS active_units
    FROM resource_allocations
    WHERE status IN ('ASSIGNED', 'DISPATCHED', 'ON_SITE')
    GROUP BY resource_id
) a
    ON a.resource_id = r.resource_id
SET
    r.available_units = GREATEST(
        r.total_units - COALESCE(a.active_units, 0),
        0
    ),
    r.status = CASE
        WHEN r.status IN ('MAINTENANCE', 'UNAVAILABLE')
            THEN r.status
        WHEN COALESCE(a.active_units, 0) >= r.total_units
            THEN 'DEPLOYED'
        ELSE 'AVAILABLE'
    END;


-- ============================================================
-- SYNCHRONIZE REQUEST STATUS WITH SEEDED ACTIVE ALLOCATIONS
-- ============================================================

UPDATE emergency_requests er
LEFT JOIN
(
    SELECT
        request_id,
        SUM(allocated_units) AS active_units
    FROM resource_allocations
    WHERE request_id IS NOT NULL
      AND status IN ('ASSIGNED', 'DISPATCHED', 'ON_SITE')
    GROUP BY request_id
) a
    ON a.request_id = er.request_id
SET
    er.status = CASE
        WHEN COALESCE(a.active_units, 0) >= er.quantity_required
            THEN 'FULFILLED'
        WHEN COALESCE(a.active_units, 0) > 0
            THEN 'PROCESSING'
        ELSE 'PENDING'
    END
WHERE er.status <> 'CANCELLED';

-- 5. INCIDENT EVENTS
INSERT INTO incident_events
(incident_id, event_type, description)
VALUES
(1, 'INCIDENT_REPORTED',
 'Building fire reported to the central authority.'),

(1, 'RESOURCE_DISPATCHED',
 'Fire response truck dispatched to incident location.'),

(2, 'INCIDENT_VERIFIED',
 'Flooding incident verified by the response authority.');

-- 6. NETWORK NODES
INSERT INTO network_nodes
(node_code, node_name, latitude, longitude, node_type)
VALUES
('NODE-001', 'Central Junction',
 28.6139000, 77.2090000, 'JUNCTION'),

('NODE-002', 'Fire Station Junction',
 28.6170000, 77.2110000, 'STATION'),

('NODE-003', 'Medical Centre Junction',
 28.6200000, 77.2140000, 'HOSPITAL'),

('NODE-004', 'Flood Response Junction',
 28.6250000, 77.2160000, 'JUNCTION');

-- 7. NETWORK EDGES
INSERT INTO network_edges
(source_node_id, destination_node_id,
 distance_km, travel_time_minutes, is_blocked)
VALUES
(1, 2, 1.5, 5.0, FALSE),
(2, 3, 2.0, 7.0, FALSE),
(3, 4, 2.5, 9.0, FALSE),
(1, 3, 3.0, 10.0, FALSE),
(2, 4, 4.0, 13.0, FALSE);
