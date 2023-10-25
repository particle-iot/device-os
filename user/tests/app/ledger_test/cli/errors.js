const fs = require('fs');
const path = require('path');

const deviceOsErrors = (() => {
  const headerFile = 'services/inc/system_error.h';
  try {
    const src = fs.readFileSync(path.join(__dirname, '../../../../..', headerFile), 'utf-8');
    let r = src.match(/#define\s+SYSTEM_ERRORS\b.*?\n/);
    if (!r) {
      throw new Error();
    }
    let line = r[0];
    let pos = r.index + line.length;
    const lines = [line];
    while (/^.*?\\\s*?\n$/.test(line)) { // Ends with '\' followed by a newline
      line = src.slice(pos, src.indexOf('\n', pos) + 1);
      pos += line.length;
      lines.push(line);
    }
    const errors = new Map();
    for (const line of lines) {
      const matches = line.matchAll(/\(\s*(\w+)\s*,\s*"(.*?)"\s*,\s*(-\d+)\s*\)/g);
      for (const m of matches) {
        const name = m[1];
        const message = m[2];
        const code = Number.parseInt(m[3]);
        if (Number.isNaN(code)) {
          throw new Error();
        }
        errors.set(code, { name, message });
      }
    }
    if (!errors.size) {
      throw new Error();
    }
    return errors;
  } catch (err) {
    console.error(`Failed to parse ${headerFile}`);
    return new Map();
  }
})();

function deviceOsErrorCodeToString(code) {
  const err = deviceOsErrors.get(code);
  if (!err) {
    return `Unknown error (${code})`;
  }
  return `${err.message} (${err.name})`;
}

module.exports = {
  deviceOsErrorCodeToString
};
