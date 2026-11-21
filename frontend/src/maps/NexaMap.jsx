import { useEffect, useRef, useState } from 'react';
import maplibregl from 'maplibre-gl';
import 'maplibre-gl/dist/maplibre-gl.css';
import { DELHI, circlePolygon, darkStyle, fc, rectPolygon } from './mapStyle';
import { CATEGORY_COLORS, SEV_HEX } from '../services/format';

const esc = (s) => String(s ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));

export const CLOSED_STATUSES = new Set(['RESOLVED', 'ARCHIVED']);
export const isClosedIncident = (i) => CLOSED_STATUSES.has(i.status);

const LAYER_DEFS = [
  ['active', 'Active incidents'],
  ['closed', 'Closed incidents'],
  ['resources', 'Resources'],
  ['hospitals', 'Hospitals'],
  ['links', 'Allocation links'],
  ['routes', 'Dijkstra routes'],
];
const ALL_VISIBLE = { active: true, closed: true, resources: true, hospitals: true, links: true, routes: true };

const countOf = (counts, id) => {
  if (!counts) return 0;
  if (counts instanceof Map) return counts.get(id) || 0;
  return counts[id] || 0;
};

/**
 * Interactive operational map.
 *  incidents/resources/hospitals : entity arrays (lat, lon). Incidents with status RESOLVED/ARCHIVED
 *                 are drawn with a separate "closed" marker; open ones pulse in their severity colour.
 *  requestCounts : { incidentId: n } (or Map) open emergency requests → badge on the incident marker
 *  tags         : { incidents: { id: {text, color} }, resources: { id: {text, color} } } small labels above markers
 *  network      : { locations, roads } to draw the road graph
 *  routes       : [{ coords: [[lon,lat]...], color, width, label }]  (Dijkstra paths — animated)
 *  links        : [{ a:{lat,lon}, b:{lat,lon}, color, dashed, width, label }]  (allocation / nearest links,
 *                 label is drawn at the midpoint, e.g. "4.2 km · 9 min")
 *  labels       : [{ lat, lon, text, color }] free text labels
 *  edges        : [{ a:{lat,lon}, b:{lat,lon}, color }] (MST, traversal trees)
 *  circle       : { lat, lon, km }   rect : { minLat, minLon, maxLat, maxLon }
 *  cells        : quad-tree density cells   clusters : DBSCAN clusters
 *  highlight    : { incidents:Set, resources:Set, hospitals:Set }
 *  layerControl : show the layer on/off panel (top-left)
 *  onMapClick(lat, lon)   onSelect(kind, item)   onBoxSelect(rect) (shift + drag)
 */
export default function NexaMap({
  incidents = [], resources = [], hospitals = [], network, routes = [], links = [], labels = [], edges = [], circle, rect, cells, clusters,
  highlight, dimOthers = false, requestCounts, tags, layerControl = false,
  onMapClick, onSelect, onBoxSelect, fitTo, className = '', children, center = DELHI, zoom = 10,
}) {
  const box = useRef(null);
  const map = useRef(null);
  const ready = useRef(false);
  const markers = useRef([]);
  const labelMarkers = useRef([]);
  const handlers = useRef({});
  handlers.current = { onMapClick, onSelect, onBoxSelect };
  const [vis, setVis] = useState(ALL_VISIBLE);
  const [panelOpen, setPanelOpen] = useState(false);

  // ---------------------------------------------------------------- init once
  useEffect(() => {
    const m = new maplibregl.Map({ container: box.current, style: darkStyle, center: [center.lon, center.lat], zoom, attributionControl: { compact: true }, boxZoom: false });
    map.current = m;
    m.addControl(new maplibregl.NavigationControl({ showCompass: false }), 'top-right');
    m.addControl(new maplibregl.ScaleControl({ unit: 'metric' }), 'bottom-right');
    m.on('load', () => {
      const add = (id) => m.addSource(id, { type: 'geojson', data: fc([]) });
      ['roads', 'routes', 'links', 'edges', 'circle', 'rect', 'cells', 'clusters'].forEach(add);
      m.addLayer({ id: 'cells', type: 'fill', source: 'cells', paint: { 'fill-color': '#ffb627', 'fill-opacity': ['interpolate', ['linear'], ['get', 'count'], 1, 0.04, 6, 0.2] } });
      m.addLayer({ id: 'cells-line', type: 'line', source: 'cells', paint: { 'line-color': '#ffb627', 'line-opacity': 0.25, 'line-width': 0.6 } });
      m.addLayer({ id: 'roads', type: 'line', source: 'roads', paint: {
        'line-color': ['case', ['get', 'blocked'], '#ff3d57', '#2de2e6'], 'line-width': ['case', ['get', 'blocked'], 2.2, 1.2],
        'line-opacity': ['case', ['get', 'blocked'], 0.9, 0.32] } });
      m.addLayer({ id: 'clusters', type: 'fill', source: 'clusters', paint: { 'fill-color': '#ff3d57', 'fill-opacity': 0.12 } });
      m.addLayer({ id: 'clusters-line', type: 'line', source: 'clusters', paint: { 'line-color': '#ff3d57', 'line-width': 1.5, 'line-dasharray': [3, 2] } });
      m.addLayer({ id: 'circle', type: 'fill', source: 'circle', paint: { 'fill-color': '#2de2e6', 'fill-opacity': 0.08 } });
      m.addLayer({ id: 'circle-line', type: 'line', source: 'circle', paint: { 'line-color': '#2de2e6', 'line-width': 1.5, 'line-dasharray': [2, 2] } });
      m.addLayer({ id: 'rect', type: 'fill', source: 'rect', paint: { 'fill-color': '#9b8cff', 'fill-opacity': 0.1 } });
      m.addLayer({ id: 'rect-line', type: 'line', source: 'rect', paint: { 'line-color': '#9b8cff', 'line-width': 1.6 } });
      m.addLayer({ id: 'edges', type: 'line', source: 'edges', paint: { 'line-color': ['get', 'color'], 'line-width': 2.4, 'line-opacity': 0.9 } });
      // allocation / nearest-neighbour links (straight lines incident ↔ unit). line-dasharray cannot be
      // data-driven, so solid and dashed links are two layers over the same source.
      m.addLayer({ id: 'links', type: 'line', source: 'links', filter: ['!', ['get', 'dashed']], layout: { 'line-cap': 'round' }, paint: { 'line-color': ['get', 'color'], 'line-width': ['get', 'width'], 'line-opacity': 0.85 } });
      m.addLayer({ id: 'links-dash', type: 'line', source: 'links', filter: ['get', 'dashed'], paint: { 'line-color': ['get', 'color'], 'line-width': ['get', 'width'], 'line-opacity': 0.75, 'line-dasharray': [2, 2] } });
      m.addLayer({ id: 'routes-glow', type: 'line', source: 'routes', layout: { 'line-cap': 'round', 'line-join': 'round' }, paint: { 'line-color': ['get', 'color'], 'line-width': ['+', ['get', 'width'], 8], 'line-opacity': 0.16, 'line-blur': 4 } });
      m.addLayer({ id: 'routes', type: 'line', source: 'routes', layout: { 'line-cap': 'round', 'line-join': 'round' }, paint: { 'line-color': ['get', 'color'], 'line-width': ['get', 'width'], 'line-dasharray': [0, 4, 3] } });
      ready.current = true;
      m.fire('nexa:ready');
    });
    m.on('click', (e) => { if (handlers.current.onMapClick) handlers.current.onMapClick(e.lngLat.lat, e.lngLat.lng); });

    // animated "marching ants" along routes
    const dashSeq = [[0, 4, 3], [0.5, 4, 2.5], [1, 4, 2], [1.5, 4, 1.5], [2, 4, 1], [2.5, 4, 0.5], [3, 4, 0], [0, 0.5, 3, 3.5], [0, 1, 3, 3], [0, 1.5, 3, 2.5], [0, 2, 3, 2], [0, 2.5, 3, 1.5], [0, 3, 3, 1], [0, 3.5, 3, 0.5]];
    let step = 0;
    const anim = setInterval(() => {
      if (!ready.current || !m.getLayer('routes')) return;
      step = (step + 1) % dashSeq.length;
      m.setPaintProperty('routes', 'line-dasharray', dashSeq[step]);
    }, 70);

    // shift + drag = rectangle selection (feeds the quad/range-tree region query)
    let start = null; let boxEl = null;
    const canvas = m.getCanvasContainer();
    const pos = (e) => { const r = canvas.getBoundingClientRect(); return new maplibregl.Point(e.clientX - r.left - canvas.clientLeft, e.clientY - r.top - canvas.clientTop); };
    const onDown = (e) => {
      if (!(e.shiftKey && e.button === 0) || !handlers.current.onBoxSelect) return;
      m.dragPan.disable(); start = pos(e);
      document.addEventListener('mousemove', onMove); document.addEventListener('mouseup', onUp);
      e.preventDefault();
    };
    const onMove = (e) => {
      const cur = pos(e);
      if (!boxEl) { boxEl = document.createElement('div'); boxEl.style.cssText = 'position:absolute;border:1.5px dashed #9b8cff;background:rgba(155,140,255,.1);pointer-events:none;z-index:4'; canvas.appendChild(boxEl); }
      const minX = Math.min(start.x, cur.x), minY = Math.min(start.y, cur.y);
      boxEl.style.left = `${minX}px`; boxEl.style.top = `${minY}px`;
      boxEl.style.width = `${Math.abs(cur.x - start.x)}px`; boxEl.style.height = `${Math.abs(cur.y - start.y)}px`;
    };
    const onUp = (e) => {
      document.removeEventListener('mousemove', onMove); document.removeEventListener('mouseup', onUp);
      m.dragPan.enable();
      if (boxEl) { boxEl.remove(); boxEl = null; }
      const end = pos(e);
      if (Math.abs(end.x - start.x) < 6 || Math.abs(end.y - start.y) < 6) return;
      const a = m.unproject(start), b = m.unproject(end);
      handlers.current.onBoxSelect({ minLat: Math.min(a.lat, b.lat), maxLat: Math.max(a.lat, b.lat), minLon: Math.min(a.lng, b.lng), maxLon: Math.max(a.lng, b.lng) });
    };
    canvas.addEventListener('mousedown', onDown, true);

    return () => {
      clearInterval(anim); canvas.removeEventListener('mousedown', onDown, true);
      markers.current.forEach((mk) => mk.remove()); labelMarkers.current.forEach((mk) => mk.remove());
      m.remove(); ready.current = false;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // helper: run when style is ready
  const whenReady = (fn) => {
    const m = map.current;
    if (!m) return;
    if (ready.current) fn(m); else m.once('nexa:ready', () => fn(m));
  };
  const setData = (m, id, data) => { const s = m.getSource(id); if (s) s.setData(data); };

  // ---------------------------------------------------------------- markers
  useEffect(() => {
    const m = map.current;
    if (!m) return;
    markers.current.forEach((mk) => mk.remove());
    markers.current = [];
    const hl = highlight || {};
    const tg = tags || {};

    // MapLibre positions a marker by writing `transform` on its root element, so the root is a plain
    // wrapper and all visuals live on the inner `.pin` (whose own transform/position can't clash).
    const mk = (kind, item, pinClass, html, { color, badge, tag, zIndex } = {}) => {
      const root = document.createElement('div');
      root.className = 'nx-marker';
      if (zIndex) root.style.zIndex = String(zIndex);
      const pin = document.createElement('div');
      pin.className = pinClass;
      if (color) { pin.style.background = color; pin.style.color = color; }
      root.appendChild(pin);
      if (badge) {
        const b = document.createElement('span');
        b.className = 'pin-badge'; b.textContent = String(badge); b.title = `${badge} open emergency request(s)`;
        root.appendChild(b);
      }
      if (tag) {
        const t = document.createElement('span');
        t.className = 'pin-tag'; t.textContent = tag.text;
        if (tag.color) { t.style.color = tag.color; t.style.borderColor = tag.color; }
        root.appendChild(t);
      }
      root.addEventListener('click', (e) => { e.stopPropagation(); if (handlers.current.onSelect) handlers.current.onSelect(kind, item); });
      const popup = new maplibregl.Popup({ offset: 14, closeButton: true, maxWidth: '300px' }).setHTML(html);
      const marker = new maplibregl.Marker({ element: root }).setLngLat([item.lon, item.lat]).setPopup(popup).addTo(m);
      markers.current.push(marker);
    };
    const isHl = (set, id) => set && set.has && set.has(id);
    const anyHl = dimOthers && (hl.incidents?.size || hl.resources?.size || hl.hospitals?.size);
    const state = (set, id) => `${isHl(set, id) ? 'hl' : ''} ${anyHl && !isHl(set, id) ? 'dim' : ''}`;

    if (vis.hospitals) hospitals.forEach((h) => {
      mk('hospital', h, `pin hospital ${state(hl.hospitals, h.id)}`,
        `<div style="font-weight:700">${esc(h.name)}</div><div style="color:#93a3b8;font-size:12px;margin:4px 0">${esc(h.category)} · ${h.availableBeds}/${h.totalBeds} beds · ICU ${h.icuAvailable}/${h.icuBeds}</div><div style="font-size:11px;color:#2de2e6">${esc((h.capabilities || []).join(', '))}</div>`,
        { zIndex: 1 });
    });
    if (vis.resources) resources.forEach((r) => {
      const color = r.status === 'AVAILABLE' ? (CATEGORY_COLORS[r.category] || '#2de2e6') : '#5d6b7f';
      const tag = tg.resources?.[r.id];
      mk('resource', r, `pin resource ${state(hl.resources, r.id)}`,
        `<div style="font-weight:700">${esc(r.code)}</div><div style="font-size:12px">${esc(r.name)}</div><div style="color:#93a3b8;font-size:12px;margin-top:4px">${esc(r.status)} · ${r.available}/${r.capacity} units</div><div style="font-size:11px;color:#ffb627">${esc(r.organization || '')}</div>${tag ? `<div style="font-size:11px;margin-top:4px;color:${esc(tag.color || '#2de2e6')}">${esc(tag.text)}</div>` : ''}`,
        { color, tag, zIndex: tag || isHl(hl.resources, r.id) ? 5 : 2 });
    });
    incidents.forEach((i) => {
      const closed = isClosedIncident(i);
      if (closed ? !vis.closed : !vis.active) return;
      const tag = tg.incidents?.[i.id];
      const reqs = countOf(requestCounts, i.id);
      const popup = `<div style="font-size:11px;color:#ffb627;font-family:JetBrains Mono,monospace">${esc(i.code)}</div><div style="font-weight:700;margin:2px 0">${esc(i.title)}</div>`
        + `<div style="color:#93a3b8;font-size:12px">${esc(i.categoryPath || i.categoryName)} · severity ${i.severity} · <b style="color:${closed ? '#8a97a8' : '#3ddc97'}">${esc(i.status)}</b></div>`
        + (closed ? '<div style="font-size:12px;margin-top:4px;color:#8a97a8">Closed incident — units released</div>'
          : `<div style="font-size:12px;margin-top:4px">Priority <b style="color:#ff3d57">${Number(i.priorityScore || 0).toFixed(1)}</b> · ${i.assignedUnits}/${i.requiredUnits} units</div>`)
        + (reqs ? `<div style="font-size:12px;margin-top:4px;color:#2de2e6">${reqs} open emergency request(s)</div>` : '')
        + (i.simulated ? '<div style="font-size:10px;color:#9b8cff;margin-top:4px">SIMULATED DATA</div>' : '');
      if (closed) {
        mk('incident', i, `pin incident-closed ${state(hl.incidents, i.id)}`, popup, { tag, zIndex: 1 });
      } else {
        const pulse = i.status === 'CONTAINED' ? 'contained' : 'incident';
        mk('incident', i, `pin ${pulse} ${state(hl.incidents, i.id)}`, popup,
          { color: SEV_HEX[i.severity] || '#ffb627', badge: reqs || null, tag, zIndex: isHl(hl.incidents, i.id) ? 6 : 4 });
      }
    });
  }, [incidents, resources, hospitals, highlight, dimOthers, requestCounts, tags, vis]);

  // ---------------------------------------------------------------- text labels (distances, route stats)
  useEffect(() => {
    const m = map.current;
    if (!m) return;
    labelMarkers.current.forEach((mk) => mk.remove());
    labelMarkers.current = [];
    const all = [];
    if (vis.links) links.forEach((l) => { if (l.label) all.push({ lat: (l.a.lat + l.b.lat) / 2, lon: (l.a.lon + l.b.lon) / 2, text: l.label, color: l.color }); });
    if (vis.routes) routes.forEach((r) => {
      if (!r.label || !r.coords || r.coords.length < 2) return;
      const mid = r.coords[Math.floor((r.coords.length - 1) / 2)];
      const nxt = r.coords[Math.min(r.coords.length - 1, Math.floor((r.coords.length - 1) / 2) + 1)];
      all.push({ lon: (mid[0] + nxt[0]) / 2, lat: (mid[1] + nxt[1]) / 2, text: r.label, color: r.color, strong: true });
    });
    labels.forEach((l) => all.push(l));
    all.forEach((l) => {
      const el = document.createElement('div');
      el.className = `map-label ${l.strong ? 'strong' : ''}`;
      el.textContent = l.text;
      if (l.color) el.style.color = l.color;
      el.style.pointerEvents = 'none';
      labelMarkers.current.push(new maplibregl.Marker({ element: el }).setLngLat([l.lon, l.lat]).addTo(m));
    });
  }, [links, routes, labels, vis]);

  // ---------------------------------------------------------------- geometry layers
  useEffect(() => {
    whenReady((m) => {
      if (!network) { setData(m, 'roads', fc([])); return; }
      const byId = new Map(network.locations.map((l) => [l.id, l]));
      setData(m, 'roads', fc(network.roads.map((r) => {
        const a = byId.get(r.fromId), b = byId.get(r.toId);
        if (!a || !b) return null;
        return { type: 'Feature', properties: { blocked: r.blocked, id: r.id, name: r.name }, geometry: { type: 'LineString', coordinates: [[a.lon, a.lat], [b.lon, b.lat]] } };
      }).filter(Boolean)));
    });
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [network]);

  useEffect(() => {
    whenReady((m) => setData(m, 'routes', fc(routes.filter((r) => r.coords && r.coords.length > 1).map((r) => ({
      type: 'Feature', properties: { color: r.color || '#ffb627', width: r.width || 4 }, geometry: { type: 'LineString', coordinates: r.coords },
    })))));
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [routes]);

  useEffect(() => {
    whenReady((m) => setData(m, 'links', fc(links.filter((l) => l.a && l.b).map((l) => ({
      type: 'Feature', properties: { color: l.color || '#3ddc97', dashed: Boolean(l.dashed), width: l.width || 2 },
      geometry: { type: 'LineString', coordinates: [[l.a.lon, l.a.lat], [l.b.lon, l.b.lat]] },
    })))));
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [links]);

  useEffect(() => {
    whenReady((m) => setData(m, 'edges', fc(edges.map((e) => ({
      type: 'Feature', properties: { color: e.color || '#3ddc97' }, geometry: { type: 'LineString', coordinates: [[e.a.lon, e.a.lat], [e.b.lon, e.b.lat]] },
    })))));
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [edges]);

  // layer on/off
  useEffect(() => {
    whenReady((m) => {
      const set = (id, on) => { if (m.getLayer(id)) m.setLayoutProperty(id, 'visibility', on ? 'visible' : 'none'); };
      set('links', vis.links); set('links-dash', vis.links);
      set('routes', vis.routes); set('routes-glow', vis.routes);
    });
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [vis]);

  useEffect(() => { whenReady((m) => setData(m, 'circle', circle ? fc([circlePolygon(circle.lat, circle.lon, circle.km)]) : fc([]))); /* eslint-disable-next-line */ }, [circle]);
  useEffect(() => { whenReady((m) => setData(m, 'rect', rect ? fc([rectPolygon(rect)]) : fc([]))); /* eslint-disable-next-line */ }, [rect]);
  useEffect(() => { whenReady((m) => setData(m, 'cells', fc((cells || []).map((c) => ({ ...rectPolygon(c), properties: { count: c.count } }))))); /* eslint-disable-next-line */ }, [cells]);
  useEffect(() => { whenReady((m) => setData(m, 'clusters', fc((clusters || []).map((c) => circlePolygon(c.centerLat, c.centerLon, Math.max(0.4, c.radiusKm + 0.3)))))); /* eslint-disable-next-line */ }, [clusters]);

  // ---------------------------------------------------------------- fit bounds
  useEffect(() => {
    if (!fitTo || !fitTo.length) return;
    whenReady((m) => {
      const b = new maplibregl.LngLatBounds();
      fitTo.forEach((p) => b.extend([p.lon, p.lat]));
      m.fitBounds(b, { padding: 70, maxZoom: 14, duration: 900 });
    });
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [fitTo]);

  const counts = {
    active: incidents.filter((i) => !isClosedIncident(i)).length,
    closed: incidents.filter(isClosedIncident).length,
    resources: resources.length, hospitals: hospitals.length, links: links.length, routes: routes.length,
  };

  return (
    <div className={`map-wrap ${className}`}>
      <div ref={box} className="map-canvas" />
      {layerControl && (
        <div className="map-overlay map-layers">
          <button type="button" className="map-layers-toggle" onClick={() => setPanelOpen((o) => !o)}>Layers {panelOpen ? '▴' : '▾'}</button>
          {panelOpen && LAYER_DEFS.map(([key, label]) => (
            <label key={key} className="map-layers-row">
              <input type="checkbox" checked={vis[key]} onChange={(e) => setVis((v) => ({ ...v, [key]: e.target.checked }))} />
              <span>{label}</span><span className="mono faint">{counts[key]}</span>
            </label>
          ))}
        </div>
      )}
      {children}
    </div>
  );
}
