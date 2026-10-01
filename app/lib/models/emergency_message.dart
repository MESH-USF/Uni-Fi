enum EmergencyMessageType {
  presence('presence', 'Available'),
  distress('sos', 'SOS - immediate assistance needed'),
  medical('medical', 'Medical help needed'),
  pickup('pickup', 'Pickup needed'),
  safe('safe', 'I am safe'),
  enRoute('enroute', 'I am on my way'),
  location('location', 'Sharing my location'),
  acknowledgement('ack', 'Received - help is responding');

  final String wireName;
  final String displayText;

  const EmergencyMessageType(this.wireName, this.displayText);

  static EmergencyMessageType? fromWireName(String value) {
    for (final type in values) {
      if (type.wireName == value) return type;
    }
    return null;
  }
}

/// A compact, human-readable envelope carried inside a normal MeshCore
/// channel message. Other MeshCore clients still display the useful text,
/// while this app can recover the event type, location, and acknowledgement.
class EmergencyMessage {
  static const String _marker = '[mc:v1;';

  final EmergencyMessageType type;
  final String eventId;
  final String? nodeId;
  final double? latitude;
  final double? longitude;
  final String? acknowledgesEventId;

  const EmergencyMessage({
    required this.type,
    required this.eventId,
    this.nodeId,
    this.latitude,
    this.longitude,
    this.acknowledgesEventId,
  });

  bool get hasLocation =>
      latitude != null &&
      longitude != null &&
      latitude! >= -90 &&
      latitude! <= 90 &&
      longitude! >= -180 &&
      longitude! <= 180 &&
      (latitude != 0 || longitude != 0);

  String encode() {
    final fields = <String>[
      'type=${type.wireName}',
      'id=$eventId',
      if (nodeId != null) 'node=$nodeId',
      if (hasLocation) 'lat=${latitude!.toStringAsFixed(6)}',
      if (hasLocation) 'lon=${longitude!.toStringAsFixed(6)}',
      if (acknowledgesEventId != null) 'ack=$acknowledgesEventId',
    ];
    return '${type.displayText} [mc:v1;${fields.join(';')}]';
  }

  static EmergencyMessage? tryDecode(String text) {
    final start = text.lastIndexOf(_marker);
    if (start < 0 || !text.endsWith(']')) return null;

    final metadata = text.substring(start + _marker.length, text.length - 1);
    final fields = <String, String>{};
    for (final part in metadata.split(';')) {
      final separator = part.indexOf('=');
      if (separator <= 0 || separator == part.length - 1) return null;
      fields[part.substring(0, separator)] = part.substring(separator + 1);
    }

    final type = EmergencyMessageType.fromWireName(fields['type'] ?? '');
    final eventId = fields['id'];
    if (type == null || eventId == null || eventId.isEmpty) return null;

    final nodeId = fields['node'];
    if (nodeId != null && !RegExp(r'^[0-9a-fA-F]{12}$').hasMatch(nodeId)) {
      return null;
    }

    final hasLat = fields.containsKey('lat');
    final hasLon = fields.containsKey('lon');
    if (hasLat != hasLon) return null;
    final latitude = hasLat ? double.tryParse(fields['lat']!) : null;
    final longitude = hasLon ? double.tryParse(fields['lon']!) : null;
    if (hasLat && (latitude == null || longitude == null)) return null;

    final message = EmergencyMessage(
      type: type,
      eventId: eventId,
      nodeId: nodeId?.toLowerCase(),
      latitude: latitude,
      longitude: longitude,
      acknowledgesEventId: fields['ack'],
    );
    if ((hasLat || hasLon) && !message.hasLocation) return null;
    if (type == EmergencyMessageType.acknowledgement &&
        (message.acknowledgesEventId == null ||
            message.acknowledgesEventId!.isEmpty)) {
      return null;
    }
    return message;
  }
}
