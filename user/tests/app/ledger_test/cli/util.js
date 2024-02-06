const cbor = require('cbor');

function randomInt(min, max) {
  if (min > max) {
    const m = min;
    min = max;
    max = m;
  }
  return min + Math.floor(Math.random() * (max - min + 1));
}

function randomString(len) {
  const alpha = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_';
  let s = '';
  for (let i = 0; i < len; ++i) {
    s += alpha.charAt(Math.floor(Math.random() * alpha.length));
  }
  return s;
}

async function randomObject(size) {
  let obj = {};
  let objSize;

  async function set(key, val) {
    obj[key] = val;
    objSize = (await cbor.encodeAsync(obj)).length; // Synchronous variant of this function doesn't work with large objects
  }
  async function remove(key) {
    delete obj[key];
    objSize = (await cbor.encodeAsync(obj)).length;
  }

  // Start with a small non-empty object
  let fillKey = randomString(1);
  await set(fillKey, '');
  const minObjSize = objSize;
  if (size < minObjSize) {
    throw new Error(`Minimum data size is ${minObjSize} bytes`);
  }
  let minEntryLen;
  let maxEntryLen;
  if (size < 200) {
    // Note that everything in this algorithm is completely arbitrary
    minEntryLen = 5;
    maxEntryLen = 20;
  } else if (size < 2000) {
    minEntryLen = 10;
    maxEntryLen = 50;
  } else {
    minEntryLen = 30;
    maxEntryLen = 120;
  }
  // Fill the object with random entries
  let key;
  do {
    const entryLen = randomInt(minEntryLen, maxEntryLen);
    const keyLen = randomInt(1, Math.ceil(entryLen / 2));
    do {
      key = randomString(keyLen);
    } while (key in obj);
    let val;
    const valLen = entryLen - keyLen;
    if (valLen > minObjSize && Math.random() < 0.25) {
      val = await randomObject(valLen);
    } else {
      val = randomString(valLen);
    }
    await set(key, val);
  } while (objSize <= size);
  await remove(key);
  // Object size is at or somewhat below the requested size now
  let fillVal = randomString(size - objSize);
  await set(fillKey, fillVal);
  if (objSize > size) {
    // Encoding the value length required some extra bytes
    fillVal = randomString(fillVal.length - (objSize - size));
    await set(fillKey, fillVal);
    if (objSize < size) {
      // The value length was encoded with fewer bytes again. The reserved key is 1 character long
      // so extending it by a few characters is unlikely to exceed the requested size
      await remove(fillKey);
      do {
        fillKey = randomString(size - objSize + 1);
      } while (fillKey in obj);
      await set(fillKey, fillVal);
    }
  }
  // Sanity check
  if (objSize != size) {
    throw new Error('Failed to generate object of specified size');
  }
  return obj;
}

function timestampToString(time) {
  return new Date(time).toISOString();
}

module.exports = {
  randomInt,
  randomString,
  randomObject,
  timestampToString
};
