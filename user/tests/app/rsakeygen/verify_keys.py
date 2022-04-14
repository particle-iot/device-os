#!/usr/bin/env python
import sys
import os
import subprocess

def usage():
    print '%s [input_file or tty]' % (sys.argv[0])

if len(sys.argv) != 2:
    usage()
    sys.exit(1)

f = open(sys.argv[1], 'r')
i = -1

def hex2bin(hex):
    return hex.decode('hex')

def run(cmd, stdin):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    out, err = p.communicate(stdin)
    return (out.strip(), err.strip())

for l in f:
    l = l.strip()
    if l.startswith('done'):
        print 'Done'
        sys.exit(0)

    if l.startswith('keys:'):
        l = l[5:]
    else:
        continue
    priv, pub = l.split(":")
    i += 1
    privb = hex2bin(priv)
    pubb = hex2bin(pub)

    privcheck = run(['openssl', 'rsa', '-inform', 'DER', '-noout', '-check'], privb)
    privmod = run(['openssl', 'rsa', '-inform', 'DER', '-noout', '-modulus'], privb)
    pubmod = run(['openssl', 'rsa', '-pubin', '-inform', 'DER', '-noout', '-modulus'], pubb)
    
    if not privcheck[1] and (privmod[0] == pubmod[0]) and not privmod[1] and not pubmod[1]:
        print '%d:OK' % (i, )
    else:
        print '%d:FAIL' % (i, )
        sys.stderr.write(privcheck[0])
        sys.stderr.write(privcheck[1])
        sys.stderr.write(privmod[0])
        sys.stderr.write(privmod[1])
        sys.stderr.write(pubmod[0])
        sys.stderr.write(pubmod[1])

print 'Done'
sys.exit(0)
