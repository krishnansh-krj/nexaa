import { useEffect, useMemo, useState } from 'react';
import { api } from '../services/api';
import { useApi } from '../hooks/useApi';
import { km, mins } from '../services/format';
import NexaMap from './NexaMap';
import MapLegend from './MapLegend';

const C = { route: '#ff3d57', nearestRoute: '#2de2e6', candidate: '#ffb627', nearest: '#2de2e6', deployed: '#3ddc97' };

/** Straight-line coordinates of a Dijkstra result, including the snap legs to/from the road graph. */
function routeCoords(r, from, to) {
  if (!r || !r.found || !r.stops?.length) return null;
  return [[from.lon, from.lat], ...r.stops.map((s) => [s.lon, s.lat]), [to.lon, to.lat]];
}

/**
 * Dispatch view for one incident. Everything drawn here is computed by the C++ engine:
 *   - ranked candidates (KD-tree pre-selection → filters → Dijkstra road distance → weighted score)
 *   - the single nearest available unit (KD-tree k-NN query) — to show that nearest ≠ best
 *   - the Dijkstra shortest path from the focused unit to the incident (binary-heap PQ)
 *   - units already deployed to this incident, with the engine's route distance and ETA
 *
 * props: incident, candidates (ranking API rows), deployments (active deployments, optional),
 *        picked (Set of resource ids), focusId / onFocus(resourceId), className, maxCandidates
 */
export default function DispatchMap({ incident, candidates = [], deployments = [], picked, focusId, onFocus, className = 'short', maxCandidates = 8 }) {
  const incId = incident?.id;
  const shown = useMemo(() => candidates.slice(0, maxCandidates), [candidates, maxCandidates]);
  const best = shown[0] || null;
  const focus = shown.find((c) => c.resourceId === focusId) || best;

  // KD-tree nearest available unit (1-NN)
  const nearestQ = useApi(() => api.nearest({ lat: incident.lat, lon: incident.lon, k: 1, kind: 'resource', availableOnly: 1 }), [incId], { enabled: Boolean(incident) });
  const nearest = nearestQ.data?.items?.[0] || null;

  // resource positions, needed to draw units that are already deployed to this incident
  const layers = useApi(() => api.layers(false), [], { enabled: deployments.length > 0 });
  const resById = useMemo(() => new Map((layers.data?.resources || []).map((r) => [r.id, r])), [layers.data]);

  // Dijkstra: focused candidate → incident, and nearest → incident when it differs
  const [routes, setRoutes] = useState({ focus: null, nearest: null });
  useEffect(() => {
    let alive = true;
    setRoutes({ focus: null, nearest: null });
    if (!incident || !focus) return undefined;
    const q = (from) => api.route({ fromLat: from.lat, fromLon: from.lon, toLat: incident.lat, toLon: incident.lon, mode: 'distance' }).catch(() => null);
    const wantNearest = nearest && nearest.id !== focus.resourceId;
    Promise.all([q(focus), wantNearest ? q(nearest) : Promise.resolve(null)]).then(([f, n]) => { if (alive) setRoutes({ focus: f, nearest: n }); });
    return () => { alive = false; };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [incId, focus?.resourceId, nearest?.id]);

  const myDeployed = useMemo(() => deployments
    .filter((d) => d.incidentId === incId && d.status === 'ACTIVE')
    .flatMap((d) => d.resources.filter((r) => !r.released).map((r) => ({ ...r, deploymentId: d.id }))), [deployments, incId]);

  const resources = useMemo(() => {
    const out = new Map();
    shown.forEach((c) => out.set(c.resourceId, { id: c.resourceId, code: c.code, name: c.name, lat: c.lat, lon: c.lon, status: 'AVAILABLE', category: c.category, available: c.available, capacity: c.capacity, organization: `score ${c.score.toFixed(1)} · ${km(c.roadKm)} ${c.routable ? 'road' : 'est.'}` }));
    if (nearest && !out.has(nearest.id)) out.set(nearest.id, { id: nearest.id, code: nearest.name, name: nearest.name, lat: nearest.lat, lon: nearest.lon, status: nearest.status, category: nearest.category, available: nearest.available, capacity: nearest.capacity, organization: `nearest · ${km(nearest.distanceKm)} straight` });
    myDeployed.forEach((d) => { const r = resById.get(d.resourceId); if (r && !out.has(r.id)) out.set(r.id, { ...r, organization: `deployed · #${d.deploymentId}` }); });
    return [...out.values()];
  }, [shown, nearest, myDeployed, resById]);

  const links = useMemo(() => {
    if (!incident) return [];
    const L = [];
    shown.forEach((c, i) => {
      if (focus && c.resourceId === focus.resourceId) return; // focused one gets the full Dijkstra route
      L.push({ a: c, b: incident, color: C.candidate, dashed: true, width: 1.4, label: i < 5 ? `#${i + 1} · ${km(c.roadKm)}` : undefined });
    });
    if (nearest && (!focus || nearest.id !== focus.resourceId) && !routes.nearest?.found) {
      L.push({ a: nearest, b: incident, color: C.nearest, dashed: true, width: 2, label: `nearest · ${km(nearest.distanceKm)}` });
    }
    myDeployed.forEach((d) => {
      const r = resById.get(d.resourceId);
      if (r) L.push({ a: r, b: incident, color: C.deployed, width: 2.6, label: `${d.resourceCode} · ${km(d.routeKm)} · ${mins(d.etaMinutes)}` });
    });
    return L;
  }, [incident, shown, focus, nearest, myDeployed, resById, routes.nearest]);

  const routeLines = useMemo(() => {
    if (!incident) return [];
    const out = [];
    const f = routeCoords(routes.focus, focus, incident);
    if (f) out.push({ coords: f, color: C.route, width: 4.5, label: `Dijkstra · ${focus.code} · ${km(routes.focus.distanceKm)} · ${mins(routes.focus.minutes)}` });
    const n = nearest && routeCoords(routes.nearest, nearest, incident);
    if (n) out.push({ coords: n, color: C.nearestRoute, width: 3, label: `nearest · ${km(routes.nearest.distanceKm)} · ${mins(routes.nearest.minutes)}` });
    return out;
  }, [incident, routes, focus, nearest]);

  const tags = useMemo(() => {
    const t = {};
    if (best) t[best.resourceId] = { text: `BEST ${best.score.toFixed(1)}`, color: '#ffb627' };
    if (focus && best && focus.resourceId !== best.resourceId) t[focus.resourceId] = { text: 'ROUTED', color: C.route };
    if (nearest) t[nearest.id] = t[nearest.id] ? { text: `${t[nearest.id].text} = NEAREST`, color: '#ffb627' } : { text: 'NEAREST (KD)', color: C.nearest };
    return { resources: t };
  }, [best, focus, nearest]);

  const highlight = useMemo(() => ({ resources: new Set([...(picked ? [...picked] : []), ...(focus ? [focus.resourceId] : [])]) }), [picked, focus]);
  const fitTo = useMemo(() => (incident ? [incident, ...shown.slice(0, 5), ...(nearest ? [nearest] : [])] : null),
    // refit only when the incident or the candidate set changes, not on every focus click
    // eslint-disable-next-line react-hooks/exhaustive-deps
    [incId, shown.map((c) => c.resourceId).join(','), nearest?.id]);
  const incidents = useMemo(() => (incident ? [incident] : []), [incident]);

  const fr = routes.focus;
  return (
    <NexaMap className={className} incidents={incidents} resources={resources} links={links} routes={routeLines} tags={tags}
      highlight={highlight} fitTo={fitTo} layerControl
      onSelect={(kind, item) => { if (kind === 'resource' && onFocus && shown.some((c) => c.resourceId === item.id)) onFocus(item.id); }}>
      <div className="map-overlay dispatch-hud">
        {fr?.found && <span className="hud-chip" style={{ color: C.route }}>Dijkstra: {km(fr.distanceKm)} · {mins(fr.minutes)} · {fr.settled} settled · {fr.relaxations} relaxations · {fr.microseconds?.toFixed(0)} µs</span>}
        {fr && !fr.found && <span className="hud-chip" style={{ color: C.route }}>No road route — {fr.note || 'unreachable'}</span>}
        {nearest && <span className="hud-chip" style={{ color: C.nearest }}>KD-tree nearest: {nearest.name} · {km(nearest.distanceKm)}{nearestQ.data?.stats ? ` · ${nearestQ.data.stats.candidatesExamined} examined` : ''}</span>}
        {myDeployed.length > 0 && <span className="hud-chip" style={{ color: C.deployed }}>{myDeployed.length} unit(s) deployed</span>}
      </div>
      <MapLegend links={myDeployed.length > 0} candidates nearest route regionHint={false} />
    </NexaMap>
  );
}
