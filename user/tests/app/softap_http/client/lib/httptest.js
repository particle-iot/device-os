const client = require('./client');
const device = require('./device');
const util = require('./util');
const _ = require('lodash');
const test = require('./test');

const Client = client.Client;
const EchoClient = client.EchoClient;
const DeviceControl = device.DeviceControl;
const Promise = require('bluebird');
const Test = test.Test;

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

class HttpTest extends Test {
  constructor(name, func) {
    super(name, func);
  }

  run() {
    let memBefore = 0;
    return util.mapAll([
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
}

module.exports = {
  TestError: test.TestError,
  Test: HttpTest
};
