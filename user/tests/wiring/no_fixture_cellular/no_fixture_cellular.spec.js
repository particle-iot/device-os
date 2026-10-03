suite('No fixture cellular');

platform('cellular', 'msom');

// CLOUD_05 pulls the rug twice if the AT interface dies, with a modem power cycle and a 9 minute
// reconnect in between. Roughly 41 minutes in the worst case on boron and b5som.
timeout(60 * 60 * 1000);