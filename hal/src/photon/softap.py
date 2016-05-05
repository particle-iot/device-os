# Change these to your local environment

# the device ID (to find this, run the test and it will print the device id for you as part of the failure message, which you can paste here.)
import json

expected_device_id = "320020001847333531323135"
expected_public_key = "30819F300D06092A864886F70D010101050003818D0030818902818100AD0C6EEFB66AE1C5C4C9F5AE00BD686CBC212CC9D620B8292F668697B4BA34C8650D1B3F14F0CEFA095986F2026765FCDC10303BC3051157B29D8255CC40AE7664A2155814373CF4B895A25190F7A8496C386950904AC1230B118C9A0B191AFB25ECD3B7CEADB175C85C7B0F727CCA901E857738ACF537F6A17C4E8A789CC21F0203010001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000".upper()

claim_code = "MDMAMDMAMDMAMDMAMDMAMDMAMDMAMDMAMDMAMDMAMDMAMDMAMDMAMDMAMDMAMDM"
claimed_result = "0"
expected_version = "2"


class Object(object):
    pass

# the AP to connect to. This should match the details returned in a the scan_ap results
configure_ap = {'idx':0,
                'ssid':"guest",
                'pwd':"guest",
                'sec':4194310, # ScanResult.Security.SECURITY_WPA2_MIXED_PSK. See the .proto file.
                'ch':7} # This doesn't seem to matter if it's correct or not


import os
import re
import subprocess
import time
import socket
import httplib
import unittest
import rsa
import StringIO
import binascii
from hamcrest import assert_that, empty, is_, is_not

def hex2bin(hex):
    binary_string = binascii.unhexlify(hex)
    return binary_string


def encrypt_pwd(pwd, key_hex):
    """
    pwd: the password to encrypt
    key_hex: the public key in hex-encoded DER format as received from the device.
    """
    # offset 22 is the start of the sequence containing the public key exponent and n
    key_bin = hex2bin(key_hex[44:])

    pk = rsa.PublicKey.load_pkcs1(key_bin, 'DER')
    # http://stuvel.eu/files/python-rsa-doc/reference.html#rsa.PublicKey
    #
    # save cert as memory file w StringIO - https://docs.python.org/2/library/stringio.html
    # load_pkcs1()
    #
    result = rsa.encrypt(pwd, pk)
    return result


def scan():
    refresh()
    p = subprocess.Popen('netsh wlan show networks', shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    lines = p.stdout.readlines()
    ssid_re = re.compile("SSID \d+ : (.+)")
    type_re = re.compile("\s*Authentication\s+: (.+)")
    crypt_re = re.compile("\s*Encryption\s+: (.+)")

    result = []
    current = None

    for l in lines:
        #line = l.decode('utf-8').strip()
        line = l.strip()
        m = ssid_re.match(line)
        if m:
            current = {'ssid':m.group(1), 'type':None, 'encryption':None}
            result.append(current)
        elif type_re.match(line):
            m = type_re.match(line)
            current['type'] = m.group(1)
        elif crypt_re.match(line):
            m = crypt_re.match(line)
            current['encryption'] = m.group(1)

    return result

def connect(ssid):
    subprocess.call("netsh wlan connect name=%s" % ssid)
    time.sleep(3)

def refresh():
    pass

class BaseClient:
    def version(self):
        result = self._fetch("version", None, Object())
        return result["v"]

    def fetch_device_id(self):
        obj = self._fetch("device-id", None, Object())
        return obj["id"]

    def fetch_claimed(self):
        obj = self._fetch("device-id", None, Object())
        return obj["c"]

    def scan_ap(self):
        return self._fetch("scan-ap", None, Object())

    def configure_ap(self):
        config = configure_ap.copy()
        encrypted = encrypt_pwd(configure_ap['pwd'], expected_public_key)
        config['pwd'] = binascii.hexlify(encrypted)
        print "pwd encrypted is " + config['pwd']
        return self._fetch("configure-ap", config, Object())["r"]

    def connect_ap(self):
        connect = {'idx':0}
        return self._fetch("connect-ap", connect, Object())["r"]

    def provision_keys(self):
        provision = Object()
        provision.salt = 123
        return self._fetch("provision-keys", provision, object())["r"]

    def public_key(self):
        return self._fetch("public-key", None, Object())["b"]

    def claim_code(self):
        json = {"k":"cc","v":claim_code}
        return self._fetch("set", json, object())["r"]


    def _fetch(self, name, req, resp):
        pass


# should really use delegation here, but taking a short cut and using inheritance

class HTTPSoftAPClient(BaseClient):
    def send_request(self, name, request_obj=None, response_obj=None):
        client = httplib.HTTPConnection('192.168.0.1', timeout=10)
        body = None if not request_obj else json.dumps(request_obj)
        if body is None:
            client.request("GET", name)
        else:
            client.request("POST", name, body)

        response = client.getresponse()
        assert_that(response.status, is_(200))
        response_body = response.read()
        response_obj = self._parseResponse(response_body)
        client.close()
        return response_obj

    def _fetch(self, name, req, resp):
        return self.send_request("/"+name, req, resp)

    def _parseResponse(self, msg):
        obj = json.loads(msg) if msg else Object()
        return obj


class TCPSoftAPClient(BaseClient):
    def __init__(self):
        self.s = None

    def _connect(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('192.168.0.1', 5609))
        s.settimeout(10)
        self.s = s

    def send(self, cmd, data=None):
        msg = cmd+"\n"
        if data is not None:
            ser = json.dumps(data)
            msg += str(len(ser)) + "\n\n"
            msg += ser
        else:
            msg += "0\n\n"
        self.s.send(msg)

    def recv(self, obj):
        msg = ""
        data = self.s.recv(4096)
        while data:
            msg += data
            data = self.s.recv(4096)

        return self._parseResponse(msg)

    def close(self):
        self.s.close()
        self.s = None

    def _fetch(self, name, req, resp):
        self._connect()
        self.send(name, req)
        if resp is not None:
            resp = self.recv(resp)
        self.close()
        return resp

    def _parseResponse(self, msg):
        lines = msg.split('\n')
        i = 0
        for x in lines:
            i = i+1
            if len(x)==0:
                break

        msg = "\n".join(lines[0:])
        obj = json.loads(msg) if i else Object()
        return obj


#client = TCPSoftAPClient()
client = HTTPSoftAPClient()


# class EncryptionTestCase(unittest.TestCase):
#     def test_encryption_decryption(self):
#         encrypted = encrypt_pwd("hello", expected_public_key)
#         assert_that(len(encrypted),is_(256))
#         print encrypted

class SoftAPTestCase(unittest.TestCase):
    def test_soft_ap(self):
        if False and os.name=='nt':
            ssid = self.find_photon_ap()
            print("Connecting to network ssid=%s " % ssid)
            connect(ssid)

        if True:
            version = client.version()
            assert_that(version, is_(expected_version))
            print("Protocol Version is %d" % version)

        device_id = client.fetch_device_id()
        print("Device id is '%s'" % device_id)
        assert_that(device_id, is_(expected_device_id))

        claimed = client.fetch_claimed()
        # assert_that(claimed, is_(claimed_result), "claim state was different")

        result = client.claim_code()
        assert_that(result, is_(0), "claim code failed")

        public_key_hex = client.public_key()
        print("public key is '%s'" % public_key_hex)
        assert_that(public_key_hex, is_(expected_public_key))

        scanResults = client.scan_ap()
        assert_that(scanResults, is_not(empty()), "expected some APs")
        for ap in scanResults["scans"]:
            print(ap)

        #assert_that(client.provision_keys(), is_(0), "Error provisioning keys")
        assert_that(client.configure_ap(), is_(0), "Error configuring ap")
        print("connecting device to AP '%s'" % configure_ap['ssid'])
        assert_that(client.connect_ap(), is_(0))

    def find_photon_ap(self):
        start = time.time()
        photons = []
        photon_re = re.compile("photon-\d+")
        while (not photons and (time.time()-start)<30):
            networks = scan()
            for ap in networks:
                if photon_re.match(ap['ssid']):
                    photons.append(ap)

        assert_that(photons, is_not(empty()), "expected at least one photon soft AP network")

        ssid = photons[0]['ssid']
        return ssid


if __name__ == '__main__':
    unittest.main()
