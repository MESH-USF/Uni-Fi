import 'package:flutter_test/flutter_test.dart';
import 'package:meshcore_open/models/emergency_message.dart';

void main() {
  group('EmergencyMessage', () {
    test('round trips a distress event with location', () {
      const message = EmergencyMessage(
        type: EmergencyMessageType.distress,
        eventId: 'a1event',
        nodeId: 'aabbccddeeff',
        latitude: 40.7128,
        longitude: -74.0060,
      );

      final encoded = message.encode();
      final decoded = EmergencyMessage.tryDecode(encoded);

      expect(encoded, startsWith('SOS - immediate assistance needed'));
      expect(decoded?.type, EmergencyMessageType.distress);
      expect(decoded?.eventId, 'a1event');
      expect(decoded?.nodeId, 'aabbccddeeff');
      expect(decoded?.latitude, closeTo(40.7128, 0.000001));
      expect(decoded?.longitude, closeTo(-74.0060, 0.000001));
    });

    test('round trips an acknowledgement', () {
      const message = EmergencyMessage(
        type: EmergencyMessageType.acknowledgement,
        eventId: 'responder1',
        acknowledgesEventId: 'victim1',
      );

      final decoded = EmergencyMessage.tryDecode(message.encode());

      expect(decoded?.type, EmergencyMessageType.acknowledgement);
      expect(decoded?.acknowledgesEventId, 'victim1');
    });

    test('keeps ordinary channel messages untouched', () {
      expect(EmergencyMessage.tryDecode('Meet at the trailhead'), isNull);
    });

    test('round trips an authorized presence heartbeat', () {
      const message = EmergencyMessage(
        type: EmergencyMessageType.presence,
        eventId: 'heartbeat1',
        nodeId: '001122aabbcc',
      );

      final decoded = EmergencyMessage.tryDecode(message.encode());

      expect(decoded?.type, EmergencyMessageType.presence);
      expect(decoded?.nodeId, '001122aabbcc');
    });

    test('rejects malformed node identifiers', () {
      expect(
        EmergencyMessage.tryDecode(
          'Available [mc:v1;type=presence;id=heartbeat1;node=not-a-key]',
        ),
        isNull,
      );
    });

    test('rejects partial and invalid coordinates', () {
      expect(
        EmergencyMessage.tryDecode(
          'SOS [mc:v1;type=sos;id=event1;lat=40.000000]',
        ),
        isNull,
      );
      expect(
        EmergencyMessage.tryDecode(
          'SOS [mc:v1;type=sos;id=event1;lat=95.000000;lon=10.000000]',
        ),
        isNull,
      );
    });

    test('rejects an acknowledgement without a target event', () {
      expect(
        EmergencyMessage.tryDecode(
          'Received [mc:v1;type=ack;id=responder1]',
        ),
        isNull,
      );
    });
  });
}
