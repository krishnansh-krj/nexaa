# NEXA Backend — API Reference (for the frontend)

Base URL: `http://127.0.0.1:8080` · All bodies are JSON · CORS origin from `NEXA_CORS_ORIGIN` (default `http://localhost:5173`).

Every response has the same envelope:

```json
{ "success": true,  "message": "...", "data": { } }
{ "success": false, "message": "...", "error": { "code": "INCIDENT_NOT_FOUND" } }
```

## Core operations

| Method | Path | Notes |
|---|---|---|
| GET | `/api/health`, `/api/db/health` | Liveness and DB connectivity |
| GET | `/api/incidents` · `/api/incidents/{id}` | Now include `latitude`, `longitude`, `address`, `reported_at`, `resolved_at` |
| **POST** | `/api/incidents` | **New.** `{title, incident_type, severity, [description], [status: REPORTED\|VERIFIED\|ACTIVE], [incident_code], [latitude, longitude], [address]}` → `201 {incident_id, incident_code}`. Code auto-generated as `NEXA-INC-###` when omitted; an `INCIDENT_REPORTED` event is written in the same transaction. |
| **PATCH** | `/api/incidents/{id}/status` | **New.** `{status, [note]}`. Lifecycle: REPORTED→VERIFIED/ACTIVE/CLOSED, VERIFIED→ACTIVE/RESOLVED/CLOSED, ACTIVE→RESOLVED, RESOLVED→CLOSED/ACTIVE. `409` if the transition is not allowed or active allocations remain when resolving/closing. Writes a `STATUS_CHANGED` event. |
| GET/POST | `/api/incidents/{id}/events` | Event log |
| GET | `/api/resources` · `/api/resources/{id}` · `/api/resources/available?type=AMBULANCE` | |
| POST | `/api/resources` | `{resource_code, resource_name, resource_type, total_units, [latitude, longitude], [address]}` — location is **new** (needed for map + KD tree) |
| GET | `/api/requests` · `/api/requests/{id}` · `/api/requests/incident/{id}` | |
| POST | `/api/requests` | Create emergency request |
| GET | `/api/requests/prioritized` | Binary-heap priority order |
| GET | `/api/requests/matching-resources?request_id=` | Compatible available resources; `404` if the request does not exist |
| GET/POST | `/api/allocations` · `/api/allocations/{id}` · `/incident/{id}` · `/resource/{id}` | Allocations now include `assigned_at`, `released_at` |
| POST | `/api/allocations/{id}/release` | Returns units to inventory |
| GET | `/api/network/nodes` · `/nodes/{id}` · `/edges` · `/nodes/{id}/edges` | Road graph |
| PATCH | `/api/network/edges/{id}/block` | Block / unblock a road |
| GET | `/api/network/route?source=&destination=` | Dijkstra (min-heap) |
| GET | `/api/network/traverse/bfs?start=` · `/dfs?start=` | BFS / DFS |

## Data-structure intelligence (`/api/dsa`, read-only)

Each response carries `structures_used` so the UI can label which structure produced it.

| Path | Structures | Parameters |
|---|---|---|
| `/api/dsa/catalogue` | all 28 | — (no DB access) |
| `/api/dsa/spatial/nearest` | KD Tree, Hash Table, Linear Search | `incident_id` **or** `lat`+`lng`; `[k=5]`, `[type]`, `[only_available=true]` |
| `/api/dsa/spatial/region` | Quad Tree, Range Tree (cross-checked) | `min_lat`, `max_lat`, `min_lng`, `max_lng` |
| `/api/dsa/search/prefix` | Trie | `q`, `[limit=20]` — search-as-you-type over codes, titles, names, types |
| `/api/dsa/search/pattern` | Suffix Array + LCP | `q` (≥ 2 chars), `[limit=50]` — substring search in descriptions |
| `/api/dsa/resources/rank` | Hash Table, Array, Linear Search, Bucket Sort, Merge/Quick Sort, Median | `request_id`, `[algorithm=merge\|quick]` — ranked candidates with `suggested_units` |
| `/api/dsa/incidents/index` | Radix + Counting Sort, BST, AVL, Red-Black, chained + probing Hash, Binary + Linear Search | `[from_id]`, `[to_id]`, `[code]` |
| `/api/dsa/incidents/timeline` | Doubly Linked List | `incident_id` |
| `/api/dsa/incidents/classification` | K-ary Tree | — |
| `/api/dsa/operations/history` | Stack | `[limit=20]` — most recent first, with reverse-action hint |
| `/api/dsa/requests/queue` | Queue vs Priority Queue | — |
| `/api/dsa/allocations/matrix` | Multi-Linked List, Sparse Matrix | `[include_closed=false]` |
| `/api/dsa/analytics/availability` | Interval Tree, Segment Tree, Median | `[from]`, `[to]` (epoch s, default last 7 days), `[resource_id]`, `[bucket_seconds=3600]` |
| `/api/dsa/network/mst` | Kruskal + DSU, Prim + Adjacency Matrix | — |

Common error codes: `INVALID_PARAMETER` (400), `INCIDENT_NOT_FOUND` / `REQUEST_NOT_FOUND` (404),
`INVALID_STATUS_TRANSITION` / `ACTIVE_ALLOCATIONS_EXIST` / `DUPLICATE_INCIDENT_CODE` (409),
`INCIDENT_WITHOUT_LOCATION` / `UNSUPPORTED_REQUEST_TYPE` (422), `DATABASE_ERROR` (500).
