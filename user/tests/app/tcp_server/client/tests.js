const client = require('./lib/client');
const test = require('./lib/test');
const _ = require('lodash');

const Client = client.Client;
const EchoClient = client.EchoClient;
const Promise = require('bluebird');

const DEFAULT_PORT = test.DEFAULT_PORT;

// Maximum number of TCP connections supported by the device (as defined by LwIP's MEMP_NUM_TCP_PCB option)
const MAX_CONNECTIONS = 5;
// Maxumum number of TCP servers supported by the device (as defined by WICED_MAXIMUM_NUMBER_OF_SERVERS macro)
const MAX_SERVERS = 5;

exports.EchoTest = (test) => {
  const client = test.newClient(EchoClient);
  // Start server
  return test.startServer('EchoServer')
  // Connect to server
  .then(() => {
    return client.connect();
  })
  // Send random message and receive it back
  .then(() => {
    return client.echo();
  });
};

exports.MaxConnectionsTest = (test) => {
  let clients = [];
  // Start server
  return test.startServer('EchoServer')
  // Establish maximum number of connections
  .then(() => {
    for (let i = 0; i < MAX_CONNECTIONS; ++i) {
      clients.push(test.newClient(EchoClient));
    }
    return Promise.each(clients, (client) => {
      return client.connect();
    });
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
  // Check that all connections work as expected
  .then(() => {
    return Promise.each(clients, (client) => {
      return client.echo();
    });
  })
}

exports.ExtraConnectionTest = (test) => {
  let clients = [];
  let extraClient = null;
  let extraClientPromise = null;
  // Start server
  return test.startServer('EchoServer')
  // Establish maximum number of connections
  .then(() => {
    for (let i = 0; i < MAX_CONNECTIONS; ++i) {
      clients.push(test.newClient(EchoClient));
    }
    return Promise.each(clients, (client) => {
      return client.connect();
    });
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
    // Check that all connections work as expected
    clients.push(extraClient);
    return Promise.each(clients, (client) => {
      return client.echo();
    });
  });
};

exports.MaxServersTest = (test) => {
  // Start maximum number of servers
  let serverPromises = [];
  for (let i = 0; i < MAX_SERVERS; ++i) {
    serverPromises.push(test.startServer('EchoServer', DEFAULT_PORT + i));
  }
  return Promise.all(serverPromises)
  // Try to start one more server
  .then(() => {
    return test.startServer('EchoServer', DEFAULT_PORT + MAX_SERVERS)
      .then(() => {
        throw new Error('Extra server instance has been started unexpectedly');
      }, () => {
        // Ignore error silently
      });
  })
  // Check that all servers work as expected
  .then(() => {
    var clients = [];
    for (let i = 0; i < MAX_SERVERS; ++i) {
      clients.push(test.newClient(EchoClient, DEFAULT_PORT + i));
    }
    return Promise.each(clients, (client) => {
      return client.connect().then(() => {
          return client.echo();
      });
    })
  })
};

// FIXME: This test may crash the device since there's a bug in socket HAL, which destroys all
// sockets on network disconnection, leaving invalid socket handles at the application side
exports.NoServerSocketLeakOnNetworkDisconnectTest = (test) => {
  // Start maximum number of servers
  let serverPromises = [];
  for (let i = 0; i < MAX_SERVERS; ++i) {
    serverPromises.push(test.startServer('SimpleServer', DEFAULT_PORT + i));
  }
  return Promise.all(serverPromises)
  // Disconnect from network
  .then(() => {
    return test.disconnectFromNetwork();
  })
  .delay(1000)
  // Stop all servers
  .then(() => {
    return test.stopServers();
  })
  // Connect to network
  .then(() => {
    return test.connectToNetwork();
  })
  // Try to start servers again
  .then(() => {
    serverPromises = [];
    for (let i = 0; i < MAX_SERVERS; ++i) {
      serverPromises.push(test.startServer('SimpleServer', DEFAULT_PORT + i));
    }
    return Promise.all(serverPromises);
  })
};

exports.StoppedServerDoesntAffectOtherServersTest = (test) => {
  const PORT1 = DEFAULT_PORT;
  const PORT2 = DEFAULT_PORT + 1;
  const checkServer = (port) => {
    const client = test.newClient(EchoClient, port);
    return client.connect()
      .then(() => {
        return client.echo();
      })
      .then(() => {
        return client.disconnect();
      });
  };
  // Start 1st server
  return test.startServer('EchoServer', 'server1', PORT1)
  // Start 2nd server
  .then(() => {
    return test.startServer('EchoServer', 'server2', PORT2);
  })
  // Check that both servers work as expected
  .then(() => {
    return checkServer(PORT1);
  })
  .then(() => {
    return checkServer(PORT2);
  })
  // Stop 1st server
  .then(() => {
    return test.stopServer('server1');
  })
  // Check that 2nd server still works normally
  .then(() => {
    return checkServer(PORT2);
  })
};

exports.StoppingServerWithActiveConnectionsTest = (test) => {
  let clients = [];
  let clientPromises = [];
  for (let i = 0; i < MAX_CONNECTIONS; ++i) {
    clients.push(test.newClient(EchoClient));
  }
  // Start server
  return test.startServer('SimpleServer', 'server1')
  // Make several connections
  .then(() => {
    return Promise.each(clients, (client) => {
      return client.connect();
    });
  })
  // Stop server
  .then(() => {
    clientPromises = _.map(clients, (client) => {
      return client.waitUntilClosed();
    });
    return test.stopServer('server1');
  })
  // All connections should have been closed by the server
  .then(() => {
    return Promise.all(clientPromises);
  });
};

exports.TcpClientClosesOnDestructionTest = (test) => {
  const client = test.newClient();
  let clientPromise = null;
  // Start server
  return test.startServer('TcpClientClosesOnDestructionTestServer')
  // Connect to server
  .then(() => {
    return client.connect();
  })
  // Internally, TCPServer holds a reference to last accepted client connection, so here we make
  // another connection to ensure that the TCPClient instance representing first connection gets
  // destroyed
  .then(() => {
    clientPromise = client.waitUntilClosed();
    return test.newClient().connect();
  })
  // Wait until first connection is closed
  .then(() => {
    return clientPromise;
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
  for (let i = 0; i < TOTAL_CONNS; ++i) {
    const client = test.newClient(EchoClient);
    // Longer client timeout can be useful when number of concurrent connection attempts is greater
    // than maximum number of simultaneous connections supported by the device
    client.timeout = 60000;
    clients.push(client);
  }
  let failedConns = 0;
  // Start server
  return test.startServer('EchoServer')
  // Start connecting to the server
  .then(() => {
    return Promise.map(clients, (client) => {
      return Promise.delay(Math.random() * 500)
        .then(() => {
          return client.connect()
        })
        .then(() => {
          return client.echo();
        })
        .delay(Math.random() * 500)
        .then(() => {
          return client.disconnect();
        })
        .catch((err) => {
          console.error('Connection failed: %s', err.message);
          if (++failedConns >= MAX_FAILED_CONNS) {
            throw err; // Fatal error
          }
        });
    }, { concurrency: CONCURRENT_CONNS });
  })
};
