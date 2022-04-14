const exec = require('child_process').exec;
const util = require('util');
const _ = require('lodash');

const Promise = require('bluebird');

const DEFAULT_TIMEOUT = 5000;

let commands = [];

// Device control interface
class DeviceControl {
  static startServer(id, type, port, timeout) {
    return this.command('startServer', { id: id, type: type, port: port }, timeout);
  }

  static stopServer(id, timeout) {
    return this.command('stopServer', { id: id }, timeout);
  }

  static stopServers(timeout) {
    return this.command('stopServers', timeout);
  }

  static connectToNetwork(timeout) {
    return this.command('connectToNetwork', timeout).then((rep) => {
      return rep.address;
    });
  }

  static disconnectFromNetwork(timeout) {
    return this.command('disconnectFromNetwork', timeout);
  }

  static getNetworkStatus(timeout) {
    return this.command('getNetworkStatus', timeout);
  }

  static getFreeMemory(timeout) {
    return this.command('getFreeMemory', timeout).then((rep) => {
      return rep.bytes;
    });
  }

  static command(cmd, paramsOrTimeout, timeout) {
    let p = {}; // Parameters
    let t = DEFAULT_TIMEOUT; // Timeout
    if (_.isNumber(paramsOrTimeout)) {
      t = paramsOrTimeout;
    } else if (!_.isUndefined(paramsOrTimeout)) {
      p = paramsOrTimeout;
      if (_.isNumber(timeout)) {
        t = timeout;
      }
    }
    return new Promise((resolve, reject) => {
      // Put device command into the queue
      commands.push({
        name: cmd,
        params: p,
        resolve: resolve,
        reject: reject,
        timeout: t
      });
      // Start processing commands if the queue was empty
      if (commands.length == 1) {
        this._nextCommand();
      }
    });
  }

  static _nextCommand() {
    if (commands.length > 0) {
      const cmd = commands[0]; // Get next command
      const req = cmd.params || {}; // Request object
      req.cmd = cmd.name;
      const json = JSON.stringify(req); // Request JSON data
      const hex = Buffer.from(json, 'utf8').toString('hex');
      // Request: 80 ('P'); value: 0; index: 10 (USB_REQUEST_CUSTOM)
      exec(util.format('send_usb_req -t %d -r 80 -v 0 -i 10 -x %s', cmd.timeout, hex), (err, stdout, stderr) => {
        try {
          if (err) {
            throw err;
          }
          const json = Buffer.from(stdout.trim(), 'hex').toString('utf8'); // Reply JSON data
          const rep = JSON.parse(json); // Reply object
          if (_.has(rep, 'error')) {
            throw new Error(rep.error);
          }
          cmd.resolve(rep);
        } catch (err) {
          cmd.reject(err);
        } finally {
          commands.shift(); // Remove current command from the queue
          this._nextCommand(); // Process next command
        }
      });
    }
  }
}

module.exports = {
  DEFAULT_TIMEOUT: DEFAULT_TIMEOUT,
  DeviceControl: DeviceControl
};
