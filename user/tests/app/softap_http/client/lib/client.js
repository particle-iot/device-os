const randomstring = require('randomstring');
const net = require('net');
const _ = require('lodash');

const EventEmitter = require('events');
const Promise = require('bluebird');

const DEFAULT_TIMEOUT = 30000;

const State = {
  NEW: 1,
  CONNECTING: 2,
  CONNECTED: 3,
  READING: 4,
  WRITING: 5,
  CLOSING: 6,
  CLOSED: 7
};

// Promise-based TCP client for testing purposes
class Client extends EventEmitter {
  constructor(host, port) {
    super();
    this._host = host;
    this._port = port;
    this._data = '';
    this._readSize = 0;
    this._resolve = null;
    this._reject = null;
    this._timer = null;
    this._timeout = DEFAULT_TIMEOUT;
    this._sock = this._newSocket();
    this._state = State.NEW;
  }

  connect() {
    return this._setState(State.NEW, State.CONNECTING, () => {
      this._sock.connect({ host: this._host, port: this._port });
    });
  }

  disconnect() {
    return this._setState(State.CONNECTED, State.CLOSING, () => {
      this._sock.end();
    });
  }

  read(size) {
    return this._setState(State.CONNECTED, State.READING, () => {
      this._readSize = size;
      this._read();
    });
  }

  write(data) {
    return this._setState(State.CONNECTED, State.WRITING, () => {
      this._sock.write(data, null, () => {
        this._resolveState(State.WRITING, State.CONNECTED);
      });
    });
  }

  waitUntilClosed() {
    return this._setState(State.CONNECTED, State.CLOSING);
  }

  // Closes connection synchronously
  close() {
    this._state = State.CLOSED;
    this._sock.destroy();
    if (this._timer) {
      clearTimeout(this._timer);
      this._timer = null
    }
    if (this._reject) {
      this._reject(new Error('Operation has been aborted'));
      this._reject = null;
    }
    this._resolve = null;
    this._data = '';
  }

  get connecting() {
    return this._state == State.CONNECTING;
  }

  get connected() {
    return this._state == State.CONNECTED || this._state == State.READING || this._state == State.WRITING;
  }

  get closing() {
    return this._state == State.CLOSING;
  }

  get closed() {
    return this._state == State.CLOSED;
  }

  get host() {
    return this._host;
  }

  get port() {
    return this._port;
  }

  get timeout() {
    return this._timeout;
  }

  set timeout(timeout) {
    this._timeout = timeout;
  }

  _setState(curState, newState, func) {
    return new Promise((resolve, reject) => {
      if (this._state != curState) {
        throw new Error('Invalid object state');
      }
      this._state = newState;
      this._resolve = resolve;
      this._reject = reject;
      this._timer = setTimeout(() => {
        this._error(new Error('Operation has timed out'));
      }, this._timeout);
      if (func) {
        func();
      }
    });
  }

  _resolveState(curState, newState, data) {
    if (this._state == curState) {
      this._state = newState;
      if (this._timer) {
        clearTimeout(this._timer);
        this._timer = null;
      }
      if (this._resolve) {
        this._resolve(data);
        this._resolve = null;
      }
      this._reject = null;
    }
  }

  _error(err) {
    if (this._reject) {
      this._reject(err);
      this._reject = null;
    }
    this.close();
  }

  _read() {
    if (this._state == State.READING && this._data.length >= this._readSize) {
      const d = this._data.substr(0, this._readSize);
      this._data = this._data.substr(this._readSize);
      this._resolveState(State.READING, State.CONNECTED, d);
    }
  }

  _newSocket() {
    const sock = new net.Socket();
    sock.setEncoding('utf8');
    sock.on('connect', () => {
      this._resolveState(State.CONNECTING, State.CONNECTED);
      this.emit('connect');
    });
    sock.on('close', () => {
      if (this._state == State.CLOSING) {
        this._resolveState(State.CLOSING, State.CLOSED);
      } else {
        this._error(new Error('Connection has been closed'));
      }
      this.close();
      this.emit('close');
    });
    sock.on('data', (data) => {
      this._data += data;
      this._read();
    });
    sock.on('error', (err) => {
      this._error(err);
    });
    return sock;
  }
}

class EchoClient extends Client {
  echo(strOrLength) {
    let str = strOrLength;
    if (_.isNumber(strOrLength)) {
      str = randomstring.generate(strOrLength);
    } else if (_.isUndefined(strOrLength)) {
      const len = Math.floor(Math.random() * 125 + 4); // 4 to 128 characters
      str = randomstring.generate(len);
    }
    return this.write(str)
      .then(() => {
        return this.read(str.length)
      })
      .then((reply) => {
        if (reply != str) {
          throw new Error('Unexpected reply from server');
        }
      });
  }
}

module.exports = {
  DEFAULT_TIMEOUT: DEFAULT_TIMEOUT,
  Client: Client,
  EchoClient: EchoClient,
  State: State
};
