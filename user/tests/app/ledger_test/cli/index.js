#!/usr/bin/env node

const { name: PACKAGE_NAME, description: PACKAGE_DESC } = require('./package.json');

const { getDevices } = require('particle-usb');
const cbor = require('cbor');
const parseArgs = require('minimist');
const _ = require('lodash');

const fs = require('fs').promises;

const WRITE_BLOCK_SIZE = 1024;
const REQUEST_TYPE = 10; // ctrl_request_type::CTRL_REQUEST_APP_CUSTOM

const BinaryRequestType = {
  READ: 1,
  WRITE: 2
};

function printUsage() {
  console.log(`\
${PACKAGE_DESC}

Usage: ${PACKAGE_NAME} <command> [options...]`);
}

function scopeName(scope) {
  switch (scope) {
    case 0: return 'Unknown';
    case 1: return 'Device';
    case 2: return 'Product';
    case 3: return 'Owner';
    default: return `Unknown (${scope})`;
  }
}

function syncDirectionName(dir) {
  switch (dir) {
    case 0: return 'Unknown';
    case 1: return 'Device-to-cloud';
    case 2: return 'Cloud-to-device';
    default: return `Unknown (${dir})`;
  }
}

async function openDevice() {
  const devs = await getDevices();
  if (!devs.length) {
    throw new Error('No devices found');
  }
  if (devs.length !== 1) {
    throw new Error('Multiple devices found');
  }
  const dev = devs[0];
  await dev.open();
  return dev;
}

async function sendJsonRequest(dev, data) {
  data = Buffer.from(JSON.stringify(data));
  let resp = await dev.sendControlRequest(REQUEST_TYPE, data);
  if (resp.result < 0) {
    throw new Error(`Request failed: ${resp.result}`);
  }
  if (resp.data) {
    resp = JSON.parse(resp.data.toString());
  } else {
    resp = null;
  }
  return resp;
}

async function sendBinaryRequest(dev, type, data) {
  const buf = Buffer.alloc((data ? data.length : 0) + 4);
  buf.writeUInt32BE(type, 0);
  if (data) {
    data.copy(buf, 4);
  }
  let resp = await dev.sendControlRequest(REQUEST_TYPE, buf);
  if (resp.result < 0) {
    throw new Error(`Request failed: ${resp.result}`);
  }
  return resp.data || null;
}

async function list(dev) {
  const resp = await sendJsonRequest(dev, {
    cmd: 'list'
  });
  for (const name of resp) {
    console.log(name);
  }
}

async function remove(dev, args) {
  if (!args._.length) {
    throw new Error('Missing ledger name');
  }
  await sendJsonRequest(dev, {
    cmd: 'remove',
    name: args._[0]
  });
}

async function clear(dev) {
  await sendJsonRequest(dev, {
    cmd: 'clear',
  });
}

async function set(dev, args) {
  if (!args._.length) {
    throw new Error('Missing ledger name');
  }
  if (args._.length < 2) {
    throw new Error('Missing ledger data');
  }
  let data = args._[1];
  if (!data.trimStart().startsWith('{')) {
    try {
      data = await fs.readFile(data, { encoding: 'utf8' });
    } catch (err) {
      throw new Error('Failed to load file', { cause: err });
    }
  }
  try {
    data = JSON.parse(data);
  } catch (err) {
    throw new Error('Failed to parse JSON', { cause: err });
  }
  if (!_.isPlainObject(data)) {
    throw new Error('Ledger data is not an object');
  }
  data = await cbor.encodeAsync(data);
  await sendJsonRequest(dev, {
    cmd: 'set',
    name: args._[0],
    size: data.length
  });
  let offs = 0;
  while (offs < data.length) {
    const n = Math.min(data.length - offs, WRITE_BLOCK_SIZE);
    await sendBinaryRequest(dev, BinaryRequestType.WRITE, data.slice(offs, offs + n));
    offs += n;
  }
}

async function get(dev, args) {
  if (!args._.length) {
    throw new Error('Missing ledger name');
  }
  let resp = await sendJsonRequest(dev, {
    cmd: 'get',
    name: args._[0]
  });
  const size = resp.size;
  let data = Buffer.alloc(0);
  while (data.length < size) {
    resp = await sendBinaryRequest(dev, BinaryRequestType.READ);
    data = Buffer.concat([data, resp]);
  }
  data = await cbor.decodeFirst(data);
  data = JSON.stringify(data, null, 2);
  console.log(data);
}

async function info(dev, args) {
  if (!args._.length) {
    throw new Error('Missing ledger name');
  }
  let resp = await sendJsonRequest(dev, {
    cmd: 'info',
    name: args._[0]
  });
  console.log(`Scope: ${scopeName(resp.scope)}
Sync direction: ${syncDirectionName(resp.sync_direction)}
Data size: ${resp.data_size}
Last update: N/A
Last sync: N/A`);
}

async function connect(dev) {
  await sendJsonRequest(dev, {
    cmd: 'connect'
  });
}

async function disconnect(dev) {
  await sendJsonRequest(dev, {
    cmd: 'disconnect'
  });
}

async function autoConnect(dev, args) {
  let enabled = true;
  if (args._.length) {
    enabled = !!Number.parseInt(args._[0]);
  }
  await sendJsonRequest(dev, {
    cmd: 'auto_connect',
    enabled
  });
}

async function debug(dev, args) {
  let enabled = true;
  if (args._.length) {
    enabled = !!Number.parseInt(args._[0]);
  }
  await sendJsonRequest(dev, {
    cmd: 'debug',
    enabled
  });
}

async function reset(dev) {
  await sendJsonRequest(dev, {
    cmd: 'reset'
  });
}

async function runCommand(args) {
  if (!args._.length) {
    throw new Error('Missing command name');
  }
  let fn;
  const cmd = args._.shift();
  switch (cmd) {
    case 'list': fn = list; break;
    case 'remove': fn = remove; break;
    case 'clear': fn = clear; break;
    case 'set': fn = set; break;
    case 'get': fn = get; break;
    case 'info': fn = info; break;
    case 'connect': fn = connect; break;
    case 'disconnect': fn = disconnect; break;
    case 'auto-connect': fn = autoConnect; break;
    case 'debug': fn = debug; break;
    case 'reset': fn = reset; break;
    default:
      throw new Error(`Unknown command: ${cmd}`)
  }
  const dev = await openDevice();
  try {
    await fn(dev, args);
  } finally {
    await dev.close();
  }
}

async function runApp() {
  let ok = true;
  try {
    let args = process.argv.slice(2);
    args = parseArgs(args, {
      string: ['_'],
      boolean: ['help'],
      alias: {
        'help': 'h'
      },
      unknown: arg => {
        if (arg.startsWith('-')) {
          throw new RangeError(`Unknown argument: ${arg}`);
        }
      }
    });
    if (args.help) {
      printUsage();
    } else {
      await runCommand(args);
    }
  } catch (err) {
    console.error(err);
    ok = false;
  }
  process.exit(ok ? 0 : 1);
}

runApp();
