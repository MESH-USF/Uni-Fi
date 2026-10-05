import test from 'node:test';
import assert from 'node:assert/strict';
import { CommandQueue, DeviceSession, NetworkState } from '../session.js';
import { encodeEnvelope, encoder, PRESENCE_TTL } from '../protocol.js';
const turn = () => new Promise((resolve) => setImmediate(resolve));
const node = 'abcdef012345', peer = '123456abcdef';
const sos = { type: 'sos', id: node + '00001234', node };
const msg = (event, extra = {}) => ({ channel: 0, name: 'Peer', text: encodeEnvelope(event), timestamp: 1, ...extra });

test('presence refresh is local, duplicate refreshes and expires after four hours', () => {
  let now = 100; const state = new NetworkState(() => now);
  state.receive(msg(sos)); now += PRESENCE_TTL - 1; state.trim(); assert.equal(state.roster.size, 1);
  state.receive(msg(sos, { timestamp: 0 })); now += 2; state.trim(); assert.equal(state.roster.size, 1);
  now += PRESENCE_TTL; state.trim(); assert.equal(state.roster.size, 0);
  assert.equal(state.incidents.size, 0);
});
test('invalid or other-channel messages cannot authorize nodes', () => {
  const state = new NetworkState(() => 0);
  state.receive(msg(sos, { channel: 1 })); state.receive({ channel: 0, name: 'Peer', text: 'hello' });
  assert.equal(state.roster.size, 0); assert.equal(state.messages.length, 1);
});
test('only a matching remote ACK changes a sent incident receipt status', () => {
  const state = new NetworkState(() => 0); state.sent(sos, 'Me'); const incident = state.incidents.values().next().value;
  state.receive(msg({ type: 'ack', id: 'a1', node: peer, ack: 'wrong' })); assert.equal(incident.ack, undefined);
  state.receive(msg({ type: 'ack', id: 'a2', node, ack: sos.id })); assert.equal(incident.ack, undefined);
  state.receive(msg({ type: 'ack', id: 'a3', node: peer, ack: sos.id })); assert.equal(incident.ack.node, peer); assert.match(incident.status, /Remote receipt/);
});
test('presence stays out of chat, duplicate events coalesce, and ACK refreshes incident TTL', () => {
  let now = 0; const state = new NetworkState(() => now);
  state.receive(msg({ type: 'presence', id: 'heartbeat1', node: peer })); assert.equal(state.messages.length, 0); assert.equal(state.roster.size, 1);
  state.sent(sos, 'Me'); state.receive(msg(sos)); assert.equal(state.messages.length, 1);
  state.receive(msg(sos)); assert.equal(state.messages.length, 1);
  now = PRESENCE_TTL - 100;
  state.receive(msg({ type: 'ack', id: 'ack1', node: peer, ack: sos.id }));
  const incident = state.incidents.values().next().value; assert.match(incident.status, /Remote receipt/);
  state.receive(msg(sos)); assert.match(state.incidents.values().next().value.status, /Remote receipt/);
  now += 101; state.trim(); assert.equal(state.incidents.size, 1);
  now += PRESENCE_TTL; state.trim(); assert.equal(state.incidents.size, 0);
});
test('browser roster remains bounded to 128 most recently heard nodes', () => {
  const state = new NetworkState(() => 0);
  for (let i = 0; i < 140; i++) state.receive(msg({ type: 'presence', id: String(i), node: i.toString(16).padStart(12, '0') }));
  assert.equal(state.roster.size, 128); assert.equal(state.messages.length, 0); assert.equal(state.roster.has('000000000000'), false);
});
test('commands serialize writes, ignore pushes, reject firmware errors and continue', async () => {
  const written = [], queue = new CommandQueue((frame) => written.push(frame[0]));
  const first = queue.request(Uint8Array.of(6), [0]); const second = queue.request(Uint8Array.of(31, 0), [18]);
  await turn(); assert.deepEqual(written, [6]); assert.equal(queue.receive(Uint8Array.of(0x83)), false);
  queue.receive(Uint8Array.of(0)); await first; await turn(); assert.deepEqual(written, [6, 31]);
  const rejected = assert.rejects(second, /error 2/); queue.receive(Uint8Array.of(1, 2)); await rejected;
  const third = queue.request(Uint8Array.of(8, 65), [0]); await turn(); const disabled = assert.rejects(third, /disabled/); queue.receive(Uint8Array.of(15)); await disabled;
  queue.close();
});
test('timeout closes queue and rejects waiting commands to avoid stale OK matching', async () => {
  let fatal = 0; const written = [], queue = new CommandQueue((frame) => written.push(frame[0]), { timeout: 15, onFatal: () => fatal++ });
  const first = assert.rejects(queue.request(Uint8Array.of(6), [0]), /timed out/);
  const second = assert.rejects(queue.request(Uint8Array.of(8), [0]), /timed out/);
  await Promise.all([first, second]); assert.equal(fatal, 1); assert.deepEqual(written, [6]);
  assert.equal(queue.receive(Uint8Array.of(0)), false); await assert.rejects(queue.request(Uint8Array.of(5), [9]), /closed/);
});
test('write failure and disconnect settle active and queued commands', async () => {
  const queue = new CommandQueue(() => Promise.reject(new Error('unplugged')));
  await assert.rejects(queue.request(Uint8Array.of(6), [0]), /unplugged/); assert.equal(queue.closed, true);
  const queue2 = new CommandQueue(() => {});
  const first = assert.rejects(queue2.request(Uint8Array.of(6), [0]), /Disconnected/);
  const second = assert.rejects(queue2.request(Uint8Array.of(8), [0]), /Disconnected/);
  queue2.close(); await Promise.all([first, second]);
});

class TestTransport {
  constructor() { this.commands = []; this.messages = []; this.closeCount = 0; this.statusFlags = 1; this.ownId = ''; this.pendingId = ''; this.pendingName = ''; this.legacy = false; }
  write(frame) {
    this.commands.push(frame[0]);
    let reply;
    if (frame[0] === 1) {
      reply = new Uint8Array(62); reply[0] = 5; reply.set([0xab, 0xcd, 0xef, 1, 0x23, 0x45], 4);
      const data = new DataView(reply.buffer); data.setUint32(48, 910525, true); data.setUint32(52, 62500, true);
      reply[56] = 7; reply[57] = 5; reply.set(encoder.encode('Test'), 58);
    } else if (frame[0] === 22) { reply = new Uint8Array(80); reply[0] = 13; reply.set(encoder.encode('unifi-prototype'), 60); }
    else if (frame[0] === 31) { reply = new Uint8Array(50); reply[0] = 18; reply.set(encoder.encode('Uni-Fi'), 2); }
    else if (frame[0] === 0x70) {
      if (this.legacy) reply = Uint8Array.of(1, 1);
      else {
        reply = new Uint8Array(101); reply[0] = 0x70; reply[1] = 1; reply[2] = this.statusFlags;
        reply.set(encoder.encode(this.ownId), 4); reply.set(encoder.encode(this.pendingId), 36); reply.set(encoder.encode(this.pendingName), 68);
      }
    }
    else if (frame[0] === 10) reply = this.messages.shift() ?? Uint8Array.of(10);
    else reply = Uint8Array.of(0);
    queueMicrotask(() => this.onFrame(reply));
  }
  async close() { this.closeCount++; }
}
function channelFrame(event) {
  const text = encoder.encode(`Peer: ${encodeEnvelope(event)}`), frame = new Uint8Array(11 + text.length);
  frame[0] = 17; frame[4] = 0; frame[5] = 255; new DataView(frame.buffer).setUint32(7, 100, true); frame.set(text, 11); return frame;
}
test('real command order initialization, offline queue drain, push drain, cleanup', async () => {
  const transport = new TestTransport(), session = new DeviceSession(transport);
  transport.messages.push(channelFrame(sos));
  await session.initialize(); assert.equal(session.ready, true);
  assert.deepEqual(transport.commands, [1, 22, 6, 31, 0x70, 10, 10]); assert.equal(session.network.roster.size, 1);
  transport.messages.push(channelFrame({ ...sos, node: peer, id: 'second' })); transport.onFrame(Uint8Array.of(0x83));
  await turn(); assert.equal(session.network.roster.size, 2);
  await session.close(); assert.equal(session.ready, false); assert.equal(transport.closeCount, 1);
  await session.close(); assert.equal(transport.closeCount, 1);
});
test('explicitly unprovisioned device fails initialization and cannot send', async () => {
  const transport = new TestTransport(); transport.statusFlags = 0;
  const session = new DeviceSession(transport);
  try {
    await assert.rejects(session.initialize(), /no Uni-Fi network key/);
    assert.equal(session.ready, false); await assert.rejects(session.sendText('test'), /initialize/);
    assert.equal(transport.commands.includes(3), false);
  } finally { await session.close(); }
});
test('unsupported status remains explicit legacy state and handshake still drains messages', async () => {
  const transport = new TestTransport(); transport.legacy = true; const logs = [];
  const session = new DeviceSession(transport, { onLog: (line) => logs.push(line) });
  try {
    await session.initialize(); assert.equal(session.ready, true); assert.equal(session.status, null);
    assert.ok(logs.some((line) => line.includes('cannot be independently verified')));
  } finally { await session.close(); }
});
test('physical-button SOS status appears as a pending incident and actual device receipt updates it', async () => {
  const transport = new TestTransport(); transport.statusFlags = 1 | 2 | 8; transport.ownId = sos.id; transport.pendingId = 'peerDistress'; transport.pendingName = 'Victim';
  const session = new DeviceSession(transport);
  try {
    await session.initialize(); const incident = session.network.incidents.values().next().value;
    assert.equal(incident.event.id, sos.id); assert.match(incident.status, /awaiting/);
    assert.equal(session.status.pendingId, 'peerDistress'); assert.equal(session.status.pendingName, 'Victim');
    transport.statusFlags = 1 | 4;
    await session.refreshStatus(); assert.match(incident.status, /Remote receipt confirmed by device/);
    assert.equal(session.status.pending, false);
  } finally { await session.close(); }
});
