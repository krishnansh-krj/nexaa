import { SEV_HEX } from '../services/format';

/**
 * Map key. Turn on only the rows a page actually draws:
 *   closed    – closed (resolved/archived) incident marker
 *   requests  – open-request badge on incidents
 *   links     – allocation link (incident ↔ deployed unit, engine road distance)
 *   nearest   – KD-tree nearest-unit link
 *   candidates– ranked candidate link
 *   route     – Dijkstra shortest path
 *   showRoads – blocked road
 */
export default function MapLegend({ showRoads = false, closed = false, requests = false, links = false, nearest = false, candidates = false, route = false, regionHint = true }) {
  return (
    <div className="map-overlay map-legend">
      <div><span className="legend-dot legend-pulse" style={{ background: SEV_HEX[5], color: SEV_HEX[5] }} />Active incident (colour = severity)</div>
      {closed && <div><span className="legend-dot legend-closed">✓</span>Closed incident</div>}
      {requests && <div><span className="legend-badge">2</span>Open emergency requests</div>}
      <div><span className="legend-dot" style={{ background: '#2de2e6', borderRadius: 2, transform: 'rotate(45deg)' }} />Resource (grey = busy)</div>
      <div><span className="legend-dot" style={{ background: '#fff', borderRadius: 3 }} />Hospital</div>
      {links && <div><span className="legend-line" style={{ background: '#3ddc97' }} />Allocated unit · road km</div>}
      {candidates && <div><span className="legend-line dashed" style={{ color: '#ffb627' }} />Ranked candidate · road km</div>}
      {nearest && <div><span className="legend-line dashed" style={{ color: '#2de2e6' }} />Nearest unit (KD-tree)</div>}
      {route && <div><span className="legend-line thick" style={{ background: '#ff3d57' }} />Dijkstra shortest path</div>}
      {showRoads && <div><span className="legend-dot" style={{ background: '#ff3d57', borderRadius: 1, height: 3 }} />Blocked road</div>}
      {regionHint && <div className="faint" style={{ fontSize: 10 }}>Shift + drag: region query</div>}
    </div>
  );
}
