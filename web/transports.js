import { serialFrame, SerialDecoder, MAX_FRAME } from './protocol.js';
const SERVICE = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const RX = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
const TX = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

export class BluetoothTransport {
  async connect() {
    if (!navigator.bluetooth) throw new Error('Web Bluetooth is unavailable. Use Chrome/Edge on a supported desktop, HTTPS or localhost.');
    this.device = await navigator.bluetooth.requestDevice({ filters: [{ services: [SERVICE] }] });
    this.disconnected = () => this.onDisconnect?.();
    this.device.addEventListener('gattserverdisconnected', this.disconnected);
    try {
      const server = await this.device.gatt.connect();
      const service = await server.getPrimaryService(SERVICE);
      this.rx = await service.getCharacteristic(RX);
      this.tx = await service.getCharacteristic(TX);
      this.notification = (event) => {
        const value = event.target.value;
        this.onFrame?.(new Uint8Array(value.buffer, value.byteOffset, value.byteLength).slice());
      };
      this.tx.addEventListener('characteristicvaluechanged', this.notification);
      await this.tx.startNotifications();
    } catch (error) { await this.close(); throw error; }
  }
  async write(frame) {
    if (!this.device?.gatt.connected || !this.rx) throw new Error('Bluetooth disconnected');
    if (frame.length > MAX_FRAME) throw new Error('Command too large');
    // ESP32 treats each characteristic write as a complete companion frame.
    // Do not split a long command into independent GATT writes.
    await this.rx.writeValueWithResponse(frame);
  }
  async close() {
    this.device?.removeEventListener('gattserverdisconnected', this.disconnected);
    if (this.tx && this.notification) this.tx.removeEventListener('characteristicvaluechanged', this.notification);
    if (this.device?.gatt.connected) this.device.gatt.disconnect();
    this.rx = null; this.tx = null;
  }
}

export class SerialTransport {
  async connect() {
    if (!navigator.serial) throw new Error('Web Serial is unavailable. Use Chrome/Edge desktop, HTTPS or localhost.');
    this.port = await navigator.serial.requestPort();
    try {
      await this.port.open({ baudRate: 115200 });
      this.reader = this.port.readable.getReader(); this.writer = this.port.writable.getWriter();
      this.decoder = new SerialDecoder((frame) => this.onFrame?.(frame));
      this.running = true;
      this.readTask = this.read();
    } catch (error) { await this.close(); throw error; }
  }
  async read() {
    try {
      while (this.running) {
        const { value, done } = await this.reader.read();
        if (done) break;
        this.decoder.feed(value);
      }
    } catch (error) { if (this.running) this.lastError = error; }
    finally { if (this.running) { this.running = false; this.onDisconnect?.(); } }
  }
  async write(frame) {
    if (!this.running || !this.writer) throw new Error('USB serial disconnected');
    await this.writer.write(serialFrame(frame));
  }
  async close() {
    this.running = false;
    try { if (this.reader) await this.reader.cancel(); } catch { /* unplugged */ }
    if (this.readTask) await this.readTask;
    if (this.reader) { this.reader.releaseLock(); this.reader = null; }
    if (this.writer) { this.writer.releaseLock(); this.writer = null; }
    try { if (this.port) await this.port.close(); } catch { /* already closed */ }
  }
}
