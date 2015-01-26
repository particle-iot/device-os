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

var default_tcp_port = 5609;


module.exports = {

    device_id: "220038000947333531303339",      // the device id
    claimed: false,                             // if this device is claimed or not
    public_key: "30819F300D06092A864886F70D010101050003818D0030818902818100C3F333868D89524B4ADD33AC523F07C518F50E66CF531F92F15DAEFB686591C2B3FB2BD11D0558D342E890391F5E92FF41996E69C0FCD040824C7C95F0B1EDF175535CFF3946013896EA1BD3D4F53C20E8D9ECCA505E149C75061C9B97625F36C930715EE06AB7C463AABF42E89B712474822306E0585742189EE365CCDE19150203010001",
    scans: [
        { "ssid":"ssid-name", "rssi":-30, "sec":1234, "ch":5, "mdr":54000 } /* ... */
    ],

    tcp_host: "127.0.0.1",
    tcp_port: default_tcp_port,
    end:""

};
