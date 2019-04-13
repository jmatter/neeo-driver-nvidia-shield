'use strict';

// 3rd party modules
const NeeoSdk = require('neeo-sdk');

// Supporting debug modules
const WebunitySdkHelperModule = require('./lib/webunitySdkHelper');
const WebunitySdkHelper = new WebunitySdkHelperModule('nvidia-shield:main');

// Start discovery object
const Interface = require('./lib/interface');
const WebsocketInterface = new Interface();


// Set the device info, used to identify it on the Brain
const deviceName = 'Shield Bluetooth driver';
const deviceVersion = 1;
const neeoDriverNvidiaShield = NeeoSdk.buildDevice(deviceName)
	.setDriverVersion(deviceVersion)
	.setManufacturer('NVidia')
	.addAdditionalSearchToken('NVidia')
	.addAdditionalSearchToken('Shield')
	.addAdditionalSearchToken('Bluetooth')
	.setType('GAMECONSOLE')
	.addButtonGroup('POWER')
	.addButtonGroup('Menu and Back')
	.addButtonGroup('Controlpad')
	.addButtonGroup('Volume')
	.addButtonGroup('Transport')
	.addButtonGroup('Transport Scan')
	.addButtonGroup('Transport Skip')
	.addButton({ name: 'HOME', label: WebunitySdkHelper.ConfigHasOr('nvidiaShield.UiLabels.Home', 'HOME SCREEN') })
	.addButtonHandler((btn, serialNumberOrUniqueId) => { WebsocketInterface.ButtonPressed(serialNumberOrUniqueId, btn); });

// Module export
module.exports = {
	devices: [
		neeoDriverNvidiaShield,
	],
  };