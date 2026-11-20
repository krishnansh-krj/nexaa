-- ============================================================
-- NEXA migration 002 - inventory integrity constraint
-- Prevents available_units from ever exceeding total_units
-- (available_units is already UNSIGNED, so it cannot go below 0).
--
-- Run once, after migration 001. If it reports
-- "Duplicate check constraint name", it is already applied.
-- ============================================================

USE nexa_db;

-- Must return zero rows, otherwise the ALTER below will fail.
SELECT resource_id, total_units, available_units
FROM resources
WHERE available_units > total_units;

ALTER TABLE resources
    ADD CONSTRAINT chk_resource_units CHECK (available_units <= total_units);

-- Verify
SELECT CONSTRAINT_NAME, CHECK_CLAUSE
FROM information_schema.CHECK_CONSTRAINTS
WHERE CONSTRAINT_SCHEMA = 'nexa_db' AND CONSTRAINT_NAME = 'chk_resource_units';
