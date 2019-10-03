'use strict'

const { EventEmitter } = require('events')
const ws = require('ws')

const _WebSocket = (typeof ws === 'function') ? ws : window.WebSocket

const MAXIMUM_RECONNECT_DELAY = 20000 // twenty seconds

class OneWebSocket extends EventEmitter {
    constructor(url, options) {
        super()

        this._validateUrl(url)

        this._url = url
        this._options = buildOptions(options || {})

        this._socket = null

        this._isConnected = false
        this._isCleanUpMode = false

        this._isDestroyed = false
        this._isReconnecting = true
        this._reconnectTimer = null
        this._retryAttempts = 0

        this._openSocket()
    }

    binaryType() {
        return 'arraybuffer'
    }

    bufferedAmount() {
        return this._socket ? this._socket.bufferedAmount : 0
    }

    isConnected() {
        return this._isConnected
    }

    isDestroyed() {
        return this._isDestroyed
    }

    readyState() {
        return this._socket ? this._socket.readyState : _WebSocket.CLOSED
    }

    url() {
        return this._url
    }

    destroy() {
        this._isDestroyed = true
        this._cleanUp()
    }

    send(data) {
        if (this._isDestroyed) {
            throw new Error('cannot call send after the socket has been destroyed')
        }
        this._socket.send(data)
    }

    static get CLOSED() {
        return _WebSocket.CLOSED
    }

    static get CLOSING() {
        return _WebSocket.CLOSING
    }

    static get CONNECTING() {
        return _WebSocket.CONNECTING
    }

    static get OPEN() {
        return _WebSocket.OPEN
    }

    static get IS_WEBSOCKET_SUPPORTED() {
        return _WebSocket != null
    }

    _validateUrl(url) {
        if (typeof url !== 'string') {
            throw new Error('WebSocket url must be of type string')
        }

        const isValidUrl = url.startsWith('ws://') || url.startsWith('wss://')

        if (!isValidUrl) {
            throw new Error("WebSocket url must begin with either 'ws://' or 'wss://'")
        }
    }

    _openSocket() {
        this._isCleanUpMode = false

        if (typeof ws === 'function') {
            // Node.js
            this._socket = new _WebSocket(this._url, this._options)
        } else {
            // Browser
            this._socket = new _WebSocket(this._url)
        }
        this._socket.binaryType = 'arraybuffer'
        this._addWebSocketHandlers()
    }

    _addWebSocketHandlers() {
        const self = this
        self._socket.onopen = function (event) {
            self._onOpen(event)
        }

        self._socket.onmessage = function (event) {
            self._onMessage(event)
        }

        self._socket.onclose = function (event) {
            self._onClose(event)
        }

        self._socket.onerror = function (event) {
            self._onError(event)
        }
    }

    _onOpen(event) {
        if (this._isConnected || this._isCleanUpMode || this._isDestroyed) {
            return
        }

        this._isConnected = true

        if (this._isReconnecting) {
            this._isReconnecting = false
            this._retryAttempts = 0
        }

        this.emit('connect', event)
    }

    _onMessage(event) {
        if (this._isCleanUpMode || this._isDestroyed) {
            return
        }
        this.emit('data', event.data)
    }

    _onClose(event) {
        if (this._isCleanUpMode || this._isDestroyed) {
            return
        }

        this._cleanUp()
        this.emit('disconnect')

        if (this._options.autoReconnect) {
            this._startReconnectTimer()
        } else {
            this._isDestroyed = true
        }
    }

    _onError(event) {
        if (this._isCleanUpMode || this._isDestroyed) {
            return
        }

        this._cleanUp()

        const err = new Error(`WebSocket connection to "${this._url}" failed`)
        this.emit('warning', err)

        if (this._options.autoReconnect) {
            this._startReconnectTimer()
        } else {
            this._isDestroyed = true
        }
    }

    _startReconnectTimer() {
        if (this._isDestroyed) return

        this._isReconnecting = true
        clearTimeout(this._reconnectTimer)

        const millisecondsDelay = this._calculateBackoff()

        this._reconnectTimer = setTimeout(() => {
            if (this._retryAttempts < this._options.maxReconnectAttempts) {
                this._retryAttempts++
                this._openSocket()
            } else {
                this.destroy()
            }
        }, millisecondsDelay)
    }

    _calculateBackoff() {
        let maximumDelay = MAXIMUM_RECONNECT_DELAY

        if (this._retryAttempts < 5) {
            maximumDelay = Math.pow(2, this._retryAttempts) * 1000
        }

        return getRandomInt(maximumDelay)
    }

    _cleanUp() {
        this._isConnected = false
        this._isCleanUpMode = true

        clearTimeout(this._reconnectTimer)

        if (this._socket) {
            const socket = this._socket
            const onClose = function () {
                socket.onclose = null
            }
            if (socket.readyState === _WebSocket.CLOSED) {
                onClose()
            } else {
                try {
                    socket.onclose = onClose
                    socket.close()
                } catch (err) {
                    onClose()
                }
            }

            socket.onopen = null
            socket.onmessage = null
            socket.onerror = function () { }
        }
        this._socket = null
    }
}

function buildOptions(options) {
    const updatedOptions = {
        autoReconnect: options.autoReconnect !== false,
        maxReconnectAttempts: Infinity
    }

    if (Number.isInteger(options.maxReconnectAttempts)) {
        updatedOptions.maxReconnectAttempts = options.maxReconnectAttempts
    }

    return Object.assign(updatedOptions, options)
}

/**
 * Returns a random integer between 0 (inclusive) and max (inclusive).
 * @param  {Number} max
 * @return {Number} random integer
 */
function getRandomInt(max) {
    return Math.floor(Math.random() * (max + 1))
}

module.exports = OneWebSocket