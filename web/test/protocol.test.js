import test from 'node:test';
import assert from 'node:assert/strict';
import { decodeEnvelope, encodeEnvelope, channelMessage, messageBudget, nameCommand, parseSelf, parseChannel, parseStatus, parseChannelMessage, SerialDecoder, serialFrame, encoder } from '../protocol.js';

const node = 'abcdef012345';
const event = { type: 'sos', id: node + '00001234', node };
test('compact events round-trip including optional future GPS and ACK references', () => {
  assert.deepEqual(decodeEnvelope(encodeEnvelope(event)), event);
  const ack = { type: 'ack', id: 'abcdef0123451', node, ack: event.id };
  assert.deepEqual(decodeEnvelope(encodeEnvelope(ack)), ack);
  assert.deepEqual(decodeEnvelope(encodeEnvelope({ ...event, lat: 28.12, lon: -82.25 })), { ...event, lat: 28.12, lon: -82.25 });
  assert.deepEqual(decodeEnvelope(encodeEnvelope({ ...event, lat: 0, lon: 0 })), { ...event, lat: 0, lon: 0 });
});
test('reject malformed envelopes rather than authorizing a roster entry', () => {
  const text = encodeEnvelope(event);
  for (const invalid of [text + 'extra', text.slice(0, -1), text.replace('type=sos', 'type=unknown'), text.replace('node=abcdef012345', 'node=abcd'), text.replace('id=abcdef01234500001234', 'id=a=b'), text.replace('type=sos', 'type=sos;type=ack'), text.replace('type=sos', 'type=ack'), text.replace('type=sos', 'type=sos;ack=1'), text.replace('type=sos', 'type=sos;lat=12'), text.replace('type=sos', 'type=sos;lat=NaN;lon=1'), text.replace('type=sos', 'type=sos;lat=91;lon=1'), text.replace('type=sos', 'type=sos;lat=1;lon=Infinity'), text.replace('type=sos', 'type=sos;extra=1'), '[mc:v1;' + text, text + '\0']) assert.equal(decodeEnvelope(invalid), null, invalid);
});
test('UTF-8 radio budget accounts for sender prefix and never silently truncates', () => {
  const name = '救援';
  assert.equal(messageBudget(name), 152);
  const text = 'é'.repeat(76);
  const frame = channelMessage(text, name, 0x12345678);
  assert.equal(frame.length, 159);
  assert.deepEqual(Array.from(frame.slice(0, 7)), [3, 0, 0, 0x78, 0x56, 0x34, 0x12]);
  assert.throws(() => channelMessage(text + 'a', name, 1), /exceeds/);
  assert.doesNotThrow(() => channelMessage(encodeEnvelope({ type: 'ack', id: event.id, node, ack: event.id }), 'a'.repeat(31), 1));
  assert.throws(() => channelMessage('a\0b', name, 1), /null/);
});
test('names are bounded by bytes and cannot break sender parsing', () => {
  assert.equal(nameCommand('é'.repeat(15)).length, 31);
  for (const name of ['', '  ', 'abc: def', 'a:b', 'a\nb', 'a\0b', '[name]', ' abc', 'a'.repeat(32), 'é'.repeat(16)]) assert.throws(() => nameCommand(name));
});
test('self/channel offsets match current firmware, zero masked key is accepted', () => {
  const frame = new Uint8Array(62); const data = new DataView(frame.buffer);
  frame[0] = 5; frame[2] = 20; frame.set([0xab, 0xcd, 0xef, 1, 0x23, 0x45], 4);
  data.setUint32(48, 910525, true); data.setUint32(52, 62500, true); frame[56] = 7; frame[57] = 5; frame.set(encoder.encode('Test'), 58);
  const self = parseSelf(frame); assert.equal(self.node, node); assert.equal(self.name, 'Test'); assert.equal(self.frequency, 910.525); assert.equal(self.bandwidth, 62.5);
  const channel = new Uint8Array(50); channel[0] = 18; channel.set(encoder.encode('Uni-Fi'), 2);
  assert.deepEqual(parseChannel(channel), { index: 0, name: 'Uni-Fi' });
  assert.throws(() => parseSelf(frame.slice(0, 58)));
  assert.throws(() => parseChannel(channel.slice(0, 49)));
});
test('received v1/v3 text frames read SNR, channel and timestamp at correct offsets', () => {
  for (const version of [8, 17]) {
    const base = version === 17 ? 4 : 1, text = encoder.encode('Sanjay: hello');
    const frame = new Uint8Array(base + 7 + text.length); frame[0] = version; if (base === 4) frame[1] = 248;
    frame[base] = 0; frame[base + 1] = 2; frame[base + 2] = 0;
    new DataView(frame.buffer).setUint32(base + 3, 123456, true); frame.set(text, base + 7);
    assert.deepEqual(parseChannelMessage(frame), { channel: 0, path: 2, snr: base === 4 ? -2 : null, timestamp: 123456, name: 'Sanjay', text: 'hello' });
    frame[base + 2] = 2; assert.throws(() => parseChannelMessage(frame));
  }
});
test('Uni-Fi status reads fixed-size IDs, flags and retained roster independently of masked key', () => {
  const frame = new Uint8Array(101); frame[0] = 0x70; frame[1] = 1; frame[2] = 1 | 4 | 8 | 32;
  frame.set(encoder.encode(event.id), 4); frame.set(encoder.encode('pending123'), 36); frame.set(encoder.encode('Peer'), 68); frame[100] = 9;
  assert.deepEqual(parseStatus(frame), { provisioned: true, ownWaiting: false, ownAcked: true, pending: true, gps: false, relay: true, ownId: event.id, pendingId: 'pending123', pendingName: 'Peer', rosterCount: 9 });
  frame[2] = 0; assert.equal(parseStatus(frame).provisioned, false);
  frame[1] = 2; assert.throws(() => parseStatus(frame));
  frame[1] = 1; frame[2] = 2; frame.fill(0, 4, 36); assert.throws(() => parseStatus(frame), /no event ID/);
});
test('USB decoder tolerates byte-by-byte fragmentation, combined frames and boot noise', () => {
  const frames = [], parser = new SerialDecoder((frame) => frames.push(Array.from(frame)));
  const stream = Uint8Array.of(10, 85, 62, 0, 0, 62, 255, 255, 62, 3, 0, 5, 62, 0, 62, 1, 0, 10);
  for (const byte of stream) parser.feed(Uint8Array.of(byte));
  assert.deepEqual(frames, [[5, 62, 0], [10]]);
  parser.feed(Uint8Array.of(62, 1, 0, 0, 62, 2, 0, 1, 4));
  assert.deepEqual(frames.slice(-2), [[0], [1, 4]]);
  assert.deepEqual(Array.from(serialFrame(Uint8Array.of(31, 0))), [60, 2, 0, 31, 0]);
});
