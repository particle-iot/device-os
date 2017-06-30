const net = require('net');
const os = require('os');
const exec = require('child_process').exec;
const util = require('util');
const _ = require('lodash');

const DEFAULT_PORT = 1234;
const USB_REQUEST_TIMEOUT = 10000;

function hostAddress() {
  // Get first non-internal IPv4 address available
  const ifaces = os.networkInterfaces();
  for (const iface in ifaces) {
    const addrs = ifaces[iface];
    for (var i = 0; i < addrs.length; ++i) {
      const addr = addrs[i];
      if (!addr.internal && addr.family == 'IPv4') {
        return addr.address;
      }
    }
  }
  throw new Error('Unable to determine host address');
}

function exit(err) {
  if (_.isUndefined(err)) {
    process.exit();
  } else if (_.isError(err)) {
    console.error('Error: %s', err.message);
    process.exit(1);
  } else {
    process.exit(err);
  }
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
      if (callback) {
        callback(null, rep);
      }
    } catch (err) {
      if (callback) {
        callback(err);
      }
    }
  });
}

function startServer(host, port, callback) {
  // Simple echo server
  const server = net.createServer((sock) => {
    sock.on('data', (data) => {
      sock.write(data, (err) => {
        if (err) {
          sock.destroy(err);
        }
      });
    });
    sock.on('error', (err) => {
      console.log('Socket error: %s', err.message);
      sock.destroy();
    });
  });
  const opts = {
    host: host,
    port: port
  };
  server.listen(opts, callback);
  return server;
}

function startTest(host, port, callback) {
  const req = {
    cmd: 'start',
    host: host,
    port: port
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
var host = null;
if (process.argv.length >= 3) {
  host = process.argv[2];
} else {
  host = hostAddress();
}
var port = DEFAULT_PORT;
if (process.argv.length >= 4) {
  port = parseInt(process.argv[3]);
}

// Start server
console.log('Server address: %s:%d', host, port);
startServer(host, port, (err) => {
  if (err) {
    return exit(err);
  }
  console.log('Server started');
  // Start tests
  console.log('Running tests...');
  startTest(host, port, (err) => {
    if (err) {
      return exit(err);
    }
    // Wait until tests are finished
    setInterval(() => {
      getStatus((err, stat) => {
        if (err) {
          return exit(err);
        }
        if (stat.done) {
          console.log(stat.passed ? 'PASSED' : 'FAILED');
          exit(stat.passed ? 0 : 1);
        }
      });
    }, 1000);
  });
});
