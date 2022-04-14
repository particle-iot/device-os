const client = require('./client');
const randomstring = require('randomstring');
const _ = require('lodash');
const Promise = require('bluebird');

const Client = client.Client;
const State = client.State;

const CERTIFICATE_SIZE = 4096;
const PRIVATE_KEY_SIZE = 2048;
const CONFIGURE_AP_DEFAULT_BODY_SIZE = 400;

const DEFAULT_HTTP_HOST = '192.168.0.1';
const DEFAULT_HTTP_PORT = 80;
const DEFAULT_HTTP_ECHO_PATH = '/echo';
const DEFAULT_HTTP_QUERY_ECHO_PATH = '/query';

const HTTP_MAX_URL_LENGTH = 100;

class HttpEchoClient extends Client {
  constructor(host, port, path) {
    super(DEFAULT_HTTP_HOST , DEFAULT_HTTP_PORT);
    this._path = path || DEFAULT_HTTP_ECHO_PATH;
    this._echoStr = '';
  }

  get path() {
    return this._path;
  }

  read(size) {
    return this._setState(State.CONNECTED, State.READING);
  }

  _read() {
    try {
      if (this._checkReply(this._data, this._echoStr) == true) {
        this._resolveState(State.READING, State.CONNECTED);
      }
      // Otherwise need more data
    } catch (e) {
      this._error(e);
    }
  }

  _consume(size) {
    this._data = '';
  }

  _checkReply(reply, echoStr) {
    if (!_.isString(reply)) {
      throw new Error('Unexpected reply from server');
    }

    const lines = reply.split('\r\n');
    const headersEnd = lines.indexOf('') + 1;
    const headers = lines.slice(0, headersEnd);
    const body = lines.slice(headersEnd);

    if (headers[0] != 'HTTP/1.1 200 OK') {
      throw new Error('Unexpected HTTP code');
    }

    const chunked = lines.includes('Transfer-Encoding: chunked');
    let data = body.join('\r\n');
    if (chunked) {
      data = '';

      let total = 0;
      let sawLastChunk = false;
      for (let i = 0; i < body.length && (i + 1) < body.length; i += 2) {
        let len = parseInt(body[i], 16);
        if (len == 0 && (i + 3) == body.length && body[i + 1] == '' && body[i + 2] == '') {
          // Last chunk
          sawLastChunk = true;
          break;
        }
        let chunk = body[i + 1];
        total += len;
        data += chunk;
      }

      if (!sawLastChunk) {
        return false;
      }

      if (total != echoStr.length) {
        throw new Error('Reconstructed data size doesn\'t match');
      }
    } else {
      if (data.length < echoStr.length) {
        return false;
      }
    }

    this._consume();
    return true;
  }

  echo(size, close) {
    // 400 (average configure-ap request size) to 12k (certiticate + ca + hex-encoded private key)
    const len = !_.isUndefined(size) ? size :
                               Math.floor(Math.random()
                               * (2 * CERTIFICATE_SIZE + 2 * PRIVATE_KEY_SIZE)
                               + CONFIGURE_AP_DEFAULT_BODY_SIZE);
    const str = randomstring.generate(len);
    // NB: SoftAP page database uses application/octet-stream for all the requests for some reason
    const req = `POST ${this._path} HTTP/1.1\r\n` +
                `Host: ${this._host}\r\n` +
                `User-Agent: HttpEchoClient/1.0.0\r\n` +
                `Accept: text/plain\r\n` +
                `Content-Length: ${len}\r\n` +
                `Content-Type: application/octet-stream\r\n` +
                `${close ? 'Connection: Close' : 'Connection: Keep-Alive'}\r\n` +
                `\r\n` +
                str;
    return this.write(req)
      .then(() => {
        this._echoStr = str;
        return this.read(len);
      });
  }
}

class HttpQueryEchoClient extends HttpEchoClient {
  constructor(host, port, path) {
    super(host, port, path || DEFAULT_HTTP_QUERY_ECHO_PATH);
  }

  echo(size, close) {
    // 400 (average configure-ap request size) to 12k (certiticate + ca + hex-encoded private key)
    const len = !_.isUndefined(size) ? size : HTTP_MAX_URL_LENGTH - this._path.length - 2;
    const str = randomstring.generate(len);
    // NB: SoftAP page database uses application/octet-stream for all the requests for some reason
    const req = `GET ${this._path}?${str} HTTP/1.1\r\n` +
                `Host: ${this._host}\r\n` +
                `User-Agent: HttpQueryEchoClient/1.0.0\r\n` +
                `Accept: text/plain\r\n` +
                `Content-Length: 0\r\n` +
                `${close ? 'Connection: Close' : 'Connection: Keep-Alive'}\r\n` +
                `\r\n`;
    return this.write(req)
      .then(() => {
        this._echoStr = str;
        return this.read(len);
      });
  }
}

module.exports = {
  DEFAULT_HTTP_PORT: DEFAULT_HTTP_PORT,
  DEFAULT_HTTP_ECHO_PATH: DEFAULT_HTTP_ECHO_PATH,
  HttpEchoClient: HttpEchoClient,
  HttpQueryEchoClient: HttpQueryEchoClient
};
