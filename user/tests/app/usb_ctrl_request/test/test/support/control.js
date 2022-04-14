import * as proto from './proto';
import VError from 'verror';
import Promise from 'bluebird';

// Polling interval for CHECK service requests
const CHECK_INTERVAL = 250;

function sendInitRequest(dev, type, dataSize) {
  const setup = proto.setupPacket(proto.ServiceType.INIT).requestType(type).payloadSize(dataSize);
  return dev.controlTransfer(setup).then((data) => {
    return proto.parseServiceReply(data);
  });
}

function sendCheckRequest(dev, id) {
  const setup = proto.setupPacket(proto.ServiceType.CHECK).requestId(id);
  return dev.controlTransfer(setup).then((data) => {
    return proto.parseServiceReply(data);
  });
}

function sendSendRequest(dev, id, data) {
  const setup = proto.setupPacket(proto.ServiceType.SEND).requestId(id).payloadSize(data.length);
  return dev.controlTransfer(setup, data);
}

function sendRecvRequest(dev, id, dataSize) {
  const setup = proto.setupPacket(proto.ServiceType.RECV).requestId(id).payloadSize(dataSize);
  return dev.controlTransfer(setup);
}

function waitForStatus(dev, id, status) {
  return sendCheckRequest(dev, id).then((serviceRep) => {
    const curStatus = serviceRep.status;
    if (curStatus == proto.Status.PENDING) {
      return Promise.delay(CHECK_INTERVAL).then(() => {
        return waitForStatus(dev, id, status);
      });
    } else if (curStatus != status) {
      throw new Error(`CHECK request failed, status code: ${curStatus} (expected code: ${status})`);
    }
    return serviceRep;
  });
}

// Request types (see `ctrl_request_type` enum defined in the system firmware)
export const RequestType = {
  APP_CUSTOM: 10, // Application-specific request
  DEVICE_ID: 20, // Get device ID (low-level request)
  SYSTEM_VERSION: 30, // Get firmware version (low-level request)
  MODULE_INFO: 90, // Get module info
  DIAGNOSTIC_INFO: 100 // Get diagnostic info
};

// Result codes (see `system_error_t` enum defined in the system firmware)
export const Result = {
  OK: 0
};

// Maximum number of concurrent requests supported by the device (see `USB_REQUEST_MAX_ACTIVE_COUNT`
// parameter defined in the system firmware)
export const MAX_ACTIVE_REQUESTS = 4;

// Sends an asynchronous request
export function sendRequest(dev, type, reqData) {
  const reqSize = reqData ? reqData.length : 0;
  return sendInitRequest(dev, type, reqSize)
      .then((serviceRep) => {
        const status = serviceRep.status;
        if (status == proto.Status.OK) {
          const id = serviceRep.id;
          if (reqSize > 0) { // Send request data
            return sendSendRequest(dev, id, reqData).then(() => {
              return id;
            });
          }
          return id;
        } else if (status == proto.Status.PENDING) {
          const id = serviceRep.id;
          return waitForStatus(dev, id, proto.Status.OK).then(() => {
            return sendSendRequest(dev, id, reqData).then(() => { // Send request data
              return id;
            });
          });
        } else {
          throw new Error(`INIT request failed, status code: ${status}`);
        }
      })
      .then((id) => {
        return waitForStatus(dev, id, proto.Status.OK).then((serviceRep) => {
          serviceRep.id = id; // Just in case
          return serviceRep;
        });
      })
      .then((serviceRep) => {
        const result = serviceRep.result;
        const repSize = serviceRep.size;
        if (repSize > 0) {
          const id = serviceRep.id;
          return sendRecvRequest(dev, id, repSize).then((data) => {
            return {
              result: result,
              data: data.toString()
            };
          });
        }
        return {
          result: result
        };
      })
}
