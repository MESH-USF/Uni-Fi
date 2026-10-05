#include "../../examples/companion_radio/UniFiProtocol.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static unsigned assertions = 0;
#define CHECK(condition) do { ++assertions; if (!(condition)) { \
  std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition); std::exit(1); \
} } while (0)

static std::string message(const std::string& type = "sos", const std::string& id = "event1",
                           const std::string& node = "aabbccddeeff", const std::string& extras = "") {
  return "Alice: SOS [mc:v1;type=" + type + ";id=" + id + ";node=" + node + extras + "]";
}

static unifi::Envelope event(const std::string& type = "sos", const std::string& id = "event1",
                             const std::string& node = "aabbccddeeff", const std::string& extra = "") {
  const auto text = message(type, id, node, extra);
  unifi::Envelope value;
  CHECK(unifi::parse(text.data(), text.size(), value));
  return value;
}

static bool parses(const std::string& text) {
  unifi::Envelope value;
  return unifi::parse(text.data(), text.size(), value);
}

static void parserTests() {
  auto parsed = event();
  CHECK(parsed.type == unifi::Type::Sos);
  CHECK(std::string(parsed.name) == "Alice");
  CHECK(parsed.node[0] == 0xaa && parsed.node[5] == 0xff);
  CHECK(!parsed.has_location);
  for (const char* type : {"presence", "sos", "medical", "pickup", "safe", "enroute", "location"}) CHECK(parses(message(type)));
  CHECK(parses(message("ack", "ack1", "aabbccddeeff", ";ack=event1")));
  CHECK(parses(message("sos", std::string(32, 'a'))));
  CHECK(!parses(message("sos", std::string(33, 'a'))));
  for (const auto& bad : {
      message("unknown"), message("sos", ""), message("sos", "has spaces"),
      message("sos", "event1", "a"), message("sos", "event1", "aabbccddeeff00"),
      message("sos", "event1", "aabbccddeefg"),
      message("sos", "event1", "aabbccddeeff", ";type=medical"),
      message("sos", "event1", "aabbccddeeff", ";id=event2"),
      message("sos", "event1", "aabbccddeeff", ";node=aabbccddeeff"),
      message("sos", "event1", "aabbccddeeff", ";unknown=1"),
      message("sos", "event1", "aabbccddeeff", ";ack=event0"),
      message("ack"), message("ack", "ack1", "aabbccddeeff", ";ack="),
      message("sos", "event1", "aabbccddeeff", ";"),
      std::string("Alice: [mc:v1;type=sos;id=1]"),
      std::string("Alice: [mc:v1;id=1;node=aabbccddeeff]"),
      std::string("Alice: [mc:v1;type=sos;node=aabbccddeeff]"),
      std::string("SOS [mc:v1;type=sos;id=1;node=aabbccddeeff]"),
      message() + "trailing", message() + message(),
      std::string(32, 'n') + message().substr(5), std::string("A\nlice") + message().substr(5)}) CHECK(!parses(bad));

  parsed = event("location", "loc1", "aabbccddeeff", ";lat=28.058703;lon=-82.413862");
  CHECK(parsed.has_location && parsed.latitude_e6 == 28058703 && parsed.longitude_e6 == -82413862);
  CHECK(parses(message("location", "1", "aabbccddeeff", ";lat=-90.000000;lon=180.000000")));
  for (const char* bad : {";lat=28", ";lon=-82", ";lat=91;lon=1", ";lat=1;lon=-181",
      ";lat=nan;lon=1", ";lat=1;lon=inf", ";lat=28abc;lon=1", ";lat=1e3;lon=1",
      ";lat=;lon=1", ";lat=--1;lon=1", ";lat=28;lon=1;lat=29"})
    CHECK(!parses(message("location", "1", "aabbccddeeff", bad)));

  const std::string good = message();
  // Test each truncation on an exactly sized buffer (no trailing NUL to mask an
  // out-of-bounds read); the sanitizer also checks these malformed radio inputs.
  for (size_t n = 0; n < good.size(); ++n) {
    const std::vector<char> raw(good.begin(), good.begin() + n);
    unifi::Envelope value;
    CHECK(!unifi::parse(raw.data(), raw.size(), value));
  }
  const std::vector<char> raw(good.begin(), good.end());
  CHECK(unifi::parse(raw.data(), raw.size(), parsed));
  std::string embedded = good;
  embedded[2] = 0;
  CHECK(!parses(embedded));
  CHECK(!parses(std::string(161, 'x')));
  const std::string outgoing = good.substr(7);
  CHECK(unifi::parse(outgoing.data(), outgoing.size(), parsed, false));
}

static void presenceTests() {
  unifi::PresenceTable<2> table;
  unifi::Presence copy[2];
  auto alice = event("presence");
  table.update(alice, 0, 100); // Uptime zero is a valid used entry.
  CHECK(table.copy(copy, 2, 0) == 1 && copy[0].last_seen == 100);
  table.update(alice, unifi::kPresenceTtl - 1, 200);
  CHECK(table.copy(copy, 2, unifi::kPresenceTtl) == 1 && copy[0].last_seen == 200);
  CHECK(table.copy(copy, 2, 2 * unifi::kPresenceTtl - 2) == 1);
  CHECK(table.copy(copy, 2, 2 * unifi::kPresenceTtl - 1) == 0);
  CHECK(table.copy(nullptr, 2, 0) == 0 && table.copy(copy, -1, 0) == 0);

  const uint32_t near_wrap = UINT32_MAX - 5000;
  table.update(alice, near_wrap, 1);
  CHECK(table.contains(alice.node, 200));
  CHECK(table.copy(copy, 2, near_wrap + unifi::kPresenceTtl - 1) == 1);
  CHECK(table.copy(copy, 2, near_wrap + unifi::kPresenceTtl) == 0);

  table.update(alice, UINT32_MAX - 100, 1);
  auto bob = event("presence", "bob1", "112233445566");
  table.update(bob, 10, 2);
  auto carol = event("presence", "carol1", "102030405060");
  table.update(carol, 20, 3);
  CHECK(!table.contains(alice.node, 20)); // Eviction uses age, not numeric millis.
  CHECK(table.contains(bob.node, 20) && table.contains(carol.node, 20));
  CHECK(table.count(20) == 2);
  CHECK(table.copy(copy, 1, 20) == 1);

  alice = event("presence", "loc", "aabbccddeeff", ";lat=28.058703;lon=-82.413862");
  unifi::PresenceTable<1> location;
  location.update(alice, 0, 0);
  location.update(event("ack", "a1", "aabbccddeeff", ";ack=someone"), 1, 1);
  CHECK(location.copy(copy, 1, 1) == 1 && copy[0].has_location);
  CHECK(copy[0].latitude_e6 == 28058703);
  location.update(bob, 2, 2);
  CHECK(location.copy(copy, 1, 2) == 1 && !copy[0].has_location);
}

static void emergencyTests() {
  const uint8_t self[6] = {1, 2, 3, 4, 5, 6};
  using Receive = unifi::EmergencyState::Receive;
  unifi::EmergencyState state;
  CHECK(!state.hasPending() && !state.ownAcknowledged());
  CHECK(state.receive(event(), self, 0) == Receive::Distress);
  CHECK(state.hasPending() && std::string(state.pendingId()) == "event1");
  CHECK(std::string(state.pendingName()) == "Alice");
  CHECK(state.receive(event(), self, 100) == Receive::None);
  CHECK(state.receive(event("presence"), self, 101) == Receive::None);
  CHECK(state.receive(event("medical", "med1"), self, 102) == Receive::Distress);
  CHECK(std::string(state.pendingId()) == "med1");
  state.sent(event("ack", "wrongAck", "010203040506", ";ack=event1"), 103);
  CHECK(state.hasPending());
  state.sent(event("ack", "ack1", "010203040506", ";ack=med1"), 104);
  CHECK(!state.hasPending());
  CHECK(state.receive(event("medical", "med1"), self, 105) == Receive::None);
  CHECK(state.receive(event("pickup", "pickup1"), self, 106) == Receive::Distress);
  state.expire(106 + unifi::kPresenceTtl);
  CHECK(!state.hasPending());

  state.sent(event("sos", "own1", "010203040506"), 200);
  CHECK(std::string(state.ownId()) == "own1" && !state.ownAcknowledged());
  CHECK(state.receive(event("ack", "ack3", "aabbccddeeff", ";ack=other"), self, 201) == Receive::None);
  CHECK(state.receive(event("ack", "ack4", "010203040506", ";ack=own1"), self, 202) == Receive::None);
  CHECK(!state.ownAcknowledged());
  CHECK(state.receive(event("sos", "selfSOS", "010203040506"), self, 203) == Receive::None);
  CHECK(state.receive(event("ack", "ack5", "aabbccddeeff", ";ack=own1"), self, 204) == Receive::OwnAcknowledged);
  CHECK(state.ownAcknowledged());
  CHECK(state.receive(event("ack", "ack6", "112233445566", ";ack=own1"), self, 205) == Receive::None);
  state.sent(event("sos", "own2", "010203040506"), 206);
  CHECK(!state.ownAcknowledged());
  CHECK(state.receive(event("ack", "ack7", "112233445566", ";ack=own1"), self, 207) == Receive::None);
  state.expire(206 + unifi::kPresenceTtl);
  CHECK(!state.ownId()[0] && !state.ownAcknowledged());
  CHECK(state.receive(event("ack", "ack8", "112233445566", ";ack=own2"), self, 206 + unifi::kPresenceTtl) == Receive::None);

  unifi::EmergencyState rollover;
  CHECK(rollover.receive(event(), self, UINT32_MAX - 100) == Receive::Distress);
  rollover.expire(100);
  CHECK(rollover.hasPending());
  rollover.expire(UINT32_MAX - 100 + unifi::kPresenceTtl);
  CHECK(!rollover.hasPending());

  unifi::EmergencyState refresh;
  CHECK(refresh.receive(event(), self, 0) == Receive::Distress);
  CHECK(refresh.receive(event(), self, unifi::kPresenceTtl - 1) == Receive::None);
  refresh.expire(unifi::kPresenceTtl);
  CHECK(refresh.hasPending());
  refresh.sent(event("ack", "r1", "010203040506", ";ack=event1"), unifi::kPresenceTtl + 1);
  CHECK(!refresh.hasPending());
  CHECK(refresh.receive(event(), self, unifi::kPresenceTtl + 2) == Receive::None);
  CHECK(!refresh.hasPending()); // A duplicate cannot reopen an acknowledged request.
  refresh.sent(event("sos", "own", "010203040506"), 0);
  CHECK(refresh.receive(event("ack", "r2", "aabbccddeeff", ";ack=own"), self, unifi::kPresenceTtl - 10) == Receive::OwnAcknowledged);
  CHECK(refresh.receive(event("ack", "r2", "aabbccddeeff", ";ack=own"), self, unifi::kPresenceTtl - 1) == Receive::None);
  refresh.expire(unifi::kPresenceTtl);
  CHECK(refresh.ownAcknowledged());
  CHECK(refresh.receive(event("ack", "r3", "112233445566", ";ack=own"), self, unifi::kPresenceTtl + 1) == Receive::None);
  refresh.expire(2 * unifi::kPresenceTtl);
  CHECK(refresh.ownAcknowledged());
}

static std::vector<uint8_t> frame(const std::string& text, bool v3) {
  std::vector<uint8_t> bytes(v3 ? 11 : 8, 0);
  bytes[0] = v3 ? 17 : 8;
  bytes.insert(bytes.end(), text.begin(), text.end());
  return bytes;
}

static void queueTests() {
  using Kind = unifi::QueueKind;
  const auto presence_bytes = frame(message("presence"), true);
  auto presence = unifi::classifyFrame(presence_bytes.data(), presence_bytes.size());
  CHECK(presence.kind == Kind::Presence && presence.node[0] == 0xaa);
  for (bool v3 : {false, true}) {
    const auto one_letter_name = frame("A: Available [mc:v1;type=presence;id=e;node=aabbccddeeff]", v3);
    CHECK(unifi::classifyFrame(one_letter_name.data(), one_letter_name.size()).kind == Kind::Presence);
    const auto one_letter_distress = frame("A: SOS [mc:v1;type=sos;id=e;node=aabbccddeeff]", v3);
    CHECK(unifi::classifyFrame(one_letter_distress.data(), one_letter_distress.size()).kind == Kind::Critical);
  }
  for (const char* type : {"sos", "medical", "pickup", "ack"}) {
    const auto bytes = frame(message(type, "e", "aabbccddeeff", std::string(type) == "ack" ? ";ack=t" : ""), false);
    CHECK(unifi::classifyFrame(bytes.data(), bytes.size()).kind == Kind::Critical);
  }
  const auto plain = frame("Alice: hello", true);
  CHECK(unifi::classifyFrame(plain.data(), plain.size()).kind == Kind::ChannelText);
  for (size_t n = 0; n < 12; ++n) CHECK(unifi::classifyFrame(plain.data(), n).kind != Kind::Presence);
  unifi::QueueInfo queue[3] = {{Kind::Critical, {}}, {Kind::ChannelText, {}}, presence};
  CHECK(unifi::queueSlot(queue, 3, 3, presence) == 2); // Same node coalesces.
  CHECK(unifi::queueSlot(queue, 2, 3, queue[0]) == 2); // Free slot preferred.
  CHECK(unifi::queueSlot(queue, 3, 3, queue[0]) == 2); // Evict heartbeat first.
  queue[2].kind = Kind::Other;
  CHECK(unifi::queueSlot(queue, 3, 3, queue[0]) == 1); // Then ordinary group text.
  queue[1].kind = Kind::Critical;
  CHECK(unifi::queueSlot(queue, 3, 3, presence) == -1); // Heartbeats cannot evict distress/direct.
  CHECK(unifi::queueSlot(queue, 3, 3, {Kind::ChannelText, {}}) == -1);
  CHECK(unifi::queueSlot(queue, 3, 3, queue[0]) == 0); // Bounded critical overflow evicts oldest.
  CHECK(unifi::queueSlot(queue, 3, 2, presence) == -1);
}

static void forwardingAndCommandTests() {
  using Kind = unifi::ForwardKind;
  CHECK(unifi::allowForward(Kind::GroupText, true, true, true, 0, 3));
  CHECK(unifi::allowForward(Kind::GroupText, true, true, true, 2, 3));
  CHECK(!unifi::allowForward(Kind::GroupText, true, true, true, 3, 3));
  CHECK(!unifi::allowForward(Kind::GroupText, true, false, true, 0, 3));
  CHECK(!unifi::allowForward(Kind::GroupText, false, true, true, 0, 3));
  for (Kind kind : {Kind::CompanionAdvert, Kind::DirectText, Kind::Ack, Kind::Path}) {
    CHECK(unifi::allowForward(kind, true, false, true, 2, 3));
    CHECK(!unifi::allowForward(kind, true, false, true, 3, 3));
    CHECK(unifi::allowForward(kind, true, false, false, 20, 3)); // Direct paths are consumed per hop.
  }
  CHECK(!unifi::allowForward(Kind::Other, true, true, true, 0, 3));
  CHECK(!unifi::allowForward(Kind::Ack, true, true, true, 0, 0));
  CHECK(unifi::minimumCommandLength(3) == 8);
  CHECK(unifi::minimumCommandLength(9) == 136);
  CHECK(unifi::minimumCommandLength(32) == 50);
  CHECK(unifi::minimumCommandLength(0x70) == 1);
  for (int command : {21, 23, 24, 25, 26, 27, 28, 29, 33, 34, 35, 36, 39, 40, 41, 42, 43, 50, 52, 54, 55, 57, 62, 63, 64, 65})
    CHECK(unifi::minimumCommandLength(command) == 0);
  CHECK(unifi::validName("Alice", 5));
  CHECK(!unifi::validName("Alice: Bob", 10));
  CHECK(!unifi::validName("", 0));
  CHECK(!unifi::validName(std::string(32, 'a').data(), 32));
  CHECK(!unifi::canSendGroup(false, true, 5, 100));
  CHECK(!unifi::canSendGroup(true, false, 5, 100));
  CHECK(unifi::canSendGroup(true, true, 31, 127));
  CHECK(!unifi::canSendGroup(true, true, 31, 128));
  CHECK(!unifi::canSendGroup(true, true, 0, 100));
}

static void statusAndWireBudgetTests() {
  unifi::EmergencyState state;
  uint8_t status[unifi::kStatusSize];
  unifi::encodeStatus(status, false, state, false, false, 0);
  CHECK(sizeof(status) == 101 && status[0] == 0x70 && status[1] == 1 && status[2] == 0);
  for (size_t i = 3; i < sizeof(status); ++i) CHECK(status[i] == 0);
  const uint8_t self[6] = {1, 2, 3, 4, 5, 6};
  const std::string id(32, 'x');
  state.sent(event("sos", id, "010203040506"), 0);
  state.receive(event("medical", "pending"), self, 1);
  unifi::encodeStatus(status, true, state, false, true, 2);
  CHECK(status[2] == (1 | 2 | 8 | 32) && status[3] == 0 && status[100] == 2);
  CHECK(std::string(reinterpret_cast<char*>(status + 4), 32) == id);
  CHECK(std::string(reinterpret_cast<char*>(status + 36)) == "pending");
  CHECK(std::string(reinterpret_cast<char*>(status + 68)) == "Alice");
  state.receive(event("ack", "ack1", "aabbccddeeff", ";ack=" + id), self, 2);
  unifi::encodeStatus(status, true, state, true, true, 2);
  CHECK(status[2] == (1 | 4 | 8 | 16 | 32));
  // Maximum supported IDs still fit with the longest stored username. This is
  // the exact metadata grammar sent by MyMesh; no truncation is needed.
  const std::string ack = "Received [mc:v1;type=ack;id=" + id + ";node=aabbccddeeff;ack=" + id + "]";
  CHECK(ack.size() + 31 + 2 <= unifi::kMaxText);
  unifi::Envelope parsed;
  CHECK(unifi::parse(ack.data(), ack.size(), parsed, false));
  const std::string gps = "Available [mc:v1;type=presence;id=" + id + ";node=aabbccddeeff;lat=-90.000000;lon=-180.000000]";
  CHECK(gps.size() + 31 + 2 <= unifi::kMaxText);
  CHECK(unifi::parse(gps.data(), gps.size(), parsed, false));
  char first[29], second[29], reboot[29];
  CHECK(unifi::makeEventId(first, sizeof(first), self, 0x12345678, 1));
  CHECK(unifi::makeEventId(second, sizeof(second), self, 0x12345678, 2));
  CHECK(unifi::makeEventId(reboot, sizeof(reboot), self, 0x87654321, 1));
  CHECK(std::string(first) == "0102030405061234567800000001");
  CHECK(std::string(first) != second && std::string(first) != reboot);
  CHECK(!unifi::makeEventId(first, 28, self, 1, 1));
  CHECK(parses(message("sos", reboot, "010203040506")));
}

static void boundedInputStressTest() {
  uint32_t random = 12345;
  for (size_t length = 1; length <= 161; ++length) {
    std::vector<char> bytes(length);
    for (int sample = 0; sample < 25; ++sample) {
      for (char& value : bytes) { random = random * 1664525u + 1013904223u; value = static_cast<char>(random >> 24); }
      unifi::Envelope output;
      CHECK(!unifi::parse(bytes.data(), bytes.size(), output));
    }
  }
}

int main() {
  parserTests();
  presenceTests();
  emergencyTests();
  queueTests();
  forwardingAndCommandTests();
  statusAndWireBudgetTests();
  boundedInputStressTest();
  std::printf("Uni-Fi protocol: 7 test groups passed (%u assertions)\n", assertions);
}
