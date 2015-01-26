require("net");
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

var net = require("net");
var settings = require("./settings.js");
var SoftAPMock = require("./softapmock.js");

mock = new SoftAPMock();
net.createServer(mock.server()).listen(settings.tcp_port, settings.tcp_host, function() {
    console.log('Server listening on ' + settings.tcp_host +':'+ settings.tcp_port);
});





