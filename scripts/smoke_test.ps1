<#
    NEXA backend smoke test (READ-ONLY).
    Calls every GET endpoint, checks HTTP status codes, the CORS preflight,
    and the self-checks reported by the data-structure endpoints.
    It never sends POST / PATCH requests and never changes the database.

    Usage (server must be running):
        powershell -ExecutionPolicy Bypass -File scripts\smoke_test.ps1
        powershell -ExecutionPolicy Bypass -File scripts\smoke_test.ps1 -BaseUrl http://127.0.0.1:8080
#>
param(
    [string]$BaseUrl = "http://127.0.0.1:8080",
    [int]$IncidentId = 1,
    [int]$RequestId = 4
)

$ErrorActionPreference = "Stop"
$script:passed = 0
$script:failed = 0

function Invoke-Nexa([string]$Method, [string]$Path) {
    $uri = "$BaseUrl$Path"
    try {
        $r = Invoke-WebRequest -Uri $uri -Method $Method -UseBasicParsing -TimeoutSec 15
        return @{ Status = [int]$r.StatusCode; Body = $r.Content; Headers = $r.Headers }
    } catch [System.Net.WebException] {
        $resp = $_.Exception.Response
        if ($null -eq $resp) { return @{ Status = 0; Body = $_.Exception.Message; Headers = @{} } }
        $reader = New-Object System.IO.StreamReader($resp.GetResponseStream())
        return @{ Status = [int]$resp.StatusCode; Body = $reader.ReadToEnd(); Headers = $resp.Headers }
    }
}

function Check([string]$Path, [int[]]$Expected, [scriptblock]$Assert = $null, [string]$Method = "GET") {
    $r = Invoke-Nexa $Method $Path
    $ok = $Expected -contains $r.Status
    $note = ""
    if ($ok -and $Assert -and $r.Status -lt 300) {
        try {
            $json = $r.Body | ConvertFrom-Json
            $ok = [bool](& $Assert $json)
            if (-not $ok) { $note = " (self-check failed)" }
        } catch { $ok = $false; $note = " (invalid JSON)" }
    }
    if ($ok) { $script:passed++; Write-Host ("  PASS {0,-4} {1} -> {2}" -f $Method, $Path, $r.Status) -ForegroundColor Green }
    else {
        $script:failed++
        $snippet = if ($r.Body) { $r.Body.Substring(0, [Math]::Min(160, $r.Body.Length)) } else { "" }
        Write-Host ("  FAIL {0,-4} {1} -> {2}, expected {3}{4}  {5}" -f $Method, $Path, $r.Status, ($Expected -join "/"), $note, $snippet) -ForegroundColor Red
    }
}

Write-Host "NEXA smoke test against $BaseUrl" -ForegroundColor Cyan

Write-Host "`n[Health]"
Check "/api/health" 200
Check "/api/db/health" 200 { param($j) $j.status -eq "connected" }

Write-Host "`n[CORS]"
$pre = Invoke-Nexa "OPTIONS" "/api/incidents"
if ($pre.Status -eq 204 -and $pre.Headers["Access-Control-Allow-Origin"]) {
    $script:passed++; Write-Host "  PASS OPTIONS /api/incidents -> 204 (allow-origin: $($pre.Headers['Access-Control-Allow-Origin']))" -ForegroundColor Green
} else {
    $script:failed++; Write-Host "  FAIL OPTIONS /api/incidents -> $($pre.Status)" -ForegroundColor Red
}

Write-Host "`n[Incidents / resources / requests]"
Check "/api/incidents" 200 { param($j) $j.success -and $j.data.Count -ge 1 }
Check "/api/incidents/$IncidentId" 200 { param($j) $j.data.PSObject.Properties.Name -contains "latitude" }
Check "/api/incidents/999999" 404
Check "/api/incidents/$IncidentId/events" 200
Check "/api/resources" 200
Check "/api/resources/available?type=AMBULANCE" 200
Check "/api/resources/999999" 404
Check "/api/requests" 200
Check "/api/requests/prioritized" 200
Check "/api/requests/matching-resources?request_id=$RequestId" 200,400
Check "/api/requests/matching-resources?request_id=999999" 404
Check "/api/requests/incident/$IncidentId" 200

Write-Host "`n[Allocations]"
Check "/api/allocations" 200
Check "/api/allocations/incident/$IncidentId" 200

Write-Host "`n[Network: graph, BFS, DFS, Dijkstra]"
Check "/api/network/nodes" 200
Check "/api/network/edges" 200
Check "/api/network/route?source=1&destination=4" 200
Check "/api/network/traverse/bfs?start=1" 200
Check "/api/network/traverse/dfs?start=1" 200

Write-Host "`n[Synopsis data structures: /api/dsa]"
Check "/api/dsa/catalogue" 200 { param($j) $j.data.structure_count -eq 28 }
Check "/api/dsa/spatial/nearest?incident_id=$IncidentId&k=3" 200
Check "/api/dsa/spatial/nearest?lat=28.6139&lng=77.2090&type=HOSPITAL&only_available=false" 200
Check "/api/dsa/spatial/nearest" 400
Check "/api/dsa/spatial/region?min_lat=28.5&max_lat=28.7&min_lng=77.1&max_lng=77.3" 200 { param($j) $j.data.cross_check_passed }
Check "/api/dsa/search/prefix?q=nexa" 200 { param($j) $j.data.result_count -ge 1 }
Check "/api/dsa/search/pattern?q=fire" 200
Check "/api/dsa/resources/rank?request_id=$RequestId" 200,404,422
Check "/api/dsa/resources/rank?request_id=$RequestId&algorithm=quick" 200,404,422
Check "/api/dsa/resources/rank?request_id=999999" 404
Check "/api/dsa/incidents/index?code=NEXA-INC-001" 200 { param($j) $j.data.code_lookup.methods_agree -and $j.data.tree_comparison.avl_valid -and $j.data.tree_comparison.red_black_valid }
Check "/api/dsa/incidents/timeline?incident_id=$IncidentId" 200
Check "/api/dsa/incidents/timeline?incident_id=999999" 404
Check "/api/dsa/incidents/classification" 200
Check "/api/dsa/operations/history?limit=5" 200
Check "/api/dsa/requests/queue" 200
Check "/api/dsa/allocations/matrix" 200
Check "/api/dsa/allocations/matrix?include_closed=true" 200
Check "/api/dsa/analytics/availability" 200
Check "/api/dsa/network/mst" 200 { param($j) $j.data.kruskal_prim_agree }

Write-Host ""
$total = $script:passed + $script:failed
if ($script:failed -eq 0) {
    Write-Host "SMOKE TEST PASSED: $($script:passed)/$total" -ForegroundColor Green
    exit 0
}
Write-Host "SMOKE TEST FAILED: $($script:failed) of $total checks failed" -ForegroundColor Red
exit 1
