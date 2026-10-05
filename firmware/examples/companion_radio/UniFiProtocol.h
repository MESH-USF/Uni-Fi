#pragma once

// Bounded, allocation-free Uni-Fi envelope and local state. This header is also
// compiled by native tests: no Arduino, radio, or wall clock dependencies.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

namespace unifi {
constexpr size_t kMaxText = 160;
constexpr size_t kMaxId = 32;
constexpr uint32_t kPresenceTtl = 4UL * 60UL * 60UL * 1000UL;
enum class Type { Presence, Sos, Medical, Pickup, Safe, EnRoute, Location, Ack };
enum class ForwardKind { GroupText, CompanionAdvert, DirectText, Ack, Path, Other };

inline bool allowForward(ForwardKind kind, bool provisioned, bool matching_channel,
                         bool flood, unsigned hops, unsigned maximum_hops) {
  if (!provisioned || maximum_hops == 0 || (flood && hops >= maximum_hops)) return false;
  switch (kind) {
    case ForwardKind::GroupText: return matching_channel;
    case ForwardKind::CompanionAdvert:
    case ForwardKind::DirectText:
    case ForwardKind::Ack:
    case ForwardKind::Path: return true;
    default: return false;
  }
}

// The retained subset of MeshCore companion commands, with minimum lengths.
// Unknown/pruned commands return zero; callers reject them before inspecting
// payload fields. This keeps malformed short frames out of the dispatcher.
inline size_t minimumCommandLength(uint8_t command, size_t maximum_path = 64) {
  switch (command) {
    case 1: return 8; // APP_START
    case 2: return 14; // direct text: header, time, pubkey prefix, text
    case 3: return 8; // group text: header, time, text
    case 6: return 5; // set clock
    case 8: return 2; // set name
    case 9: return 1 + 32 + 3 + maximum_path + 32 + 4; // complete contact
    case 11: return 11; // radio params (product replies disabled)
    case 12: return 2; // power
    case 13: case 15: case 16: case 30: return 33; // contact key
    case 14: return 9; // reserved latitude/longitude support
    case 18: return 99; // import signed advert/contact
    case 19: return 7; // reboot confirmation
    case 22: case 31: case 38: case 56: case 58: return 2;
    case 32: return 50; // channel index/name/128-bit secret
    case 37: return 5; // BLE PIN
    case 51: return 6; // reset confirmation
    case 61: return 3; // path-hash mode
    case 4: case 5: case 7: case 10: case 17: case 20:
    case 59: case 60: case 0x70: return 1;
    default: return 0;
  }
}

inline bool isDistress(Type type) {
  return type == Type::Sos || type == Type::Medical || type == Type::Pickup;
}

inline bool canSendGroup(bool provisioned, bool configured_key, size_t name_length, size_t text_length) {
  return provisioned && configured_key && name_length > 0 && name_length <= 31 &&
      text_length > 0 && name_length + 2 + text_length <= kMaxText;
}

inline bool validName(const char* name, size_t length) {
  if (!name || length == 0 || length > 31) return false;
  for (size_t i = 0; i < length; ++i) {
    const unsigned char c = static_cast<unsigned char>(name[i]);
    if (c < 32 || c == 127 || c == ':' || c == '[' || c == ']') return false;
  }
  return true;
}

inline bool copyId(char dest[kMaxId + 1], const char* value, size_t length) {
  if (length == 0 || length > kMaxId) return false;
  for (size_t i = 0; i < length; ++i) {
    const char c = value[i];
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z') || c == '-' || c == '_')) return false;
  }
  memcpy(dest, value, length);
  dest[length] = 0;
  return true;
}

inline int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

inline bool makeEventId(char* dest, size_t capacity, const uint8_t node[6],
                        uint32_t boot_nonce, uint32_t sequence) {
  if (!dest || !node || capacity < 29) return false;
  const char* digits = "0123456789ABCDEF";
  for (size_t i = 0; i < 6; ++i) {
    dest[2 * i] = digits[node[i] >> 4];
    dest[2 * i + 1] = digits[node[i] & 15];
  }
  const int size = snprintf(dest + 12, capacity - 12, "%08lX%08lX",
      static_cast<unsigned long>(boot_nonce), static_cast<unsigned long>(sequence));
  return size == 16;
}

struct Envelope {
  Type type;
  char id[kMaxId + 1];
  char ack[kMaxId + 1];
  uint8_t node[6];
  char name[32];
  int32_t latitude_e6;
  int32_t longitude_e6;
  bool has_location;
};

inline bool equals(const char* value, size_t length, const char* literal) {
  return strlen(literal) == length && memcmp(value, literal, length) == 0;
}

inline bool containsEnvelope(const char* text, size_t length) {
  for (size_t i = 0; i + 7 <= length; ++i) if (memcmp(text + i, "[mc:v1;", 7) == 0) return true;
  return false;
}

inline bool coordinate(const char* value, size_t length, double max, int32_t& out) {
  if (length == 0 || length > 23) return false;
  char number[24];
  memcpy(number, value, length);
  number[length] = 0;
  // Deliberately accept decimal numbers only, not NaN, infinities, hex, or
  // exponent notation. Flutter and firmware both serialize fixed decimals.
  size_t i = number[0] == '-' ? 1 : 0;
  bool digit = false, dot = false;
  for (; i < length; ++i) {
    if (number[i] == '.' && !dot) dot = true;
    else if (number[i] >= '0' && number[i] <= '9') digit = true;
    else return false;
  }
  if (!digit) return false;
  char* end;
  const double degrees = strtod(number, &end);
  if (end != number + length || !isfinite(degrees) || degrees < -max || degrees > max) return false;
  out = static_cast<int32_t>(round(degrees * 1000000.0));
  return true;
}

// Length is supplied by the caller. Never reads beyond it, even for malformed
// or non-NUL-terminated radio/client input. Name is optional for outgoing text.
inline bool parse(const char* text, size_t length, Envelope& out, bool require_name = true) {
  memset(&out, 0, sizeof(out));
  if (!text || length == 0 || length > kMaxText || memchr(text, 0, length) || text[length - 1] != ']') return false;
  const char* marker = nullptr;
  for (size_t i = 0; i + 7 <= length; ++i) {
    if (memcmp(text + i, "[mc:v1;", 7) == 0) {
      if (marker) return false;
      marker = text + i;
    }
  }
  if (!marker) return false;
  if (require_name) {
    const char* separator = nullptr;
    for (const char* p = text; p + 1 < marker; ++p) {
      if (p[0] == ':' && p[1] == ' ') { separator = p; break; }
    }
    if (!separator || !validName(text, separator - text)) return false;
    memcpy(out.name, text, separator - text);
  }
  unsigned seen = 0;
  const char* p = marker + 7;
  const char* end = text + length - 1;
  while (p < end) {
    const char* field_end = static_cast<const char*>(memchr(p, ';', end - p));
    if (!field_end) field_end = end;
    const char* eq = static_cast<const char*>(memchr(p, '=', field_end - p));
    if (!eq || eq == p || eq + 1 == field_end) return false;
    const char* value = eq + 1;
    const size_t size = field_end - value;
    unsigned bit;
    if (equals(p, eq - p, "type")) {
      bit = 1;
      if (equals(value, size, "presence")) out.type = Type::Presence;
      else if (equals(value, size, "sos")) out.type = Type::Sos;
      else if (equals(value, size, "medical")) out.type = Type::Medical;
      else if (equals(value, size, "pickup")) out.type = Type::Pickup;
      else if (equals(value, size, "safe")) out.type = Type::Safe;
      else if (equals(value, size, "enroute")) out.type = Type::EnRoute;
      else if (equals(value, size, "location")) out.type = Type::Location;
      else if (equals(value, size, "ack")) out.type = Type::Ack;
      else return false;
    } else if (equals(p, eq - p, "id")) {
      bit = 2;
      if (!copyId(out.id, value, size)) return false;
    } else if (equals(p, eq - p, "node")) {
      bit = 4;
      if (size != 12) return false;
      for (size_t i = 0; i < 6; ++i) {
        const int hi = hexNibble(value[2 * i]), lo = hexNibble(value[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.node[i] = static_cast<uint8_t>((hi << 4) | lo);
      }
    } else if (equals(p, eq - p, "lat")) {
      bit = 8;
      if (!coordinate(value, size, 90, out.latitude_e6)) return false;
    } else if (equals(p, eq - p, "lon")) {
      bit = 16;
      if (!coordinate(value, size, 180, out.longitude_e6)) return false;
    } else if (equals(p, eq - p, "ack")) {
      bit = 32;
      if (!copyId(out.ack, value, size)) return false;
    } else return false;
    if (seen & bit) return false;
    seen |= bit;
    if (field_end < end && field_end + 1 == end) return false;
    p = field_end + 1;
  }
  if ((seen & 7) != 7 || ((seen & 8) != 0) != ((seen & 16) != 0)) return false;
  if ((out.type == Type::Ack) != ((seen & 32) != 0)) return false;
  out.has_location = (seen & 24) == 24;
  return true;
}

struct Presence {
  uint8_t node_prefix[6];
  char name[32];
  int32_t latitude_e6;
  int32_t longitude_e6;
  uint32_t last_seen;
  bool has_location;
};

enum class QueueKind : uint8_t { Other, ChannelText, Presence, Critical };
struct QueueInfo { QueueKind kind; uint8_t node[6]; };

inline QueueInfo classifyFrame(const uint8_t* frame, size_t length) {
  QueueInfo info = {};
  if (!frame || !length) return info;
  size_t text_start, channel_index;
  if (frame[0] == 8) { channel_index = 1; text_start = 8; }
  else if (frame[0] == 17) { channel_index = 4; text_start = 11; }
  else return info;
  info.kind = QueueKind::ChannelText;
  if (length <= text_start || frame[channel_index] != 0) return info;
  Envelope event;
  if (parse(reinterpret_cast<const char*>(frame + text_start), length - text_start, event)) {
    memcpy(info.node, event.node, sizeof(info.node));
    if (event.type == Type::Presence) info.kind = QueueKind::Presence;
    else if (isDistress(event.type) || event.type == Type::Ack) info.kind = QueueKind::Critical;
  }
  return info;
}

// Return an entry to replace, the next free entry, or -1 to drop the incoming
// frame. Presence coalesces by node. A queue containing only emergency/direct
// messages cannot be displaced by periodic heartbeats or ordinary group text.
inline int queueSlot(const QueueInfo* queue, size_t length, size_t capacity, const QueueInfo& incoming) {
  if (!queue || !capacity || length > capacity) return -1;
  int presence = -1, ordinary = -1;
  for (size_t i = 0; i < length; ++i) {
    if (queue[i].kind == QueueKind::Presence) {
      if (incoming.kind == QueueKind::Presence && memcmp(queue[i].node, incoming.node, 6) == 0) return i;
      if (presence < 0) presence = i;
    } else if (queue[i].kind == QueueKind::ChannelText && ordinary < 0) ordinary = i;
  }
  if (length < capacity) return length;
  if (presence >= 0) return presence;
  if (ordinary >= 0) return ordinary;
  return incoming.kind == QueueKind::Critical ? 0 : -1;
}

template<size_t Capacity> class PresenceTable {
  struct Slot { Presence presence; uint32_t received; bool used; } slots[Capacity] = {};
public:
  void expire(uint32_t now) {
    for (auto& slot : slots) {
      if (slot.used && uint32_t(now - slot.received) >= kPresenceTtl) slot.used = false;
    }
  }
  void update(const Envelope& event, uint32_t now, uint32_t timestamp) {
    expire(now);
    size_t use = Capacity, oldest = 0;
    uint32_t oldest_age = 0;
    for (size_t i = 0; i < Capacity; ++i) {
      if (slots[i].used && memcmp(slots[i].presence.node_prefix, event.node, 6) == 0) { use = i; break; }
      if (!slots[i].used && use == Capacity) use = i;
      if (slots[i].used && uint32_t(now - slots[i].received) >= oldest_age) {
        oldest = i; oldest_age = uint32_t(now - slots[i].received);
      }
    }
    if (use == Capacity) use = oldest;
    Slot& slot = slots[use];
    if (!slot.used || memcmp(slot.presence.node_prefix, event.node, 6) != 0) memset(&slot, 0, sizeof(slot));
    memcpy(slot.presence.node_prefix, event.node, 6);
    memcpy(slot.presence.name, event.name, sizeof(slot.presence.name));
    // Keep a known fix when a compact ACK/SOS omits coordinates. The entry still
    // expires locally after four hours; no location is invented when GPS is off.
    if (event.has_location) {
      slot.presence.latitude_e6 = event.latitude_e6;
      slot.presence.longitude_e6 = event.longitude_e6;
      slot.presence.has_location = true;
    }
    slot.presence.last_seen = timestamp;
    slot.received = now;
    slot.used = true;
  }
  int copy(Presence* dest, int maximum, uint32_t now) {
    expire(now);
    int count = 0;
    if (!dest || maximum <= 0) return 0;
    for (auto& slot : slots) if (slot.used && count < maximum) dest[count++] = slot.presence;
    return count;
  }
  size_t count(uint32_t now) {
    expire(now);
    size_t count = 0;
    for (auto& slot : slots) if (slot.used) ++count;
    return count;
  }
  bool contains(const uint8_t node[6], uint32_t now) {
    expire(now);
    for (auto& slot : slots) if (slot.used && memcmp(slot.presence.node_prefix, node, 6) == 0) return true;
    return false;
  }
};

class EmergencyState {
  struct Seen { char id[kMaxId + 1]; uint8_t node[6]; uint32_t received; bool used; } seen[16] = {};
  size_t next = 0;
  char pending_id[kMaxId + 1] = {}, pending_name[32] = {}, own_id[kMaxId + 1] = {};
  uint8_t pending_node[6] = {};
  uint32_t pending_time = 0, own_time = 0;
  bool pending = false, own_acknowledged = false;
public:
  enum class Receive { None, Distress, OwnAcknowledged };
  void expire(uint32_t now) {
    if (pending && uint32_t(now - pending_time) >= kPresenceTtl) pending = false;
    if (own_id[0] && uint32_t(now - own_time) >= kPresenceTtl) { own_id[0] = 0; own_acknowledged = false; }
    for (auto& event : seen) if (event.used && uint32_t(now - event.received) >= kPresenceTtl) event.used = false;
  }
  Receive receive(const Envelope& event, const uint8_t self[6], uint32_t now) {
    expire(now);
    if (memcmp(event.node, self, 6) == 0 || (!isDistress(event.type) && event.type != Type::Ack)) return Receive::None;
    for (auto& prior : seen) {
      if (prior.used && strcmp(prior.id, event.id) == 0 && memcmp(prior.node, event.node, 6) == 0) {
        prior.received = now;
        if (pending && strcmp(pending_id, event.id) == 0 && memcmp(pending_node, event.node, 6) == 0) pending_time = now;
        if (event.type == Type::Ack && own_acknowledged && strcmp(event.ack, own_id) == 0) own_time = now;
        return Receive::None;
      }
    }
    Seen& recent = seen[next]; next = (next + 1) % 16;
    strcpy(recent.id, event.id); memcpy(recent.node, event.node, 6); recent.received = now; recent.used = true;
    if (isDistress(event.type)) {
      strcpy(pending_id, event.id); strcpy(pending_name, event.name);
      memcpy(pending_node, event.node, 6);
      pending_time = now; pending = true;
      return Receive::Distress;
    }
    if (own_id[0] && strcmp(event.ack, own_id) == 0) {
      const bool first_ack = !own_acknowledged;
      own_acknowledged = true;
      own_time = now;
      return first_ack ? Receive::OwnAcknowledged : Receive::None;
    }
    return Receive::None;
  }
  void sent(const Envelope& event, uint32_t now = 0) {
    if (isDistress(event.type)) { strcpy(own_id, event.id); own_time = now; own_acknowledged = false; }
    else if (event.type == Type::Ack && pending && strcmp(event.ack, pending_id) == 0) pending = false;
  }
  bool hasPending() const { return pending; }
  bool ownAcknowledged() const { return own_acknowledged; }
  const char* pendingId() const { return pending_id; }
  const char* pendingName() const { return pending_name; }
  const char* ownId() const { return own_id; }
};

constexpr size_t kStatusSize = 101;
inline void encodeStatus(uint8_t dest[kStatusSize], bool provisioned,
                         const EmergencyState& emergency, bool gps, bool relay,
                         uint8_t roster_count) {
  memset(dest, 0, kStatusSize);
  dest[0] = 0x70;
  dest[1] = 1;
  dest[2] = (provisioned ? 1 : 0) |
      (emergency.ownId()[0] && !emergency.ownAcknowledged() ? 2 : 0) |
      (emergency.ownAcknowledged() ? 4 : 0) | (emergency.hasPending() ? 8 : 0) |
      (gps ? 16 : 0) | (relay ? 32 : 0);
  dest[3] = 0; // provisioned channel slot
  memcpy(dest + 4, emergency.ownId(), strlen(emergency.ownId()));
  if (emergency.hasPending()) {
    memcpy(dest + 36, emergency.pendingId(), strlen(emergency.pendingId()));
    memcpy(dest + 68, emergency.pendingName(), strlen(emergency.pendingName()));
  }
  dest[100] = roster_count;
}
} // namespace unifi
