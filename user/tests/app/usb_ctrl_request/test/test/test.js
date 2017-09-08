import * as usb from './support/usb';
import * as proto from './support/proto';
import * as control from './support/control';
import './support/setup';

describe('USB control request', function() {
  let dev = null;

  before('find a Particle USB device', function() {
    return usb.enumDevices().then((devs) => {
      if (devs.length == 0) {
        throw new Error('No Particle USB devices found');
      }
      dev = devs[0]; // Use the first device in the list
      dev.encoding = 'utf8'; // Enable text mode
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

  describe('asynchronous request protocol', function() {
    it('unsupported service request should be rejected', function() {
      const setup = { // Setup packet
        bmRequestType: proto.bmRequestType.DEVICE_TO_HOST,
        bRequest: 0, // Invalid service request
        wIndex: control.RequestType.CUSTOM_1, // Request type
        wValue: 0,
        wLength: usb.MIN_WLENGTH
      };
      return dev.controlTransfer(setup).should.be.rejectedWith(usb.UsbError);
    });
  });

  describe('asynchronous request', function() {
    it('echo request should succeed', function() {
      const reqData = 'hello';
      return control.sendRequest(dev, control.RequestType.APP_CUSTOM, reqData).should.become({
        result: control.Result.OK,
        data: 'hello'
      });
    });
  });
});
