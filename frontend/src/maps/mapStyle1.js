// Dark basemap: OpenStreetMap data, rendered by MapLibre GL JS.
// Map rendering only — every route, distance and spatial query comes from the
// C++ engine (spec §2.4: rendering and route computation stay separate).
//
// CARTO's basemap tiles now require an API key ("API KEY REQUIRED" tiles), so
// the default is OpenFreeMap: free OSM vector tiles, no key, no sign-up.
// Change BASEMAP below to switch source:
//   'openfreemap' → OpenFreeMap "dark" vector style (default, sharpest)
//   'osm'         → standard OpenStreetMap raster tiles, darkened in MapLibre
export const DELHI = { lon: 77.2167, lat: 28.6139 };

const BASEMAP = 'openfreemap';

const OSM_ATTRIBUTION = '© <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors';

// Fallback: official OSM raster tiles. MapLibre has no "invert" filter, but
// setting brightness-min above brightness-max inverts the light map into a dark one.
const osmDarkStyle = {
  version: 8,
  sources: {
    basemap: {
      type: 'raster',
      tiles: ['https://tile.openstreetmap.org/{z}/{x}/{y}.png'],
      tileSize: 256,
      attribution: OSM_ATTRIBUTION,
      maxzoom: 19,
    },
  },
  layers: [
    { id: 'bg', type: 'background', paint: { 'background-color': '#05080d' } },
    {
      id: 'basemap', type: 'raster', source: 'basemap',
      paint: {
        'raster-brightness-min': 0.92,
        'raster-brightness-max': 0.04,
        'raster-saturation': -0.85,
        'raster-contrast': 0.1,
        'raster-opacity': 0.95,
      },
    },
  ],
};

export const darkStyle = BASEMAP === 'osm' ? osmDarkStyle : 'https://tiles.openfreemap.org/styles/dark';

/** Polygon approximating a circle of `km` radius (for radius queries). */
export function circlePolygon(lat, lon, km, steps = 64) {
  const coords = [];
  for (let i = 0; i <= steps; i++) {
    const a = (i / steps) * 2 * Math.PI;
    const dLat = (km / 111.195) * Math.cos(a);
    const dLon = (km / (111.195 * Math.cos((lat * Math.PI) / 180))) * Math.sin(a);
    coords.push([lon + dLon, lat + dLat]);
  }
  return { type: 'Feature', geometry: { type: 'Polygon', coordinates: [coords] }, properties: {} };
}

export function rectPolygon(r) {
  const c = [[r.minLon, r.minLat], [r.maxLon, r.minLat], [r.maxLon, r.maxLat], [r.minLon, r.maxLat], [r.minLon, r.minLat]];
  return { type: 'Feature', geometry: { type: 'Polygon', coordinates: [c] }, properties: r };
}

export const fc = (features) => ({ type: 'FeatureCollection', features });
