import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:meshcore_open/models/channel.dart';
import 'package:meshcore_open/models/channel_message.dart';
import 'package:meshcore_open/models/contact.dart';
import 'package:meshcore_open/models/emergency_message.dart';
import 'package:meshcore_open/models/unifi_network.dart';

void main() {
  test('accepts only the provisioned slot-zero Uni-Fi channel', () {
    final channels = [
      Channel(index: 1, name: 'Uni-Fi', psk: Uint8List(16)),
      Channel(index: 0, name: 'Uni-Fi', psk: Uint8List(16)),
    ];

    expect(UniFiNetwork.findChannel(channels)?.index, 0);
    expect(UniFiNetwork.findChannel(channels.take(1)), isNull);
  });

  ChannelMessage presence({
    required DateTime receivedAt,
    required String nodeId,
    String senderName = 'Ranger 1',
    double? latitude,
    double? longitude,
  }) {
    return ChannelMessage(
      senderName: senderName,
      text: EmergencyMessage(
        type: EmergencyMessageType.presence,
        eventId: receivedAt.millisecondsSinceEpoch.toString(),
        nodeId: nodeId,
        latitude: latitude,
        longitude: longitude,
      ).encode(),
      timestamp: receivedAt,
      receivedAt: receivedAt,
      isOutgoing: false,
    );
  }

  test('keeps only the latest four-hour presence for each node', () {
    final now = DateTime(2026, 9, 28, 12);
    final records = UniFiNetwork.collectPresence(
      [
        presence(
          receivedAt: now.subtract(const Duration(hours: 5)),
          nodeId: '001122334455',
        ),
        presence(
          receivedAt: now.subtract(const Duration(minutes: 30)),
          nodeId: 'aabbccddeeff',
          senderName: 'Old name',
        ),
        presence(
          receivedAt: now.subtract(const Duration(minutes: 5)),
          nodeId: 'aabbccddeeff',
          senderName: 'Ranger 2',
          latitude: 28.0630,
          longitude: -82.4139,
        ),
      ],
      now: now,
    );

    expect(records.keys, ['aabbccddeeff']);
    expect(records['aabbccddeeff']?.name, 'Ranger 2');
    expect(records['aabbccddeeff']?.latitude, closeTo(28.0630, 0.000001));
  });

  test('authorizes a contact only when its public key matches presence', () {
    final contact = Contact(
      publicKey: Uint8List.fromList([
        0xaa,
        0xbb,
        0xcc,
        0xdd,
        0xee,
        0xff,
        ...List<int>.filled(26, 0),
      ]),
      name: 'Untrusted advert name',
      type: 1,
      pathLength: -1,
      path: Uint8List(0),
      lastSeen: DateTime(2026, 9, 28),
    );
    final record = UniFiPresenceRecord(
      nodeId: 'aabbccddeeff',
      name: 'Authorized name',
      latitude: null,
      longitude: null,
      lastReceived: DateTime(2026, 9, 28),
      lastEvent: EmergencyMessageType.presence,
    );

    expect(
      UniFiNetwork.isAuthorizedContact(contact, {'aabbccddeeff': record}),
      isTrue,
    );
    expect(UniFiNetwork.isAuthorizedContact(contact, const {}), isFalse);
  });
}
