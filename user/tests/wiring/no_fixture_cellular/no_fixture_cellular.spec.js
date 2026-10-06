suite('No fixture cellular');

platform('cellular', 'msom');

// CLOUD_05 allows a 9 minute cloud connect before and after pulling the rug
timeout(20 * 60 * 1000);