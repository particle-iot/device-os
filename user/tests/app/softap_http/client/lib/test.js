const client = require('./client');
const device = require('./device');
const util = require('./util');
const _ = require('lodash');

const Client = client.Client;
const EchoClient = client.EchoClient;
const DeviceControl = device.DeviceControl;
const Promise = require('bluebird');

const DEFAULT_PORT = 1234;
const MEMORY_USAGE_THRESH = 0.025;

class TestError extends Error {
  constructor(msg, stack) {
    super(msg);
    this.name = this.constructor.name;
    if (stack) {
      this.stack = stack;
    } else {
      Error.captureStackTrace(this, this.constructor);
    }
  }
}

class Test {
  constructor(name, func) {
    this._name = name;
    this._func = func;
    this._devAddr = 'localhost';
    this._clients = {};
    this._servers = new Set();
    this._nextClientId = 1;
    this._nextServerId = 1;
    this._verbose = true;
  }

  run() {
    let memBefore = 0;
    return util.mapAll([
      // Ensure device is connected to network
      () => {
        return this.connectToNetwork();
      },
      // Get device's free memory before test
      () => {
        return DeviceControl.getFreeMemory()
          .then((freeMem) => {
            memBefore = freeMem;
          })
      },
      // Run test
      () => {
        return this._func(this)
          .catch((err) => {
            if (!(err instanceof TestError)) {
              err = new TestError(err.message, err.stack);
            }
            throw err;
          });
      },
      // Close all client connections
      () => {
        if (_.isEmpty(this._clients)) {
          return Promise.resolve();
        }
        return util.mapAll(_.values(this._clients), (client) => {
          return client.disconnect(); // Disconnect gracefully
        })
        .catch((err) => {
          _.forEach(this._clients, (client) => {
            client.close(); // Close connection synchronously
          });
          throw err;
        });
      },
      // Stop all servers
      () => {
        return this.stopServers();
      },
      // Check device's free memory after test
      () => {
        return DeviceControl.getFreeMemory()
          .then((memAfter) => {
            if (memAfter < memBefore && (memBefore - memAfter) >= memBefore * MEMORY_USAGE_THRESH) {
              throw new Error('Memory check failed');
            }
          })
      }],
      (func) => {
        return func();
      }
    );
  }

  newClient(classOrPort, port) {
    let cls = Client;
    let p = DEFAULT_PORT;
    if (_.isNumber(classOrPort)) {
      p = classOrPort;
    } else if (!_.isUndefined(classOrPort)) {
      cls = classOrPort;
      if (_.isNumber(port)) {
        p = port;
      }
    }
    const id = this._nextClientId++;
    const client = new cls(this._devAddr, p);
    client.on('connect', () => {
      if (this._verbose) {
        console.log('Connected to %s:%d', client.host, client.port);
      }
      this._clients[id] = client;
    });
    client.on('close', () => {
      if (this._verbose) {
        console.log('Disconnected from %s', client.host);
      }
      delete this._clients[id];
    });
    return client;
  }

  startServer(type, idOrPort, port) {
    let id = util.format('server%d', this._nextServerId++);
    let p = DEFAULT_PORT;
    if (_.isNumber(idOrPort)) {
      p = idOrPort;
    } else if (!_.isUndefined(idOrPort)) {
      id = idOrPort;
      if (_.isNumber(port)) {
        p = port;
      }
    }
    return DeviceControl.startServer(id, type, p)
      .then(() => {
        if (this._verbose) {
          console.log('Started server: %s, port: %d (%s)', id, p, type);
        }
        this._servers.add(id);
      })
  }

  stopServer(id) {
    return DeviceControl.stopServer(id)
      .then(() => {
        if (this._verbose) {
          console.log('Stopped server: %s', id);
        }
        this._servers.delete(id);
      });
  }

  stopServers() {
    if (_.isEmpty(this._servers)) {
      return Promise.resolve();
    }
    return DeviceControl.stopServers()
      .then(() => {
        if (this._verbose) {
          console.log('Stopped all servers');
        }
        this._servers.clear();
      });
  }

  connectToNetwork() {
    return DeviceControl.getNetworkStatus()
      .then((stat) => {
        if (!stat.connected) {
          return DeviceControl.connectToNetwork(45000)
            .then((addr) => {
              this._devAddr = addr;
              if (this._verbose) {
                console.log('Connected to network, device address: %s', addr);
              }
            });
        } else {
          this._devAddr = stat.address;
        }
      });
  }

  disconnectFromNetwork() {
    return DeviceControl.getNetworkStatus()
      .then((stat) => {
        if (stat.connected) {
          return DeviceControl.disconnectFromNetwork()
            .then(() => {
              if (this._verbose) {
                console.log('Disconnected from network');
              }
            });
        }
      });
  }

  set verbose(enabled) {
    this._verbose = enabled;
  }

  get name() {
    return this._name;
  }
}

module.exports = {
  DEFAULT_PORT: DEFAULT_PORT,
  TestError: TestError,
  Test: Test
};
