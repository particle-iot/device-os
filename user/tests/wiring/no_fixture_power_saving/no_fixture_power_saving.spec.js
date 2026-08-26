suite('No fixture power saving');

platform('boron','bsom','esomx');
timeout(32 * 60 * 1000);


let api = null;
let auth = null;
let device = null;
let deviceId = null;
let limits = null;
let skipTest = false;
let returnVal = 12345;
let wakeCall = null;
let sleepingDeferred = null;

// Resolved from the 'mailbox' handler when the device says it is about to sleep. The runner emits
// that event in real time from inside waitTest()'s polling loop, so it arrives while the device
// test is still running.
function waitForSleeping() {
    let resolve;
    const promise = new Promise((res) => { resolve = res; });
    sleepingDeferred = { resolve };
    return promise;
}

async function delayMs(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function getDeviceFunctionsWithRetries({ deviceId, auth, expectedFuncNum = 0, retries = 10, delay = 1000 } = {}) {
    let lastError;
    for (let i = 0; i < retries; i++) {
        try {
            const resp = await api.getDevice({ deviceId, auth });
            const funcs = resp.body.functions;
            if (expectedFuncNum > 0 && funcs.length !== expectedFuncNum) {
                throw new Error('Number of functions returned from device does not match expected');
            }
            return resp;
        } catch (e) {
            lastError = e;
        }
        await delayMs(i * delay);
    }
    if (lastError) {
        // console.log(lastError);
        throw lastError;
    }
    throw new Error('Error fetching functions from device');
}

before(function() {
    api = this.particle.apiClient.instance;
    auth = this.particle.apiClient.token;
    device = this.particle.devices[0];
    deviceId = device.id;

    // This code will run after the appropriate mailbox message is received.
    device.on('mailbox', async (msg) => {
        if (msg.d === "skip_test") {
            skipTest = true;
            return;
        }
        if (msg.d === "sleeping" && sleepingDeferred) {
            sleepingDeferred.resolve();
            sleepingDeferred = null;
            return;
        }
        if (msg.d) {
            console.log(msg.d); // device-side findings, e.g. the low power result
        }
    });
});

after(function() {
    device.removeAllListeners('mailbox');
});

test('POWER_SAVING_00_setup', async function() {
    // See power_saving_mode.cpp
});

test('POWER_SAVING_01_particle_publish_publishes_an_event_after_low_power_active', async function() {
    if (skipTest) {
        return;
    }

    const data = await this.particle.receiveEvent('my_event_low_power');
    expect(data).to.equal('event data low power');
});

test('POWER_SAVING_02_register_function_and_connect_to_cloud', async function() {
    if (skipTest) {
        return;
    }

    const expectedFuncs = ['fnlp1'];
    const resp = await getDeviceFunctionsWithRetries({ deviceId, auth, expectedFuncNum: expectedFuncs.length });
    const funcs = resp.body.functions;
    expect(funcs).to.include.members(expectedFuncs);
});

test('POWER_SAVING_03_call_function_and_check_return_value_after_low_power_active', async function() {
    if (skipTest) {
        return;
    }

    let resp = await api.callFunction({ deviceId, name: 'fnlp1', argument: 'argument string low power', auth });
    // console.log(resp.body.return_value + ' == ' + returnVal);
    expect(resp.body.return_value).to.equal(returnVal);
});

test('POWER_SAVING_04_check_function_argument_value', async function () {
    if (skipTest) {
        return;
    }

    // See power_saving_mode.cpp
});

test('POWER_SAVING_05_check_current_thread', async function () {
    if (skipTest) {
        return;
    }

    // Arm the wake here, deliberately not awaited. The runner runs each device test inside an
    // awaited beforeEach hook, so nothing in test 06_1's own body can run while the device sleeps.
    // The call has to already be in flight before that hook starts. Gating on the device's own
    // event rather than a fixed offset keeps this correct if the pre-sleep work changes length.
    const sleeping = waitForSleeping();
    wakeCall = (async () => {
        await sleeping;
        // console.log('device is asleep, waiting 30s before waking it');
        await delayMs(30000);
        // console.log('waking up device with a function call');
        return api.callFunction({ deviceId, name: 'fnlp1', argument: 'argument string low power sleep', auth });
    })();
    wakeCall.catch(() => {}); // keep node quiet until 06_1 awaits it
});

test('POWER_SAVING_06_system_sleep_with_configuration_object_ultra_low_power_mode_wake_by_network_1', async function() {
    if (skipTest) {
        return;
    }

    // Armed in test 05's body; by now it has already woken the device.
    const resp = await wakeCall;
    // console.log(resp.body.return_value + ' == ' + (returnVal + 1));
    expect(resp.body.return_value).to.equal(returnVal + 1);
});


test('POWER_SAVING_06_system_sleep_with_configuration_object_ultra_low_power_mode_wake_by_network_2', async function() {
    if (skipTest) {
        return;
    }

    // See power_saving_mode.cpp
});

test('POWER_SAVING_07_check_function_argument_value', async function () {
    if (skipTest) {
        return;
    }

    // See power_saving_mode.cpp
});

test('POWER_SAVING_99_cleanup', async function() {
    device.removeAllListeners('mailbox');
    skipTest = false;
});
