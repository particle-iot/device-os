import usb from 'usb';
import VError from 'verror';
import * as _ from 'lodash';
import * as util from 'util';

// Regular expressions matching vendor/product IDs of the Particle USB devices
const usbIdRegex = [
  // Core
  {
    idVendor: /1d50/,
    idProduct: /607[df]/
  },
  // Photon/P1/Electron
  {
    idVendor: /2b04/,
    idProduct: /[cd]00?/
  }
];

// Mask for a bit encoding the direction of the data stage transfer in the `bmRequestType` field
const BM_REQUEST_TYPE_DIRECTION_MASK = 0x80; // 10000000b

// Minimum length of the data stage for high-speed USB devices
export const MIN_WLENGTH = 64;

export class UsbError extends VError {
  constructor(cause) {
    super(cause, 'USB error');
  }
}

// Class representing a Particle USB device
export class Device {
  constructor(usbDev) {
    this._usbDev = usbDev;
  }

  open() {
    return new Promise((resolve, reject) => {
      try {
        this._usbDev.open();
        resolve();
      } catch (err) {
        reject(new UsbError(err));
      }
    });
  }

  close() {
    return new Promise((resolve, reject) => {
      try {
        this._usbDev.close();
        resolve();
      } catch (err) {
        reject(new UsbError(err));
      }
    });
  }

  // Performs a control transfer
  controlTransfer(setup, data) {
    return new Promise((resolve, reject) => {
      let dataOrLength = null;
      if (setup.bmRequestType & BM_REQUEST_TYPE_DIRECTION_MASK) {
        dataOrLength = setup.wLength; // IN transfer
      } else if (_.isString(data)) {
        dataOrLength = Buffer.from(data);
      } else {
        dataOrLength = data;
      }
      this._usbDev.controlTransfer(setup.bmRequestType, setup.bRequest, setup.wValue, setup.wIndex, dataOrLength,
          (err, data) => {
        if (err) {
          return reject(new UsbError(err));
        }
        resolve(data);
      });
    });
  }

  get usbDevice() {
    return this._usbDev;
  }
};

export function enumDevices() {
  return new Promise((resolve, reject) => {
    try {
      const devs = [];
      const usbDevs = usb.getDeviceList();
      for (let usbDev of usbDevs) {
        const descr = usbDev.deviceDescriptor;
        const idVendor = descr.idVendor.toString(16);
        const idProduct = descr.idProduct.toString(16);
        for (let regex of usbIdRegex) {
          if (idVendor.match(regex.idVendor) && idProduct.match(regex.idProduct)) {
            devs.push(new Device(usbDev));
            break;
          }
        }
      }
      resolve(devs);
    } catch (err) {
      reject(new UsbError(err));
    }
  });
}
