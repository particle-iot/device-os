const parser = require('particle-diagnostic-parser').DiagnosticParser;
const util = require('util');
const Addr2Line = require('addr2line').Addr2Line;
const Promise = require('bluebird');
const os = require('os');
const Table = require('cli-table');
const exec = require('child_process').exec;

const checkpointTableHead = ['Addr', 'Text', 'Source', 'Function'];
const stackTraceTableHead = ['Addr', 'Source', 'Function'];

function formatFileLine(file, line) {
  if (file && line) {
    return util.format('%s:%d', file, line);
  }
  return '';
};

function fixupJson(json) {
  // We currently have a limit on maximum USB request size set to 512, so JSON data might get truncated.
  // We are using this simple function to at least try to fixup the truncated JSON
  // Taken from https://gist.github.com/kekscom/10925007
  var chunk = json;

  var m, q = false;
  var stack = [];

  while (m = chunk.match(/[^\{\[\]\}"]*([\{\[\]\}"])/)) {
    switch (m[1]) {
      case '{':
        stack.push('}');
      break;
      case '[':
        stack.push(']');
      break;

      case '}':
      case ']':
        stack.pop();
      break;

      case '"':
        if (!q) {
          q = true;
          stack.push('"');
        } else {
          q = false;
          stack.pop();
        }
      break;
    }
    chunk = chunk.substring(m[0].length);
  }

  if (chunk[chunk.length-1] === ':') {
    json += '""';
  }

  while (stack.length) {
    json += stack.pop();
  }

  return json;
};

function execPromise(args, opts) {
  opts = opts || {};
  return new Promise((resolve, reject) => {
    const proc = exec(args, opts, (err, stdout, stderr) => {
      if (err) {
        return reject(err);
      } else {
        return resolve({stdout: stdout, stderr: stderr});
      }
    });
  });
};

function dump(data) {
  return new Promise((resolve, reject) => {
    let str = '';
    data.forEach((thread) => {
      str += util.format('Thread: %s [%d] %s', thread.thread, thread.id, os.EOL);
      if ('checkpoint' in thread) {
        str += 'Checkpoint:' + os.EOL;
        let table = new Table({ head: checkpointTableHead });
        table.push([thread.checkpoint.address,
                    thread.checkpoint.text || '',
                    formatFileLine(thread.checkpoint.filename, thread.checkpoint.line),
                    thread.checkpoint.function || '']);
        str += table.toString() + os.EOL;
      }
      if ('stacktrace' in thread) {
        str += 'Stacktrace:' + os.EOL;
        let table = new Table({ head: stackTraceTableHead });
        thread.stacktrace.forEach((sitem) => {
          table.push([sitem.address,
                      formatFileLine(sitem.filename, sitem.line),
                      sitem.function || '']);
        });
        str += table.toString() + os.EOL;
      }
      str += os.EOL;
    });
    resolve(str);
  });
};

var elfs = [];

function generateElfs() {
  const platform = process.env.PLATFORM;
  const pwd = process.env.SCRIPT_DIR;
  const genPath = function(pwd, platform, target, file) {
    return pwd + '/../../../../../build/target/' + target + '/' + platform + '/' + file;
  };
  switch(platform.toLowerCase()) {
    case 'photon':
      elfs = [
        genPath(pwd, 'platform-6-m', 'system-part1', 'system-part1.elf'),
        genPath(pwd, 'platform-6-m', 'system-part2', 'system-part2.elf'),
        genPath(pwd, 'platform-6-m', 'user-part', 'checkpoint.elf')
      ];
    case 'p1':
      elfs = [
        genPath(pwd, 'platform-8-m', 'system-part1', 'system-part1.elf'),
        genPath(pwd, 'platform-8-m', 'system-part2', 'system-part2.elf'),
        genPath(pwd, 'platform-8-m', 'user-part', 'checkpoint.elf')
      ];
    break;
    case 'electron':
      elfs = [
        genPath(pwd, 'platform-10-m', 'system-part1', 'system-part1.elf'),
        genPath(pwd, 'platform-10-m', 'system-part2', 'system-part2.elf'),
        genPath(pwd, 'platform-10-m', 'system-part3', 'system-part3.elf'),
        genPath(pwd, 'platform-10-m', 'user-part', 'checkpoint.elf')
      ];
  }
};

generateElfs();

const resolver = new Addr2Line(elfs, {prefix: 'arm-none-eabi-', basenames: true});
const p = new parser((addr) => {
  return resolver.resolve(addr);
});

function help() {
  console.log('Commands:');
  console.log(' - update: Forces a full diagnostic dump of a running device (includes stacktraces)');
  console.log(' - get: Retrieves current diagnostic info from a running device');
  console.log(' - getlast: Retrieves previous boot diagnostic info from a running device');
  process.exit(1);
};

function main() {
  const args = process.argv.splice(process.execArgv.length + 2);
  let v = 1;
  switch(args[0]) {
    case 'update':
    execPromise('send_usb_req -i 101 -d out', { shell: '/bin/bash' }).then(() => {
      console.log('Done');
      process.exit(0);
    });
    break;
    case 'getlast':
    v = 0;
    case 'get':
    execPromise(util.format('send_usb_req -i 100 -v %d', v), { shell: '/bin/bash' }).then((s) => {
      let j = fixupJson(s.stdout.trim());
      p.expand(j).then((res) => {
        dump(res).then((res) => {
          console.log(res);
          process.exit(0);
        });
      });
    });
    break;
    default:
    help();
    return;
  }
};

main();
