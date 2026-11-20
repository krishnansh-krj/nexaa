-- ============================================================
-- NEXA migration 001 - inventory re-synchronisation
-- Applied manually on 2026-11-18 (backup: resources_backup_20261118).
--
-- Cause: the database was seeded before nexa_seed.sql gained its
-- "synchronise inventory" block, so seeded allocations (IDs 1-3)
-- never reduced resources.available_units.
--
-- Safe to re-run: it recomputes available_units only where it
-- disagrees with active allocations and leaves MAINTENANCE /
-- UNAVAILABLE statuses untouched.
-- ============================================================

USE nexa_db;

-- 1. Read-only audit (expect zero rows after the fix).
SELECT r.resource_id, r.total_units, r.available_units,
       COALESCE(a.active_units, 0) AS active_units
FROM resources r
LEFT JOIN (SELECT resource_id, SUM(allocated_units) AS active_units
           FROM resource_allocations
           WHERE status IN ('ASSIGNED', 'DISPATCHED', 'ON_SITE')
           GROUP BY resource_id) a ON a.resource_id = r.resource_id
WHERE r.available_units <> GREATEST(r.total_units - COALESCE(a.active_units, 0), 0);

-- 2. Correction (only rows that disagree).
START TRANSACTION;

UPDATE resources r
LEFT JOIN (SELECT resource_id, SUM(allocated_units) AS active_units
           FROM resource_allocations
           WHERE status IN ('ASSIGNED', 'DISPATCHED', 'ON_SITE')
           GROUP BY resource_id) a ON a.resource_id = r.resource_id
SET r.available_units = GREATEST(r.total_units - COALESCE(a.active_units, 0), 0),
    r.status = CASE
        WHEN r.status IN ('MAINTENANCE', 'UNAVAILABLE') THEN r.status
        WHEN COALESCE(a.active_units, 0) >= r.total_units THEN 'DEPLOYED'
        ELSE 'AVAILABLE' END
WHERE r.available_units <> GREATEST(r.total_units - COALESCE(a.active_units, 0), 0);

COMMIT;
