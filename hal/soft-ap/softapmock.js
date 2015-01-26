/**
 *    Copyright (C) 2015 Spark Labs, Inc. All rights reserved. -  https://www.spark.io/
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    You can download the source here: https://github.com/spark/spark-server
 */

var HashMap = require('hashmap').HashMap;
var settings = require("./settings");

var SoftAPMock = function() {
    this.client = new HashMap();
    this.device = settings;  // for now use global values. later we might want to have different fixtures
    this.commands =  {
        "device-id": this.cmd_device_id.bind(this),
        "scan-ap": this.cmd_scan_ap.bind(this),
        "configure-ap": this.cmd_configure_ap.bind(this),
        "connect-ap": this.cmd_connect_ap.bind(this),
        "public-key": this.cmd_public_key.bind(this),
        "set": this.cmd_set.bind(this)
    }
};

SoftAPMock.prototype = {

    server: function() {
        var self = this;
        return function(sock) { self._socket_handler(sock);  }
    },

    _connect: function(sock) {
        console.log("received new connection");
        return this.client.set(sock, { data: '', complete: false, sock: sock });
    },

    _handle_request: function(client) {
        // this is a crude parser. The request is parsed once three newlines have been spotted
        var parts = client.data.split('\n');
        var processed = false;
        if (parts.length>=3) {
            var cmd = parts[0];
            var length = parts[1];
            var body = parts.slice(2).join('\n');

            console.log("received request:");
            console.log("cmd: "+cmd);
            console.log("length: "+length);
            console.log("body: "+body);
            cmd in this.commands && this.commands[cmd](client.sock, body);
            processed = true;
        }
        return processed;
    },

    _data: function(sock, data) {
        var client = this.client.get(sock);
        client.data += data;
        if (!client.complete) {
            client.complete = this._handle_request(client);
        }
    },

    _close: function(sock, data) {
        this._data(sock, data);
        this.client.remove(sock);
    },

    _socket_handler: function(sock) {
        // Add a 'data' event handler to this instance of socket
        var self = this;

        self._connect(sock);

        sock.on('data', function(data) {
            self._data(sock, data);
        });

        // Add a 'close' event handler to this instance of socket
        sock.on('close', function(data) {
            self._close(sock, data);
        });
    },

    _send_response: function(sock, response) {
        sock.write(JSON.stringify(response));
    },

    _parse_request_json: function(body) {
        return JSON.parse(body);
    },

    cmd_device_id: function(sock, body) {
        this._send_response(sock, { id: this.device.device_id, c: this.device.claimed } );
    },

    cmd_scan_ap: function(sock, body) {
        this._send_response(sock, { scans: this.device.scans } );
    },

    cmd_configure_ap: function(sock, body) {
        var request = this._parse_request_json(body);
        // not sure what to do here - what would be useful in a mock?
        // perhaps decode the encrypted password and log that?
        this._send_response(sock, { r: 0 } );
    },

    cmd_connect_ap: function(sock, body) {
        sock.clos();        // simulate switching networks by closing the socket
    },

    cmd_public_key: function(sock, body) {
        this._send_response(sock, {r:0, b:this.device.public_key});
    },

    cmd_set: function(sock, body) {
        // the set value presently supported by the device is the claim code.
        this._send_response(sock, {r:0} );
    }

};

module.exports = SoftAPMock;
