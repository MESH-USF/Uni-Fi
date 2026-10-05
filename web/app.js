import { BluetoothTransport, SerialTransport } from './transports.js';
import { DeviceSession } from './session.js';
import { messageBudget, encoder } from './protocol.js';

const $ = (id) => document.getElementById(id);
let session = null, busy = false, connecting = false, lastReadName = null;
const logLines = [];
function log(text) {
  logLines.push(`${new Date().toLocaleTimeString()}  ${text}`);
  if (logLines.length > 120) logLines.shift();
  $('log').textContent = logLines.join('\n');
}
function notice(text, error = false) { $('notice').textContent = text; $('notice').classList.toggle('error', error); }
function element(tag, text, className) { const item = document.createElement(tag); item.textContent = text; if (className) item.className = className; return item; }
function empty(container, text) { container.replaceChildren(element('p', text, 'empty')); }
function cardHeader(title, meta) { const item = document.createElement('div'); item.className = 'card-header'; item.append(element('strong', title), element('span', meta, 'meta')); return item; }
function render() {
  const ready = Boolean(session?.ready && !session.closed);
  $('connection-state').classList.toggle('connected', ready);
  $('connection-state').replaceChildren(element('i', ''), document.createTextNode(ready ? 'Connected' : connecting ? 'Connecting…' : 'Disconnected'));
  $('connect-ble').disabled = connecting || Boolean(session && !session.closed);
  $('connect-usb').disabled = connecting || Boolean(session && !session.closed);
  $('disconnect').disabled = !session || session.closed || connecting;
  document.querySelectorAll('[data-device]').forEach((button) => { button.disabled = !ready || busy; });
  $('device-details').hidden = !ready;
  if (ready) {
    $('device-name').textContent = session.self.name;
    $('node-id').textContent = session.self.node;
    $('radio').textContent = `${session.self.frequency.toFixed(3)} MHz · BW ${session.self.bandwidth} · SF${session.self.sf} · CR${session.self.cr}`;
    $('firmware-version').textContent = session.version;
    const status = session.status;
    $('device-status').textContent = status ? `Key provisioned · ${status.rosterCount} nodes retained on device · ${status.gps ? 'GPS compiled' : 'GPS deferred'} · ${status.relay ? 'Fleet relay enabled' : 'Relay disabled'}` : 'Legacy firmware: provisioning status unavailable; channel name alone cannot verify a network key.';
    $('pending-distress').hidden = !status?.pending;
    $('respond-device').hidden = !status?.pending;
    if (status?.pending) $('pending-distress').textContent = `Device has an unacknowledged distress from ${status.pendingName || 'a network node'} · ${status.pendingId}`;
    if (lastReadName !== session.self.name) { $('name-input').value = session.self.name; lastReadName = session.self.name; }
  }
  const used = encoder.encode($('message-input').value).length;
  $('message-budget').textContent = ready ? `${used} / ${messageBudget(session.self.name)} UTF-8 bytes` : 'Connect to see available message space';
  if (!session) return;
  session.network.trim();
  const incidents = Array.from(session.network.incidents.values()).reverse();
  $('incident-count').textContent = String(incidents.length);
  $('incidents').replaceChildren();
  if (!incidents.length) empty($('incidents'), 'SOS, medical, and pickup messages appear here. Received calls include a response button.');
  for (const incident of incidents) {
    const card = element('div', '', 'card');
    card.append(cardHeader(incident.name, incident.event.type.toUpperCase()), element('p', incident.event.id, 'meta'), element('p', incident.status, 'state'));
    if (!incident.outgoing) {
      const button = element('button', incident.responseSent ? 'Send receipt again' : 'Respond: received');
      button.disabled = !ready || busy;
      button.addEventListener('click', () => action(async () => {
        await session.sendEvent('ack', incident.event.id);
        incident.responseSent = true;
        notice('Receipt queued on your device. This confirms that you received the call; it does not promise that help will arrive.');
      }));
      card.append(button);
    }
    $('incidents').append(card);
  }
  const roster = Array.from(session.network.roster.values()).sort((a, b) => b.seen - a.seen);
  $('roster-count').textContent = String(roster.length);
  $('roster').replaceChildren();
  if (!roster.length) empty($('roster'), 'Waiting to hear another Uni-Fi node.');
  for (const entry of roster) {
    const card = element('div', '', 'card');
    const minutes = Math.floor((performance.now() - entry.seen) / 60000);
    card.append(cardHeader(entry.name, minutes < 1 ? 'Just heard' : `${minutes}m ago`), element('p', entry.node, 'meta'));
    if (entry.lat !== undefined) card.append(element('p', `Coordinates received: ${entry.lat}, ${entry.lon} · map planned`, 'meta'));
    $('roster').append(card);
  }
  $('messages').replaceChildren();
  if (!session.network.messages.length) empty($('messages'), 'Incoming messages will appear here.');
  for (const message of session.network.messages.slice(0, 30)) {
    const card = element('div', '', 'card');
    card.append(cardHeader(message.name, new Date(message.receivedAt).toLocaleTimeString()), element('p', message.text));
    $('messages').append(card);
  }
}
async function action(run) {
  if (busy || connecting) return;
  busy = true; render();
  try { await run(); }
  catch (error) { notice(error.message, true); log(error.message); }
  finally { busy = false; render(); }
}
async function connect(kind) {
  if (connecting || (session && !session.closed)) return;
  connecting = true; lastReadName = null; render();
  notice(kind === 'ble' ? 'Choose your Heltec device, then complete Bluetooth pairing if requested.' : 'Choose your Heltec USB serial port. Close any serial monitor or flashing tool first.');
  const transport = kind === 'ble' ? new BluetoothTransport() : new SerialTransport();
  try {
    session = new DeviceSession(transport, {
      onChange: render, onLog: log,
      onDisconnect: () => { notice('Disconnected. Reconnect to send messages.'); render(); },
    });
    await transport.connect();
    if (session.closed) throw new Error('Device disconnected during connection');
    await session.initialize();
    notice('Connected to Uni-Fi. Incoming messages are being read from the device.');
    log(`Connected via ${kind === 'ble' ? 'Bluetooth' : 'USB serial'} to ${session.self.node}`);
  } catch (error) {
    if (session) await session.close();
    notice(error.message, true); log(error.message);
  } finally { connecting = false; render(); }
}
$('connect-ble').addEventListener('click', () => connect('ble'));
$('connect-usb').addEventListener('click', () => connect('usb'));
$('disconnect').addEventListener('click', () => action(() => session.close()));
$('respond-device').addEventListener('click', () => action(async () => {
  if (!session.status?.pending) throw new Error('No pending distress on device');
  await session.sendEvent('ack', session.status.pendingId);
  if (session.status) await session.refreshStatus();
  notice('Receipt queued on device for the pending distress.');
}));
$('name-form').addEventListener('submit', (event) => { event.preventDefault(); const name = $('name-input').value; void action(async () => { await session.rename(name); notice('Device name saved and read back successfully.'); }); });
document.querySelectorAll('[data-event]').forEach((button) => button.addEventListener('click', () => action(async () => {
  await session.sendEvent(button.dataset.event);
  notice(`${button.querySelector('strong').textContent}: queued on device. A remote receipt has not yet been confirmed.`);
})));
$('message-form').addEventListener('submit', (event) => { event.preventDefault(); void action(async () => {
  if ($('message-input').value.includes('[mc:')) throw new Error('Use quick messages or the incident response button to send structured events.');
  await session.sendText($('message-input').value); $('message-input').value = ''; notice('Message queued on device; remote delivery is unconfirmed.');
}); });
$('message-input').addEventListener('input', render);
$('clear-log').addEventListener('click', () => { logLines.length = 0; $('log').textContent = 'Log cleared.'; });
window.addEventListener('pagehide', () => { if (session) void session.close(); });
setInterval(render, 30000);
if (!window.isSecureContext) notice('Open this page over HTTPS or http://localhost to enable hardware connections.', true);
else if (!navigator.bluetooth && !navigator.serial) $('support').textContent = 'This browser exposes neither Web Bluetooth nor Web Serial. Open in Chrome/Edge on a supported desktop.';
render();
