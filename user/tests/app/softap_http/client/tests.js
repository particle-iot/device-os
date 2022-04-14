const client = require('./lib/client');
const httpclient = require('./lib/httpclient');
const test = require('./lib/test');
const _ = require('lodash');

const Client = client.Client;
const EchoClient = httpclient.HttpEchoClient;
const QueryEchoClient = httpclient.HttpQueryEchoClient;
const Promise = require('bluebird');

// Maximum number of TCP connections supported by the device (as defined by LwIP's MEMP_NUM_TCP_PCB option)
const MAX_CONNECTIONS = 10;

// Maximum number of HTTP connections supported
const MAX_HTTP_CONNECTIONS = 10;

exports.EchoTest = (test) => {
  const client = test.newClient(EchoClient);

  // Connect to softap http server
  return client.connect()
  // Send random request and receive it back
  .then(() => {
    return client.echo();
  });
};

exports.QueryStringTest = (test) => {
  const client = test.newClient(QueryEchoClient);

  // Connect to softap http server
  return client.connect()
  // Send random request with query string and receive it back
  .then(() => {
    return client.echo();
  });
}

exports.MultipleEchoTest = (test) => {
  const client = test.newClient(EchoClient);

  // Connect to softap http server
  return client.connect()
  // Send random request and receive it back
  .then(() => {
    return client.echo();
  })
  .then(() => {
    return client.echo();
  })
  .then(() => {
    return client.echo();
  });
};

exports.MaxConnectionsTest = (test) => {
  let clients = [];
  // Establish maximum number of connections
  for (let i = 0; i < MAX_CONNECTIONS; ++i) {
    clients.push(test.newClient(EchoClient));
  }
  return Promise.each(clients, (client) => {
    return client.connect();
  })
  // Try to establish one more connection
  .then(() => {
    const client = test.newClient(EchoClient);
    client.timeout = 1000; // Use shorter timeout
    return client.connect()
      .then(() => {
        throw new Error('Extra connection has been established unexpectedly');
      }, () => {
        // Ignore error silently
      });
  })
  // Check that MAX_HTTP_CONNECTIONS connections work as expected
  .then(() => {
    return Promise.each(clients.slice(0, MAX_HTTP_CONNECTIONS), (client) => {
      return client.echo();
    });
  })
  // Close MAX_HTTP_CONNECTIONS and check that remaining work
  .then(() => {
    return Promise.each(clients.slice(0, MAX_HTTP_CONNECTIONS), (client) => {
      return client.disconnect();
    });
  })
  .then(() => {
    return Promise.each(clients.slice(MAX_HTTP_CONNECTIONS), (client) => {
      return client.echo();
    });
  });
}

exports.ExtraConnectionTest = (test) => {
  let clients = [];
  let extraClient = null;
  let extraClientPromise = null;

  // Establish maximum number of connections
  for (let i = 0; i < MAX_CONNECTIONS; ++i) {
    clients.push(test.newClient(EchoClient));
  }
  return Promise.each(clients, (client) => {
    return client.connect();
  })
  // Try to establish one more connection
  .then(() => {
    extraClient = test.newClient(EchoClient);
    extraClientPromise = extraClient.connect();
  })
  .delay(1000)
  // Close one of the previously established connections
  .then(() => {
    if (extraClient.connected) {
      throw new Error('Extra connection has been established unexpectedly');
    }
    let client = clients.shift();
    return client.disconnect();
  })
  // Pending extra connection should now succeed
  .then(() => {
    return extraClientPromise;
  })
  .then(() => {
    // Check that MAX_HTTP_CONNECTIONS connections work as expected
    clients.push(extraClient);
    return Promise.each(clients.slice(0, MAX_HTTP_CONNECTIONS), (client) => {
      return client.echo();
    });
  })
  // Close MAX_HTTP_CONNECTIONS and check that remaining work
  .then(() => {
    return Promise.each(clients.slice(0, MAX_HTTP_CONNECTIONS), (client) => {
      return client.disconnect();
    });
  })
  .then(() => {
    return Promise.each(clients.slice(MAX_HTTP_CONNECTIONS), (client) => {
      return client.echo();
    });
  });
};

exports.StressTest = (test) => {
  const TOTAL_CONNS = 500; // Total number of connections
  const CONCURRENT_CONNS = MAX_CONNECTIONS; // Maximum number of concurrent connection attempts
  const MAX_FAILED_CONNS = 6; // Maximum allowed number of failed connections
  console.log('Total connections: %d', TOTAL_CONNS);
  console.log('Concurrent connection attempts: %d', CONCURRENT_CONNS);
  console.log('Allowed failed connections: %d', MAX_FAILED_CONNS);
  console.log('Connecting...'); // This test may take some time
  test.verbose = false; // Disable verbose logging
  let clients = [];
  let total = 0;
  for (let i = 0; i < TOTAL_CONNS; ++i) {
    const client = test.newClient(EchoClient);
    // Longer client timeout can be useful when number of concurrent connection attempts is greater
    // than maximum number of simultaneous connections supported by the device
    client.timeout = 60000;
    clients.push(client);
  }
  let failedConns = 0;
  // Start connecting to the server
  return Promise.map(clients, (client) => {
    return Promise.delay(Math.random() * 500)
      .then(() => {
        return client.connect()
      })
      .then(() => {
        return client.echo(100);
      })
      .then(() => {
        total++;
        return client.disconnect();
      })
      .delay(Math.random() * 500)
      .catch((err) => {
        console.error('Connection failed: %s', err.message);
        if (++failedConns >= MAX_FAILED_CONNS) {
          console.log('Total done: %d', total);
          throw err; // Fatal error
        }
      });
  }, { concurrency: CONCURRENT_CONNS });
};
