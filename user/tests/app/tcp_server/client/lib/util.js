const util = require('util');
const _ = require('lodash');

const Promise = require('bluebird');

// Variant of Promise.mapSeries() that doesn't stop on errors
exports.mapAll = (input, func) => {
  return new Promise((resolve, reject) => {
    let error = null;
    Promise.mapSeries(input, (val) => {
      return func(val)
        .catch((err) => {
          error = error || err;
        });
    })
    .finally(() => {
      if (error) {
        reject(error);
      } else {
        resolve();
      }
    });
  });
};

_.extend(exports, util);
