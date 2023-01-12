suite('Test runner mailbox');

platform('gen3');
systemThread('enabled');

let device = null;

before(function() {
    device = this.particle.devices[0];
    device.on('mailbox', (msg) => {
        console.log('mailbox msg', msg);
    });
});

test('01_mailbox', async function () {
    expect(device.mailBox).to.eql([
        {t: 2, d: 'test'},
        {t: 2, d: 'deadbeef'},
        {t: 2, d: '\u0000\u0001\u0002\u0003'}
    ]);
});

test('02_mailbox_reset', async function () {
    expect(device.mailBox).to.eql([
        {t: 1} // RESET_PENDING
    ]);
});

test('03_mailbox', async function () {
    expect(device.mailBox).to.eql([
        {t: 2, d: 'test'},
        {t: 2, d: 'deadbeef'},
        {t: 2, d: '\u0000\u0001\u0002\u0003'}
    ]);
});
