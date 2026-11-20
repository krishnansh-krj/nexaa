# NEXA database migrations

Run in order against the existing `nexa_db`. Never run `seed/nexa_seed.sql`
against a database that already holds data.

| File | Purpose | Status |
|---|---|---|
| `001_inventory_resync_2026-11-18.sql` | Recompute `available_units` from active allocations | Applied manually 2026-11-18 (re-runnable) |
| `002_resource_units_check.sql` | `CHECK (available_units <= total_units)` | Run once |

```cmd
mysql -u root -p nexa_db < database\migrations\002_resource_units_check.sql
```

No migration is needed for the new `/api/dsa/*` endpoints: they only read.
