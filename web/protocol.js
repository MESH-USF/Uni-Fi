// Companion wire layouts verified against firmware/examples/companion_radio/MyMesh.cpp.
export const MAX_FRAME = 172;
export const PRESENCE_TTL = 4 * 60 * 60 * 1000;
export const encoder = new TextEncoder();
const decoder = new TextDecoder('utf-8', { fatal: true });
export const labels = Object.freeze({ presence: 'Available', sos: 'SOS', medical: 'Medical help', pickup: 'Pickup needed', safe: 'I am safe', enroute: 'On my way', location: 'Location', ack: 'Received' });
const idPattern = /^[a-zA-Z0-9_-]{1,32}$/;
const nodePattern = /^[a-fA-F0-9]{12}$/;
const allowedFields = new Set(['type', 'id', 'node', 'lat', 'lon', 'ack']);
const numberPattern = /^-?(?:\d+(?:\.\d*)?|\.\d+)$/;

export function decodeEnvelope(text) {
  if (typeof text !== 'string' || text.includes('\0') || encoder.encode(text).length > 160) return null;
  const start = text.indexOf('[mc:v1;');
  if (start < 0 || text.lastIndexOf('[mc:v1;') !== start || !text.endsWith(']')) return null;
  const fields = {};
  for (const part of text.slice(start + 7, -1).split(';')) {
    const equals = part.indexOf('=');
    if (equals < 1 || equals === part.length - 1) return null;
    const key = part.slice(0, equals);
    if (!allowedFields.has(key) || Object.hasOwn(fields, key)) return null;
    fields[key] = part.slice(equals + 1);
  }
  if (!Object.hasOwn(labels, fields.type) || !idPattern.test(fields.id ?? '') || !nodePattern.test(fields.node ?? '')) return null;
  if ((fields.lat === undefined) !== (fields.lon === undefined)) return null;
  if (fields.lat !== undefined) {
    if (fields.lat.length > 23 || fields.lon.length > 23 || !numberPattern.test(fields.lat) || !numberPattern.test(fields.lon)) return null;
    const lat = Number(fields.lat), lon = Number(fields.lon);
    if (!Number.isFinite(lat) || !Number.isFinite(lon) || lat < -90 || lat > 90 || lon < -180 || lon > 180) return null;
    fields.lat = lat; fields.lon = lon;
  }
  if (fields.type === 'ack' ? !idPattern.test(fields.ack ?? '') : fields.ack !== undefined) return null;
  fields.node = fields.node.toLowerCase();
  return fields;
}

export function encodeEnvelope({ type, id, node, ack, lat, lon }) {
  let text = `${labels[type] ?? ''} [mc:v1;type=${type};id=${id};node=${node}`;
  if (lat !== undefined) text += `;lat=${Number(lat).toFixed(6)}`;
  if (lon !== undefined) text += `;lon=${Number(lon).toFixed(6)}`;
  if (ack !== undefined) text += `;ack=${ack}`;
  text += ']';
  if (!decodeEnvelope(text)) throw new Error('Invalid emergency envelope');
  return text;
}

export function appStart() {
  const name = encoder.encode('Uni-Fi Web');
  const frame = new Uint8Array(8 + name.length);
  frame[0] = 1; frame.set(name, 8);
  return frame;
}

export function timeCommand(seconds) {
  const frame = new Uint8Array(5); frame[0] = 6;
  new DataView(frame.buffer).setUint32(1, seconds, true);
  return frame;
}

export function nameCommand(name) {
  const bytes = encoder.encode(name);
  if (!name.trim() || name !== name.trim() || bytes.length > 31 || /[\x00-\x1f\x7f:\[\]]/.test(name)) {
    throw new Error('Use 1–31 UTF-8 bytes, with no control characters, brackets, or colons');
  }
  return Uint8Array.of(8, ...bytes);
}

export function messageBudget(name) { return 160 - encoder.encode(`${name}: `).length; }
export function channelMessage(text, name, seconds) {
  const bytes = encoder.encode(text);
  if (!text.trim() || text.includes('\0')) throw new Error('Enter a non-empty message without null characters');
  if (bytes.length > messageBudget(name) || bytes.length + 7 > MAX_FRAME) {
    throw new Error(`Message exceeds ${messageBudget(name)} UTF-8 bytes after the device name`);
  }
  const frame = new Uint8Array(7 + bytes.length);
  frame[0] = 3; frame[1] = 0; frame[2] = 0;
  new DataView(frame.buffer).setUint32(3, seconds, true);
  frame.set(bytes, 7); return frame;
}

const view = (frame) => new DataView(frame.buffer, frame.byteOffset, frame.byteLength);
const cstring = (bytes) => decoder.decode(bytes.subarray(0, bytes.indexOf(0) < 0 ? bytes.length : bytes.indexOf(0)));
export function parseSelf(frame) {
  if (frame[0] !== 5 || frame.length < 59) throw new Error('Malformed self-info frame');
  const data = view(frame);
  const key = Array.from(frame.subarray(4, 36), (b) => b.toString(16).padStart(2, '0')).join('');
  return { key, node: key.slice(0, 12), name: cstring(frame.subarray(58)), txPower: frame[2], frequency: data.getUint32(48, true) / 1000, bandwidth: data.getUint32(52, true) / 1000, sf: frame[56], cr: frame[57] };
}
export function parseChannel(frame) {
  if (frame[0] !== 18 || frame.length !== 50) throw new Error('Malformed channel-info frame');
  // Uni-Fi deliberately masks the secret. Zero bytes do not mean unprovisioned.
  return { index: frame[1], name: cstring(frame.subarray(2, 34)) };
}
export function parseStatus(frame) {
  if (frame[0] !== 0x70 || frame.length !== 101 || frame[1] !== 1 || frame[3] !== 0) throw new Error('Unsupported or malformed Uni-Fi status frame');
  const ownId = cstring(frame.subarray(4, 36)), pendingId = cstring(frame.subarray(36, 68));
  if ((ownId && !idPattern.test(ownId)) || (pendingId && !idPattern.test(pendingId))) throw new Error('Invalid device event ID');
  const flags = frame[2];
  if (((flags & 6) && !ownId) || ((flags & 8) && !pendingId)) throw new Error('Device status event flag has no event ID');
  return { provisioned: Boolean(flags & 1), ownWaiting: Boolean(flags & 2), ownAcked: Boolean(flags & 4), pending: Boolean(flags & 8), gps: Boolean(flags & 16), relay: Boolean(flags & 32), ownId, pendingId, pendingName: cstring(frame.subarray(68, 100)), rosterCount: frame[100] };
}
export function parseChannelMessage(frame) {
  const base = frame[0] === 17 ? 4 : frame[0] === 8 ? 1 : -1;
  if (base < 0 || frame.length < base + 7) throw new Error('Malformed channel-message frame');
  if (frame[base + 2] !== 0) throw new Error('Unsupported channel text type');
  const text = cstring(frame.subarray(base + 7));
  const separator = text.indexOf(': ');
  if (separator <= 0) throw new Error('Channel message has no sender name');
  nameCommand(text.slice(0, separator));
  if (encoder.encode(text).length > 160) throw new Error('Channel message exceeds radio text budget');
  return { channel: frame[base], path: frame[base + 1], snr: base === 4 ? (frame[1] > 127 ? frame[1] - 256 : frame[1]) / 4 : null, timestamp: view(frame).getUint32(base + 3, true), name: text.slice(0, separator), text: text.slice(separator + 2) };
}

export function serialFrame(payload) {
  if (!payload.length || payload.length > MAX_FRAME) throw new Error('Invalid serial frame length');
  return Uint8Array.of(60, payload.length & 255, payload.length >> 8, ...payload);
}
export class SerialDecoder {
  constructor(onFrame) { this.onFrame = onFrame; this.buffer = new Uint8Array(); }
  feed(chunk) {
    const combined = new Uint8Array(this.buffer.length + chunk.length);
    combined.set(this.buffer); combined.set(chunk, this.buffer.length); this.buffer = combined;
    while (this.buffer.length) {
      const start = this.buffer.indexOf(62);
      if (start < 0) { this.buffer = new Uint8Array(); return; }
      this.buffer = this.buffer.slice(start);
      if (this.buffer.length < 3) return;
      const size = this.buffer[1] | (this.buffer[2] << 8);
      if (!size || size > 176) { this.buffer = this.buffer.slice(1); continue; }
      if (this.buffer.length < size + 3) return;
      const payload = this.buffer.slice(3, size + 3);
      this.buffer = this.buffer.slice(size + 3);
      this.onFrame(payload);
    }
  }
}
