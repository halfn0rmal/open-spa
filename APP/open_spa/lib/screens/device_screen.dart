import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../widgets/service_tile.dart';
import '../widgets/characteristic_tile.dart';
import '../widgets/descriptor_tile.dart';
import '../utils/snackbar.dart';
import '../utils/extra.dart';

class DeviceScreen extends StatefulWidget {
  final BluetoothDevice device;

  const DeviceScreen({Key? key, required this.device}) : super(key: key);

  @override
  State<DeviceScreen> createState() => _DeviceScreenState();
}

class _DeviceScreenState extends State<DeviceScreen> {
  int? _rssi;
  int? _mtuSize;
  BluetoothConnectionState _connectionState =
      BluetoothConnectionState.disconnected;
  List<BluetoothService> _services = [];
  bool _isDiscoveringServices = false;
  bool _isConnecting = false;
  bool _isDisconnecting = false;

  late StreamSubscription<BluetoothConnectionState>
      _connectionStateSubscription;
  late StreamSubscription<bool> _isConnectingSubscription;
  late StreamSubscription<bool> _isDisconnectingSubscription;
  late StreamSubscription<int> _mtuSubscription;
  late BluetoothCharacteristic _setTempCharacteristic;
  late BluetoothCharacteristic _setModeCharacteristic;
  late int _setTemp;
  bool _isJetsOn = false;

  List<String> systemState = [
    'startup',
    'transitionToHeating',
    'idle',
    'heating',
    'transitionToJets',
    'jets',
    'fault'
  ];

  @override
  void initState() {
    super.initState();

    _connectionStateSubscription =
        widget.device.connectionState.listen((state) async {
      _connectionState = state;
      if (state == BluetoothConnectionState.connected) {
        _services = []; // must rediscover services
        onDiscoverServicesPressed();
      }
      if (state == BluetoothConnectionState.connected && _rssi == null) {
        _rssi = await widget.device.readRssi();
      }
      if (mounted) {
        setState(() {});
      }
    });

    _mtuSubscription = widget.device.mtu.listen((value) {
      _mtuSize = value;
      if (mounted) {
        setState(() {});
      }
    });

    _isConnectingSubscription = widget.device.isConnecting.listen((value) {
      _isConnecting = value;
      if (mounted) {
        setState(() {});
      }
    });

    _isDisconnectingSubscription =
        widget.device.isDisconnecting.listen((value) {
      _isDisconnecting = value;
      if (mounted) {
        setState(() {});
      }
    });
  }

  @override
  void dispose() {
    _connectionStateSubscription.cancel();
    _mtuSubscription.cancel();
    _isConnectingSubscription.cancel();
    _isDisconnectingSubscription.cancel();
    super.dispose();
  }

  bool get isConnected {
    return _connectionState == BluetoothConnectionState.connected;
  }

  Future onConnectPressed() async {
    try {
      await widget.device.connectAndUpdateStream();
      Snackbar.show(ABC.c, "Connect: Success", success: true);
    } catch (e) {
      if (e is FlutterBluePlusException &&
          e.code == FbpErrorCode.connectionCanceled.index) {
        // ignore connections canceled by the user
      } else {
        Snackbar.show(ABC.c, prettyException("Connect Error:", e),
            success: false);
      }
    }
  }

  Future onCancelPressed() async {
    try {
      await widget.device.disconnectAndUpdateStream(queue: false);
      Snackbar.show(ABC.c, "Cancel: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Cancel Error:", e), success: false);
    }
  }

  Future onDisconnectPressed() async {
    try {
      await widget.device.disconnectAndUpdateStream();
      Snackbar.show(ABC.c, "Disconnect: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Disconnect Error:", e),
          success: false);
    }
  }

  Future<List<BluetoothService>> filterServices(
      List<BluetoothService> services) async {
    return services.where((s) => s.uuid.toString().contains("00FF")).toList();
  }

  Future onDiscoverServicesPressed() async {
    if (mounted) {
      setState(() {
        _isDiscoveringServices = true;
      });
    }
    try {
      _services = await widget.device.discoverServices();
      Snackbar.show(ABC.c, "Discover Services: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Discover Services Error:", e),
          success: false);
    }
    if (mounted) {
      setState(() {
        _isDiscoveringServices = false;
      });
    }
  }

  Future onRequestMtuPressed() async {
    try {
      await widget.device.requestMtu(223, predelay: 0);
      Snackbar.show(ABC.c, "Request Mtu: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Change Mtu Error:", e),
          success: false);
    }
  }

  Future<Widget> _buildTempTile(BuildContext context, BluetoothDevice d) async {
    // the service is 0x00FF and the characteristic is 0xFF01
    if (d.servicesList.length < 3) {
      return Card(
        shape: CircleBorder(side: BorderSide(color: Colors.grey[300]!)),
        child: Center(
          child: CircularProgressIndicator(
            backgroundColor: Colors.grey[100]!,
            valueColor: AlwaysStoppedAnimation(Colors.blue[300]!),
          ),
        ),
      );
    }
    List<int> temperature = await d.servicesList[2].characteristics[0].read();
    return ConstrainedBox(
      constraints: BoxConstraints(minWidth: 200, minHeight: 200),
      child: Card(
        color: Colors.grey[100]!,
        clipBehavior: Clip.antiAliasWithSaveLayer,
        shape: CircleBorder(side: BorderSide(color: Colors.grey[300]!)),
        child: Center(
          child: Text(temperature[0].toString() + '°C',
              style: TextStyle(fontSize: 50, color: Colors.blue[300])),
        ),
      ),
    );
  }

  Future<Widget> _buildModeTile(BuildContext context, BluetoothDevice d) async {
    // the service is 0x00FF and the characteristic is 0xFF03

    if (d.servicesList.length < 3) {
      return CircularProgressIndicator(
        color: Colors.grey[300]!,
      );
    }
    _setModeCharacteristic = d.servicesList[2].characteristics[2];
    List<int> mode = await _setModeCharacteristic.read();

    if (mode[0] == systemState.indexOf('jets')) {
      _isJetsOn = true;
    } else {
      _isJetsOn = false;
    }
    return Text('Mode: ' + systemState[mode[0]],
        style: Theme.of(context).textTheme.bodyMedium);
  }

  Future onUpPressed() async {
    try {
      List<int> temperature =
          await widget.device.servicesList[2].characteristics[1].read();
      temperature[0] = temperature[0] + 1;
      await _setTempCharacteristic.write(temperature);
      Snackbar.show(ABC.c, "Set temp : Success", success: true);
      setState(() {});
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Descriptor Write Error:", e),
          success: false);
    }
  }

  Future onDownPressed() async {
    try {
      List<int> temperature =
          await widget.device.servicesList[2].characteristics[1].read();
      temperature[0] = temperature[0] - 1;
      await _setTempCharacteristic.write(temperature);
      Snackbar.show(ABC.c, "Set temp : Success", success: true);
      setState(() {});
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Descriptor Write Error:", e),
          success: false);
    }
  }

  Widget buildRoundButton(
      BuildContext context, Function() onPressed, String text) {
    return TextButton(
      style: ButtonStyle(
        shape: MaterialStateProperty.all(CircleBorder(
          side: BorderSide(width: 2.0),
        )),
        backgroundColor: MaterialStateProperty.all(Colors.grey[100]),
        elevation: MaterialStateProperty.all(5.0),
        surfaceTintColor: MaterialStateProperty.all(Colors.grey[300]),
      ),
      child: Text(
        text,
        style: TextStyle(fontSize: 30, color: Colors.blue[300]),
      ),
      onPressed: onPressed,
    );
  }

  Future onJetsPressed() async {
    try {
      List<int> mode = await _setModeCharacteristic.read();
      if (mode[0] == systemState.indexOf('jets')) {
        mode[0] = systemState.indexOf('transitionToHeating');
        await _setModeCharacteristic.write(mode);
        Snackbar.show(ABC.c, "Jets Off", success: true);
        _isJetsOn = false;
      } else {
        mode[0] = systemState.indexOf('transitionToJets');
        await _setModeCharacteristic.write(mode);
        Snackbar.show(ABC.c, "Jets On", success: true);
        _isJetsOn = true;
      }

      setState(() {});
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Descriptor Write Error:", e),
          success: false);
    }
  }

  Widget buildRectangleButton(
      BuildContext context, Function() onPressed, String text, bool state) {
    return TextButton(
      style: ButtonStyle(
        minimumSize: MaterialStateProperty.all(Size(200, 50)),
        shape: MaterialStateProperty.all(RoundedRectangleBorder(
            borderRadius: BorderRadius.all(Radius.circular(10.0)))),
        backgroundColor: state
            ? MaterialStateProperty.all(Colors.green[400])
            : MaterialStateProperty.all(Colors.grey[300]),
      ),
      child: Text(
        text,
        style: TextStyle(fontSize: 20, color: Colors.blue[300]),
      ),
      onPressed: onPressed,
    );
  }

  Future<Widget> _buildSetTempTile(
      BuildContext context, BluetoothDevice d) async {
    // the service is 0x00FF and the characteristic is 0xFF02
    if (d.servicesList.length < 3) {
      return Container(
        constraints: BoxConstraints(minWidth: 200, minHeight: 150),
        child: Text('Getting set temp',
            textAlign: TextAlign.center,
            style: Theme.of(context).textTheme.displaySmall),
      );
    }
    _setTempCharacteristic = d.servicesList[2].characteristics[1];
    List<int> temperature = await d.servicesList[2].characteristics[1].read();
    _setTemp = temperature[0];
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        buildRoundButton(context, onDownPressed, '-'),
        Text(
          ' ' + temperature[0].toString() + '°C ',
          style: TextStyle(fontSize: 30, color: Colors.blue[300]),
        ),
        buildRoundButton(context, onUpPressed, '+')
      ],
    );
  }

  List<Widget> _buildServiceTiles(BuildContext context, BluetoothDevice d) {
    return _services
        .map(
          (s) => ServiceTile(
            service: s,
            characteristicTiles: s.characteristics
                .map((c) => _buildCharacteristicTile(c))
                .toList(),
          ),
        )
        .toList();
  }

  CharacteristicTile _buildCharacteristicTile(BluetoothCharacteristic c) {
    return CharacteristicTile(
      characteristic: c,
      descriptorTiles:
          c.descriptors.map((d) => DescriptorTile(descriptor: d)).toList(),
    );
  }

  Widget buildSpinner(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(14.0),
      child: AspectRatio(
        aspectRatio: 1.0,
        child: CircularProgressIndicator(
          backgroundColor: Colors.black12,
          color: Colors.black26,
        ),
      ),
    );
  }

  Widget buildRemoteId(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8.0),
      child: Text('${widget.device.remoteId}'),
    );
  }

  Widget buildRssiTile(BuildContext context) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        isConnected
            ? const Icon(Icons.bluetooth_connected)
            : const Icon(Icons.bluetooth_disabled),
        Text(((isConnected && _rssi != null) ? '${_rssi!} dBm' : ''),
            style: Theme.of(context).textTheme.bodySmall)
      ],
    );
  }

  Widget buildGetServices(BuildContext context) {
    return IndexedStack(
      index: (_isDiscoveringServices) ? 1 : 0,
      children: <Widget>[
        TextButton(
          child: const Text("Get Services"),
          onPressed: onDiscoverServicesPressed,
        ),
        const IconButton(
          icon: SizedBox(
            child: CircularProgressIndicator(
              valueColor: AlwaysStoppedAnimation(Colors.grey),
            ),
            width: 18.0,
            height: 18.0,
          ),
          onPressed: null,
        )
      ],
    );
  }

  Widget buildMtuTile(BuildContext context) {
    return ListTile(
        title: const Text('MTU Size'),
        subtitle: Text('$_mtuSize bytes'),
        trailing: IconButton(
          icon: const Icon(Icons.edit),
          onPressed: onRequestMtuPressed,
        ));
  }

  Widget buildConnectButton(BuildContext context) {
    return Row(children: [
      if (_isConnecting || _isDisconnecting) buildSpinner(context),
      TextButton(
          onPressed: _isConnecting
              ? onCancelPressed
              : (isConnected ? onDisconnectPressed : onConnectPressed),
          child: Text(
            _isConnecting ? "CANCEL" : (isConnected ? "DISCONNECT" : "CONNECT"),
            style: Theme.of(context)
                .primaryTextTheme
                .labelLarge
                ?.copyWith(color: Colors.white),
          ))
    ]);
  }

  @override
  Widget build(BuildContext context) {
    return ScaffoldMessenger(
      key: Snackbar.snackBarKeyC,
      child: Scaffold(
        appBar: AppBar(
          title: Text(widget.device.platformName),
          actions: [buildConnectButton(context)],
        ),
        body: SingleChildScrollView(
          child: Column(
            children: <Widget>[
              FutureBuilder<Widget>(
                future: _buildTempTile(context, widget.device),
                builder: (context, snapshot) {
                  if (snapshot.hasData) {
                    return snapshot.data!;
                  } else if (snapshot.hasError) {
                    return Text('Error: ${snapshot.error}');
                  } else {
                    return CircularProgressIndicator();
                  }
                },
              ),
              FutureBuilder<Widget>(
                future: _buildModeTile(context, widget.device),
                builder: (context, snapshot) {
                  if (snapshot.hasData) {
                    return snapshot.data!;
                  } else if (snapshot.hasError) {
                    return Text('Error: ${snapshot.error}');
                  } else {
                    return CircularProgressIndicator();
                  }
                },
              ),
              Padding(
                padding: EdgeInsets.all(10),
              ),
              FutureBuilder<Widget>(
                future: _buildSetTempTile(context, widget.device),
                builder: (context, snapshot) {
                  if (snapshot.hasData) {
                    return snapshot.data!;
                  } else if (snapshot.hasError) {
                    return Text('Error: ${snapshot.error}');
                  } else {
                    return CircularProgressIndicator();
                  }
                },
              ),
              Padding(padding: EdgeInsets.all(10)),
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  buildRectangleButton(
                      context, onJetsPressed, 'Jets', _isJetsOn),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
