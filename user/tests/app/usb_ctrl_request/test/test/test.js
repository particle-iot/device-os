import * as control from './support/control';
import * as proto from './support/proto';
import * as usb from './support/usb';
import './support/init';

import * as semver from 'semver';
import * as randomstring from 'randomstring';

import { expect } from 'chai';

describe('USB requests', function() {
  let dev = null;

  before('find a Particle USB device', function() {
    return usb.enumDevices().then((devs) => {
      if (devs.length == 0) {
        throw new Error('No Particle USB devices found');
      }
      dev = devs[0]; // Use the first device in the list
    });
  });

  before('open device', function() {
    return dev.open();
  });

  after('close device', function() {
    if (dev) {
      return dev.close();
    }
  });

  describe('application control requests', function() {
    this.slow(2000); // Test application replies with a random delay

    it('echo request should succeed', function() {
      const reqData = 'hello';
      return control.sendRequest(dev, control.RequestType.APP_CUSTOM, reqData).should.become({
        result: control.Result.OK,
        data: 'hello'
      });
    });

    it('large request should succeed', function() {
      const reqData = randomstring.generate(10240);
      return control.sendRequest(dev, control.RequestType.APP_CUSTOM, reqData).should.become({
        result: control.Result.OK,
        data: reqData
      });
    });

    it('concurrent requests should succeed', function() {
      let reqs = [];
      for (let i = 0; i < control.MAX_ACTIVE_REQUESTS; ++i) {
        const reqData = randomstring.generate(128);
        reqs.push(control.sendRequest(dev, control.RequestType.APP_CUSTOM, reqData).should.become({
          result: control.Result.OK,
          data: reqData
        }));
      }
      return Promise.all(reqs);
    });
  });

  describe('system control requests', function() {
    it('module info request should succeed', function() {
      this.slow(2000); // MODULE_INFO is a slow request
      return control.sendRequest(dev, control.RequestType.MODULE_INFO).then((rep) => {
        expect(rep.result).to.equal(control.Result.OK);
        const info = JSON.parse(rep.data);
        expect(info).to.have.property('p');
        expect(info).to.have.property('m');
      });
    });

    it('diagnostic info request should succeed', function() {
      return control.sendRequest(dev, control.RequestType.DIAGNOSTIC_INFO).then((rep) => {
        expect(rep.result).to.equal(control.Result.OK);
        const info = JSON.parse(rep.data);
      });
    });
  });

  describe('system low-level requests', function() {
    it('device ID request should succeed', function() {
      const setup = { // Setup packet
        bmRequestType: proto.bmRequestType.DEVICE_TO_HOST,
        bRequest: 0x50, // ASCII code of the character 'P'
        wIndex: control.RequestType.DEVICE_ID, // Request type
        wValue: 0,
        wLength: usb.MIN_WLENGTH
      };
      return dev.controlTransfer(setup).should.eventually.have.lengthOf(24);
    });

    it('system version request should succeed', function() {
      const setup = {
        bmRequestType: proto.bmRequestType.DEVICE_TO_HOST,
        bRequest: 0x50,
        wIndex: control.RequestType.SYSTEM_VERSION,
        wValue: 0,
        wLength: usb.MIN_WLENGTH
      };
      return dev.controlTransfer(setup).then((data) => {
        const ver = data.toString();
        expect(semver.valid(ver)).to.not.be.null;
      });
    });
  });
});
