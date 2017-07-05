const async = require('async');
const randomstring = require('randomstring');
const net = require('net');
const os = require('os');
const exec = require('child_process').exec;
const util = require('util');
const _ = require('lodash');

const ECHO_PORT = 2001;
const DISCARD_PORT = 2002;
const CHARGEN_PORT = 2003;

const CHARGEN_PACKET_SIZE = 1024;

const USB_REQUEST_TIMEOUT = 10000;

class TimeoutError extends Error {
  constructor(msg) {
    super(msg ? msg : 'Timeout error');
    this.name = this.constructor.name;
    Error.captureStackTrace(this, this.constructor);
  }
}

function hostAddress() {
  // Get first non-internal IPv4 address available
  const ifaces = os.networkInterfaces();
  for (const iface in ifaces) {
    const addrs = ifaces[iface];
    for (let i = 0; i < addrs.length; ++i) {
      const addr = addrs[i];
      if (!addr.internal && addr.family == 'IPv4') {
        return addr.address;
      }
    }
  }
  throw new Error('Unable to determine host address');
}

function sendUsbRequest(req, callback) {
  const json = JSON.stringify(req); // Request JSON data
  const hex = Buffer.from(json, 'utf8').toString('hex');
  // Request: 80 ('P'); value: 0; index: 10 (USB_REQUEST_CUSTOM)
  exec(util.format('send_usb_req -t %d -r 80 -v 0 -i 10 -x %s', USB_REQUEST_TIMEOUT, hex), (err, stdout) => {
    try {
      if (err) {
        throw err;
      }
      const json = Buffer.from(stdout.trim(), 'hex').toString('utf8'); // Reply JSON data
      const rep = JSON.parse(json); // Reply object
      callback(null, rep);
    } catch (err) {
      if (err.code == 2) {
        callback(new TimeoutError());
      } else {
        callback(err);
      }
    }
  });
}

function startEchoServer(host, callback) {
  const server = net.createServer((sock) => {
    sock.on('data', (data) => {
      sock.write(data, (err) => {
        if (err) {
          sock.destroy(err);
        }
      });
    });
    sock.on('error', (err) => {
      console.error('Socket error: %s', err.message);
      sock.destroy();
    });
  });
  const opts = {
    host: host,
    port: ECHO_PORT
  };
  server.listen(opts, callback);
  return server;
}

function startDiscardServer(host, callback) {
  const server = net.createServer((sock) => {
    sock.on('error', (err) => {
      console.error('Socket error: %s', err.message);
      sock.destroy();
    });
  });
  const opts = {
    host: host,
    port: DISCARD_PORT
  };
  server.listen(opts, callback);
  return server;
}

function startChargenServer(host, callback) {
  const server = net.createServer((sock) => {
    const writeCallback = (err) => {
      if (err) {
        sock.destroy(err);
      }
    };
    const send = () => {
      let data = null;
      do {
        data = randomstring.generate(CHARGEN_PACKET_SIZE);
      } while (sock.write(data, writeCallback));
    };
    sock.on('drain', () => {
      send();
    });
    sock.on('error', (err) => {
      console.error('Socket error: %s', err.message);
      sock.destroy();
    });
    send();
  });
  const opts = {
    host: host,
    port: CHARGEN_PORT
  };
  server.listen(opts, callback);
  return server;
}

function startTest(host, callback) {
  const req = {
    cmd: 'start',
    host: host,
    echoPort: ECHO_PORT,
    discardPort: DISCARD_PORT,
    chargenPort: CHARGEN_PORT
  };
  sendUsbRequest(req, callback);
}

function getStatus(callback) {
  const req = {
    cmd: 'status'
  };
  sendUsbRequest(req, callback);
}

// Parse command line arguments
let host = null;
if (process.argv.length >= 3) {
  host = process.argv[2];
} else {
  host = hostAddress();
}

console.log('Server address: %s', host);

async.series([
  // Start echo server
  (callback) => {
    startEchoServer(host, callback);
  },
  // Start discard server
  (callback) => {
    startDiscardServer(host, callback);
  },
  // Start chargen server
  (callback) => {
    startChargenServer(host, callback);
  },
  // Start tests
  (callback) => {
    startTest(host, callback);
  },
  // Wait until testing is finished
  (callback) => {
    const checkStatus = () => {
      setTimeout(() => {
        getStatus((err, stat) => {
          if (err) {
            if (err instanceof TimeoutError) { // Ignore timeout errors
              console.error('USB request timeout');
            } else {
              return callback(err);
            }
          } else if (stat.done) {
            if (stat.passed) {
              return callback();
            } else {
              return callback(new Error('FAILED'));
            }
          }
          checkStatus();
        });
      }, 1000);
    };
    console.log('Running tests...');
    checkStatus();
  }], (err) => {
    if (err) {
      console.error(err.message);
      process.exit(1);
    } else {
      console.log('PASSED');
      process.exit();
    }
  }
);
