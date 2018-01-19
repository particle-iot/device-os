import { MIN_WLENGTH } from './usb';

// Service request types
export const ServiceType = {
  INIT: 1,
  CHECK: 2,
  SEND: 3,
  RECV: 4,
  RESET: 5
};

// Field flags
export const FieldFlag = {
    STATUS: 0x01,
    ID: 0x02,
    SIZE: 0x04,
    RESULT: 0x08
};

// Status codes
export const Status = {
  OK: 0,
  ERROR: 1,
  PENDING: 2,
  BUSY: 3,
  NO_MEMORY: 4,
  NOT_FOUND: 5
};

// Values of the `bmRequestType` field used by the protocol
export const bmRequestType = {
  HOST_TO_DEVICE: 0x40, // 01000000b (direction: host-to-device; type: vendor; recipient: device)
  DEVICE_TO_HOST: 0xc0 // 11000000b (direction: device_to_host; type: vendor; recipient: device)
};

export class SetupPacket {
  constructor(serviceType) {
    if (serviceType == ServiceType.SEND) { // SEND is the only host-to-device service request defined by the protocol
      this.bmRequestType = bmRequestType.HOST_TO_DEVICE;
      this.wLength = 0;
    } else {
      this.bmRequestType = bmRequestType.DEVICE_TO_HOST;
      this.wLength = MIN_WLENGTH;
    }
    this.bRequest = serviceType;
    this.wValue = 0;
    this.wIndex = 0;
  }

  // INIT
  requestType(type) {
    this.bRequest.should.equal(ServiceType.INIT);
    this.wIndex = type;
    return this;
  }

  // CHECK, SEND, RECV, RESET
  requestId(id) {
    this.bRequest.should.be.oneOf([ServiceType.CHECK, ServiceType.SEND, ServiceType.RECV, ServiceType.RESET]);
    this.wIndex = id;
    return this;
  }

  // INIT, SEND, RECV
  payloadSize(size) {
    this.bRequest.should.be.oneOf([ServiceType.INIT, ServiceType.SEND, ServiceType.RECV]);
    if (this.bRequest == ServiceType.INIT) {
      this.wValue = size;
    } else {
      this.wLength = size;
    }
    return this;
  }
}

export function setupPacket(serviceType) {
  return new SetupPacket(serviceType);
}

// Parses service reply data
export function parseServiceReply(data) {
  const rep = {};
  let offs = 0;
  // Field flags (4 bytes)
  rep.flags = data.readUInt32LE(offs);
  offs += 4;
  // Status code (2 bytes)
  rep.status = data.readUInt16LE(offs);
  offs += 2;
  // Request ID (2 bytes, optional)
  if (rep.flags & FieldFlag.ID) {
    rep.id = data.readUInt16LE(offs);
    offs += 2;
  }
  // Payload size (4 bytes, optional)
  if (rep.flags & FieldFlag.SIZE) {
    rep.size = data.readUInt32LE(offs);
    offs += 4;
  }
  // Result code (4 bytes, optional)
  if (rep.flags & FieldFlag.RESULT) {
    rep.result = data.readInt32LE(offs); // signed int
    offs += 4;
  }
  return rep;
}
