import 'dart:async';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../connector/meshcore_connector.dart';
import '../models/channel.dart';
import '../models/channel_message.dart';
import '../models/emergency_message.dart';
import '../models/unifi_network.dart';
import '../utils/dialog_utils.dart';
import '../utils/disconnect_navigation_mixin.dart';
import '../utils/route_transitions.dart';
import '../widgets/app_bar.dart';
import '../widgets/quick_switch_bar.dart';
import 'channels_screen.dart';
import 'contacts_screen.dart';
import 'map_screen.dart';
import 'settings_screen.dart';

class EmergencyScreen extends StatefulWidget {
  final bool hideBackButton;

  const EmergencyScreen({super.key, this.hideBackButton = false});

  @override
  State<EmergencyScreen> createState() => _EmergencyScreenState();
}

class _EmergencyScreenState extends State<EmergencyScreen>
    with DisconnectNavigationMixin {
  bool _isSending = false;

  @override
  Widget build(BuildContext context) {
    final connector = context.watch<MeshCoreConnector>();
    if (!checkConnectionAndNavigate(connector)) {
      return const SizedBox.shrink();
    }

    final unifiChannel = UniFiNetwork.findChannel(connector.channels);
    final incidents = unifiChannel == null
        ? const <_IncomingIncident>[]
        : _activeIncidents(connector, unifiChannel);
    final confirmation = unifiChannel == null
        ? null
        : _latestConfirmation(connector, unifiChannel);
    final gpsEnabled = connector.currentCustomVars?['gps'] == '1';
    final hasLocation = _hasLocation(connector);

    return PopScope(
      canPop: !connector.isConnected,
      child: Scaffold(
        appBar: AppBar(
          automaticallyImplyLeading: false,
          title: const AppBarTitle('Emergency'),
          actions: [
            IconButton(
              tooltip: 'Settings',
              icon: const Icon(Icons.settings_outlined),
              onPressed: () => Navigator.push(
                context,
                MaterialPageRoute(builder: (_) => const SettingsScreen()),
              ),
            ),
            PopupMenuButton<String>(
              onSelected: (value) {
                if (value == 'disconnect') {
                  unawaited(showDisconnectDialog(context, connector));
                }
              },
              itemBuilder: (_) => const [
                PopupMenuItem(
                  value: 'disconnect',
                  child: Text('Disconnect'),
                ),
              ],
            ),
          ],
        ),
        body: ListView(
          padding: const EdgeInsets.fromLTRB(16, 12, 16, 112),
          children: [
            _DeviceStatusCard(
              nodeName: connector.selfName ?? 'Unnamed device',
              batteryPercent: connector.batteryPercent,
              gpsEnabled: gpsEnabled,
              hasLocation: hasLocation,
              networkReady: unifiChannel != null,
              onEnableGps: gpsEnabled
                  ? null
                  : () => _enableGps(connector),
            ),
            if (confirmation != null) ...[
              const SizedBox(height: 12),
              Card(
                color: Theme.of(context).colorScheme.primaryContainer,
                child: ListTile(
                  leading: Icon(
                    Icons.mark_chat_read_outlined,
                    color: Theme.of(context).colorScheme.onPrimaryContainer,
                  ),
                  title: const Text('Response received'),
                  subtitle: Text(
                    '${confirmation.message.senderName} confirmed that help is responding.',
                  ),
                ),
              ),
            ],
            const SizedBox(height: 20),
            SizedBox(
              height: 96,
              child: FilledButton.icon(
                style: FilledButton.styleFrom(
                  backgroundColor: Theme.of(context).colorScheme.error,
                  foregroundColor: Theme.of(context).colorScheme.onError,
                  textStyle: Theme.of(context).textTheme.titleLarge?.copyWith(
                    fontWeight: FontWeight.w800,
                  ),
                ),
                onPressed: _isSending || unifiChannel == null
                    ? null
                    : () => _send(
                        connector,
                        unifiChannel,
                        EmergencyMessageType.distress,
                      ),
                icon: const Icon(Icons.sos, size: 34),
                label: const Text('SEND SOS'),
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Floods an encrypted alert to provisioned Uni-Fi devices. Your device name and GPS location are included when available.',
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                color: Theme.of(context).colorScheme.onSurfaceVariant,
              ),
            ),
            const SizedBox(height: 24),
            Text(
              'Quick messages',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 10),
            GridView.count(
              crossAxisCount: MediaQuery.sizeOf(context).width >= 620 ? 4 : 2,
              mainAxisSpacing: 10,
              crossAxisSpacing: 10,
              childAspectRatio: 1.65,
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              children: [
                _QuickMessageButton(
                  icon: Icons.medical_services_outlined,
                  label: 'Medical help',
                  onPressed: _action(
                    connector,
                    unifiChannel,
                    EmergencyMessageType.medical,
                  ),
                ),
                _QuickMessageButton(
                  icon: Icons.directions_car_outlined,
                  label: 'Need pickup',
                  onPressed: _action(
                    connector,
                    unifiChannel,
                    EmergencyMessageType.pickup,
                  ),
                ),
                _QuickMessageButton(
                  icon: Icons.health_and_safety_outlined,
                  label: 'I am safe',
                  onPressed: _action(
                    connector,
                    unifiChannel,
                    EmergencyMessageType.safe,
                  ),
                ),
                _QuickMessageButton(
                  icon: Icons.directions_run,
                  label: 'On my way',
                  onPressed: _action(
                    connector,
                    unifiChannel,
                    EmergencyMessageType.enRoute,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 10),
            OutlinedButton.icon(
              onPressed:
                  _isSending || unifiChannel == null || !hasLocation
                  ? null
                  : () => _send(
                      connector,
                      unifiChannel,
                      EmergencyMessageType.location,
                      requireLocation: true,
                    ),
              icon: const Icon(Icons.my_location),
              label: Text(
                hasLocation ? 'Share my GPS location' : 'Waiting for GPS fix',
              ),
            ),
            if (incidents.isNotEmpty) ...[
              const SizedBox(height: 24),
              Text(
                'Needs a response',
                style: Theme.of(context).textTheme.titleMedium,
              ),
              const SizedBox(height: 8),
              for (final incident in incidents)
                Card(
                  child: ListTile(
                    leading: Icon(
                      Icons.warning_amber_rounded,
                      color: Theme.of(context).colorScheme.error,
                    ),
                    title: Text(incident.message.senderName),
                    subtitle: Text(incident.emergency.type.displayText),
                    trailing: FilledButton(
                      onPressed: _isSending
                          ? null
                          : () => _acknowledge(
                              connector,
                              unifiChannel!,
                              incident.emergency.eventId,
                            ),
                      child: const Text('Respond'),
                    ),
                  ),
                ),
            ],
          ],
        ),
        bottomNavigationBar: SafeArea(
          top: false,
          child: QuickSwitchBar(
            selectedIndex: 0,
            onDestinationSelected: (index) =>
                _handleQuickSwitch(index, context),
            contactsUnreadCount: connector.getTotalContactsUnreadCount(),
            channelsUnreadCount: connector.getTotalChannelsUnreadCount(),
          ),
        ),
      ),
    );
  }

  VoidCallback? _action(
    MeshCoreConnector connector,
    Channel? channel,
    EmergencyMessageType type,
  ) {
    if (_isSending || channel == null) return null;
    return () => _send(connector, channel, type);
  }

  bool _hasLocation(MeshCoreConnector connector) {
    final lat = connector.selfLatitude;
    final lon = connector.selfLongitude;
    return lat != null &&
        lon != null &&
        lat >= -90 &&
        lat <= 90 &&
        lon >= -180 &&
        lon <= 180 &&
        (lat != 0 || lon != 0);
  }

  String _eventId(MeshCoreConnector connector) {
    final key = connector.selfPublicKeyHex;
    final prefix = key.length >= 12 ? key.substring(0, 12) : 'node';
    final time = DateTime.now().millisecondsSinceEpoch.toRadixString(36);
    return '$prefix$time';
  }

  String? _nodeId(MeshCoreConnector connector) {
    final key = connector.selfPublicKeyHex.toLowerCase();
    return key.length >= 12 ? key.substring(0, 12) : null;
  }

  Future<void> _send(
    MeshCoreConnector connector,
    Channel channel,
    EmergencyMessageType type, {
    bool requireLocation = false,
  }) async {
    if (_isSending) return;
    final hasLocation = _hasLocation(connector);
    if (requireLocation && !hasLocation) return;

    setState(() => _isSending = true);
    final message = EmergencyMessage(
      type: type,
      eventId: _eventId(connector),
      nodeId: _nodeId(connector),
      latitude: hasLocation ? connector.selfLatitude : null,
      longitude: hasLocation ? connector.selfLongitude : null,
    );
    try {
      await connector.sendChannelMessage(channel, message.encode());
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('${type.displayText} sent')),
      );
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Message failed: $error')),
      );
    } finally {
      if (mounted) setState(() => _isSending = false);
    }
  }

  Future<void> _acknowledge(
    MeshCoreConnector connector,
    Channel channel,
    String eventId,
  ) async {
    if (_isSending) return;
    setState(() => _isSending = true);
    final hasLocation = _hasLocation(connector);
    final acknowledgement = EmergencyMessage(
      type: EmergencyMessageType.acknowledgement,
      eventId: _eventId(connector),
      nodeId: _nodeId(connector),
      acknowledgesEventId: eventId,
      latitude: hasLocation ? connector.selfLatitude : null,
      longitude: hasLocation ? connector.selfLongitude : null,
    );
    try {
      await connector.sendChannelMessage(channel, acknowledgement.encode());
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Response confirmation sent')),
      );
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Confirmation failed: $error')),
      );
    } finally {
      if (mounted) setState(() => _isSending = false);
    }
  }

  List<_IncomingIncident> _activeIncidents(
    MeshCoreConnector connector,
    Channel channel,
  ) {
    final messages = connector.getChannelMessagesIncludingPresence(channel);
    final cutoff = DateTime.now().subtract(UniFiNetwork.presenceTtl);
    final acknowledged = <String>{};
    for (final message in messages) {
      if (message.receivedAt.isBefore(cutoff)) continue;
      final emergency = EmergencyMessage.tryDecode(message.text);
      if (emergency != null &&
          emergency.type == EmergencyMessageType.acknowledgement &&
          emergency.acknowledgesEventId != null) {
        acknowledged.add(emergency.acknowledgesEventId!);
      }
    }

    final incidents = <_IncomingIncident>[];
    for (final message in messages.reversed) {
      if (message.isOutgoing || message.receivedAt.isBefore(cutoff)) {
        continue;
      }
      final emergency = EmergencyMessage.tryDecode(message.text);
      if (emergency == null || acknowledged.contains(emergency.eventId)) {
        continue;
      }
      if (emergency.type != EmergencyMessageType.distress &&
          emergency.type != EmergencyMessageType.medical &&
          emergency.type != EmergencyMessageType.pickup) {
        continue;
      }
      incidents.add(_IncomingIncident(message, emergency));
      if (incidents.length == 3) break;
    }
    return incidents;
  }

  _ResponseConfirmation? _latestConfirmation(
    MeshCoreConnector connector,
    Channel channel,
  ) {
    final messages = connector.getChannelMessagesIncludingPresence(channel);
    final cutoff = DateTime.now().subtract(UniFiNetwork.presenceTtl);
    final ownEventIds = <String>{};
    for (final message in messages) {
      if (!message.isOutgoing || message.receivedAt.isBefore(cutoff)) {
        continue;
      }
      final emergency = EmergencyMessage.tryDecode(message.text);
      if (emergency != null &&
          emergency.type != EmergencyMessageType.acknowledgement) {
        ownEventIds.add(emergency.eventId);
      }
    }
    for (final message in messages.reversed) {
      if (message.isOutgoing || message.receivedAt.isBefore(cutoff)) {
        continue;
      }
      final emergency = EmergencyMessage.tryDecode(message.text);
      if (emergency != null &&
          emergency.type == EmergencyMessageType.acknowledgement &&
          ownEventIds.contains(emergency.acknowledgesEventId)) {
        return _ResponseConfirmation(message);
      }
    }
    return null;
  }

  Future<void> _enableGps(MeshCoreConnector connector) async {
    try {
      await connector.setCustomVar('gps:1');
      await connector.refreshDeviceInfo();
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('GPS enabled; waiting for a fix')),
      );
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Could not enable GPS: $error')),
      );
    }
  }

  void _handleQuickSwitch(int index, BuildContext context) {
    if (index == 0) return;
    final Widget destination = switch (index) {
      1 => const ContactsScreen(hideBackButton: true),
      2 => const ChannelsScreen(hideBackButton: true),
      _ => const MapScreen(hideBackButton: true),
    };
    Navigator.pushReplacement(context, buildQuickSwitchRoute(destination));
  }
}

class _IncomingIncident {
  final ChannelMessage message;
  final EmergencyMessage emergency;

  const _IncomingIncident(this.message, this.emergency);
}

class _ResponseConfirmation {
  final ChannelMessage message;

  const _ResponseConfirmation(this.message);
}

class _QuickMessageButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback? onPressed;

  const _QuickMessageButton({
    required this.icon,
    required this.label,
    required this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    return OutlinedButton(
      onPressed: onPressed,
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(icon),
          const SizedBox(height: 6),
          Text(label, textAlign: TextAlign.center),
        ],
      ),
    );
  }
}

class _DeviceStatusCard extends StatelessWidget {
  final String nodeName;
  final int? batteryPercent;
  final bool gpsEnabled;
  final bool hasLocation;
  final bool networkReady;
  final VoidCallback? onEnableGps;

  const _DeviceStatusCard({
    required this.nodeName,
    required this.batteryPercent,
    required this.gpsEnabled,
    required this.hasLocation,
    required this.networkReady,
    required this.onEnableGps,
  });

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(14),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              nodeName,
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.w700,
              ),
            ),
            const SizedBox(height: 10),
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: [
                Chip(
                  avatar: const Icon(Icons.bluetooth_connected, size: 18),
                  label: const Text('Connected'),
                ),
                Chip(
                  avatar: const Icon(Icons.battery_5_bar, size: 18),
                  label: Text(
                    batteryPercent == null ? 'Battery --' : '$batteryPercent%',
                  ),
                ),
                Chip(
                  avatar: Icon(
                    hasLocation ? Icons.gps_fixed : Icons.gps_not_fixed,
                    size: 18,
                  ),
                  label: Text(
                    hasLocation
                        ? 'GPS ready'
                        : gpsEnabled
                        ? 'GPS searching'
                        : 'GPS off',
                  ),
                ),
                Chip(
                  avatar: Icon(
                    networkReady ? Icons.lock : Icons.no_encryption_outlined,
                    size: 18,
                  ),
                  label: Text(
                    networkReady ? 'Uni-Fi ready' : 'Not provisioned',
                  ),
                ),
              ],
            ),
            if (onEnableGps != null) ...[
              const SizedBox(height: 8),
              Wrap(
                spacing: 8,
                children: [
                  if (onEnableGps != null)
                    TextButton.icon(
                      onPressed: onEnableGps,
                      icon: const Icon(Icons.gps_fixed),
                      label: const Text('Enable GPS'),
                    ),
                ],
              ),
            ],
            if (!networkReady)
              Text(
                'Quick messages are disabled until this radio is flashed with a provisioned Uni-Fi network key.',
                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: scheme.error,
                ),
              ),
          ],
        ),
      ),
    );
  }
}
