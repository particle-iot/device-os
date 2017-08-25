const tests = require('./tests');
const test = require('./lib/httptest');
const util = require('./lib/util');
const _ = require('lodash');

const DeviceControl = require('./lib/device').DeviceControl;
const Test = test.Test;
const TestError = test.TestError;

let passedCount = 0;

// Wrap all test functions into instances of the Test class
const allTests = _.map(tests, (func, name) => {
  return new Test(name, func);
});

// Run all tests
return util.mapAll(allTests, (test) => {
  console.log('\nRunning %s...', test.name);
  return test.run()
    .then(() => {
      console.log('PASSED');
      ++passedCount;
    })
    .catch((err) => {
      console.error('FAILED: %s', err.message);
      throw err;
    });
})
.then(() => {
  console.log('\nPASSED: All %d tests passed', allTests.length);
})
.catch((err) => {
  if (!(err instanceof TestError)) {
    console.error(err.message);
  }
  console.error('\nFAILED: %d of %d tests failed', allTests.length - passedCount, allTests.length);
  process.exit(1);
});
