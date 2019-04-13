'use strict';

// Supporting debug modules
const WebunitySdkHelperModule = require('./webunitySdkHelper');
const WebunitySdkHelper = new WebunitySdkHelperModule('nvidia-shield:interface');

// 3rd party modules
const EventEmitter = require('events');
const OneWebSocket = require('one-websocket');

// Driver constants
const CONSTANTS = require('./constants');

/**
 * Manager class
 */
class Interface extends EventEmitter {
	/**
	 * Constructor
	 */
	constructor() {
		super();

		// Fix for double menu events which (at least) my remote sends.
		this.menuTimer = null;

		// Websocket variables
		this.wsHost = WebunitySdkHelper.ConfigHasOr('nvidiaShield.Peripheral.Host', 'localhost');
		this.wsPort = WebunitySdkHelper.ConfigHasOr('nvidiaShield.Peripheral.Port', 8000);
		WebunitySdkHelper.Debug('- Trying to connect to Bluetooth peripheral\'s websocket server at ' + this.wsHost + ', port ' + this.wsPort);

		// Websocket connection
		this.wsConnection = new OneWebSocket('ws://' + this.wsHost + ':' + this.wsPort);
		this.wsConnection.on('connect', this._wsConnect.bind(this));
		this.wsConnection.on('data', this._wsData.bind(this));
		this.wsConnection.on('disconnect', this._wsDisconnected.bind(this));

	}

	/**
	 * (internal) Callback function whenver a websocket is connected
	 */
	_wsConnect() {
		WebunitySdkHelper.Debug('- Connected to the peripheral.');
	}

	/**
	 * (internal) Callback function whenver data is received on the websocket connection.
	 */
	_wsData(data) {
		WebunitySdkHelper.Debug(`- Received message: ${data}`);
	}

	/**
	 * (internal) Callback function whenver a websocket is discconnected
	 */
	_wsDisconnected() {
		WebunitySdkHelper.Debug('- No longer connected to the peripheral.');
	}

	/**
	 * Handler in case a button is pressed.
	 * @param serialNumberOrUniqueId The unique ID of the device which got a button press (correlates with the found mediaboxes)
	 * @param button which button was pressed?
	 */
	ButtonPressed(serialNumberOrUniqueId, btn) {
		WebunitySdkHelper.Debug('- Received button press: ' + btn);
		if (!this.wsConnection.isConnected()) {
			WebunitySdkHelper.Debug('  - Not connected to peripheral!');
			return;
		}

		btn = btn.toUpperCase().replace(/ /g, '_');
		if (!(btn in CONSTANTS.BUTTON)) {
			WebunitySdkHelper.Debug('  - This button is not a known button!');
			return;
		}

		// BUG: The 'MENU' button is fired twice (on my remote that is)
		if (btn == 'MENU') {
			if (this.menuTimer !== null) {
				return;
			}
			var _this = this;
			this.menuTimer = setTimeout(function() { clearTimeout(_this.menuTimer); _this.menuTimer = null; }, 250);
		}

		// In all other cases we should have support for it.
		var cmd = CONSTANTS.BUTTON[btn];
		this.wsConnection.send(cmd);
	}
}

module.exports = Interface;