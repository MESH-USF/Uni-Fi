import 'channel.dart';
import 'channel_message.dart';
import 'contact.dart';
import 'emergency_message.dart';

/// Product-level rules layered on top of MeshCore's encrypted group channel.
abstract final class UniFiNetwork {
  static const String channelName = 'Uni-Fi';
  static const Duration presenceTtl = Duration(hours: 4);

  static Channel? findChannel(Iterable<Channel> channels) {
    for (final channel in channels) {
      if (channel.index == 0 && channel.name.trim() == channelName) {
        return channel;
      }
    }
    return null;
  }

  /// Returns one current record per device. Callers only pass messages from
  /// the provisioned Uni-Fi channel, so public advertisements cannot authorize
  /// a node for the product UI.
  static Map<String, UniFiPresenceRecord> collectPresence(
    Iterable<ChannelMessage> messages, {
    DateTime? now,
  }) {
    final cutoff = (now ?? DateTime.now()).subtract(presenceTtl);
    final records = <String, UniFiPresenceRecord>{};

    for (final message in messages) {
      if (message.isOutgoing || message.receivedAt.isBefore(cutoff)) continue;
      final envelope = EmergencyMessage.tryDecode(message.text);
      final nodeId = envelope?.nodeId;
      if (envelope == null || nodeId == null) continue;

      final previous = records[nodeId];
      if (previous != null && previous.lastReceived.isAfter(message.receivedAt)) {
        continue;
      }
      records[nodeId] = UniFiPresenceRecord(
        nodeId: nodeId,
        name: message.senderName,
        latitude: envelope.hasLocation ? envelope.latitude : previous?.latitude,
        longitude: envelope.hasLocation
            ? envelope.longitude
            : previous?.longitude,
        lastReceived: message.receivedAt,
        lastEvent: envelope.type,
      );
    }
    return records;
  }

  static bool isAuthorizedContact(
    Contact contact,
    Map<String, UniFiPresenceRecord> presence,
  ) {
    return presenceForContact(contact, presence) != null;
  }

  static UniFiPresenceRecord? presenceForContact(
    Contact contact,
    Map<String, UniFiPresenceRecord> presence,
  ) {
    final publicKey = contact.publicKeyHex.toLowerCase();
    for (final entry in presence.entries) {
      if (publicKey.startsWith(entry.key)) return entry.value;
    }
    return null;
  }
}

class UniFiPresenceRecord {
  final String nodeId;
  final String name;
  final double? latitude;
  final double? longitude;
  final DateTime lastReceived;
  final EmergencyMessageType lastEvent;

  const UniFiPresenceRecord({
    required this.nodeId,
    required this.name,
    required this.latitude,
    required this.longitude,
    required this.lastReceived,
    required this.lastEvent,
  });

  bool get hasLocation => latitude != null && longitude != null;
}
