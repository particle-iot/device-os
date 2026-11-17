## wiring/wifi

Runs a real Wi-Fi AP for the device (hostapd, dnsmasq for DHCP/DNS and NAT out of the default route,
so the device stays connected to the cloud through it) and tests against it:

- 802.11k/v: BTM request with BSS termination, BTM request with a bad URL length, beacon request,
  beacon request with an oversized SSID. The device has to stay connected after each one.
- Connection backoff: AP off -> 4-8 attempts in 3 min, no radio reset, BLE keeps advertising. New
  credentials reset the backoff. AP back on -> reconnects.
- RX stress, Gen 4 only, for the Gen 4 Wi-Fi RX stall (the device stopped receiving until reset,
  scans found no APs). Every external flash read reprograms the MPU, and the ROM code turns the whole
  MPU off while it does that. An interrupt in that window let the Wi-Fi threads access the RX
  descriptor ring through the cache, and the driver ended up with stale descriptor tags. The device
  reads flash nonstop from a low priority thread, with BLE advertising and scanning and the SRAM heap
  taken so `lib_wlan` allocates in PSRAM. The runner sends it 500 pps unicast and 300 pps broadcast
  UDP through the AP for 3 min, broadcast at 1 Mbps basic rates. The device has to keep receiving, no
  gaps over 15 s, and has to see both flash reads and broadcasts. A failure reports how far into the
  run reception stopped, whether it came back and how many APs a scan finds.
- The original credentials are restored at the end, the device has to get back to the cloud with
  them.

The AP side lives in `user/tests/integration/test/wifi_ap.js`. The SSID and passphrase are random per
run and get to the device over USB. The device's own Wi-Fi credentials are backed up before the suite
and restored after, also when it fails or gets interrupted.

The suite is tagged `wifi_ap`. Wi-Fi platforms only (`platform('wifi')`), trackers are scan-only and
not included.

```
device-os-test run -v p2 wiring/wifi
```

## Where the AP runs

All of them need a free radio with AP mode (not connected, not carrying the default route) and an
uplink through another interface. The suite picks the radio on its own.

### Local machine

Needs hostapd, dnsmasq, iptables and root or [passwordless sudo](#passwordless-sudo). If any of that
is missing, the suite is skipped.

```
device-os-test run -v p2 wiring/wifi
```

If NetworkManager manages the radio, the suite takes it away for the run and hands it back after.
If Wi-Fi is turned off in NetworkManager, it is turned on for the run and off again after. A radio
connected to a network is never picked, disconnect it first: `nmcli device disconnect wlan0`.

### Another machine over ssh

E.g. a box with ethernet and an unused Wi-Fi radio within range of the device. Same requirements on
that machine, plus ssh key auth without a prompt. A missing tool or radio fails the suite instead of
skipping it. For the RX stress test the runner sends the flood to that machine (the address ssh
connects to, IPv4 only) on UDP ports 50100 and 50101, and it forwards them to the device, unicast and
broadcast, so those ports have to be reachable from the runner, firewall on that machine included.

```
DEVICE_OS_TEST_RUNNER_WIFI_AP_HOST=user@host device-os-test run -v p2 wiring/wifi
```

`user@host` can also be a `~/.ssh/config` alias.

### Local container

The HIL image (`particle/rpi-device-os-test:x86-node-22`) has everything installed and runs as root, no
sudo setup needed. Build the test app on the host first (the image has no ARM toolchain), move the
radio into the container and run the suite there:

```sh
cd ~/dev/device-os/user/tests/integration
device-os-test build p2 wiring/wifi
nmcli device disconnect wlan0 && nmcli device set wlan0 managed no

docker run -d --name device-os-wifi-test --privileged \
  -v /dev/bus/usb:/dev/bus/usb \
  -v ~/dev/device-os:$HOME/dev/device-os \
  -v ~/.particle:/root/.particle:ro \
  -w $HOME/dev/device-os/user/tests/integration \
  particle/rpi-device-os-test:x86-node-22 sleep infinity
sudo iw phy phy0 set netns $(docker inspect -f '{{.State.Pid}}' device-os-wifi-test)
docker exec -it device-os-wifi-test device-os-test run -v p2 wiring/wifi

docker rm -f device-os-wifi-test
nmcli device set wlan0 managed yes
```

The radio goes back to the host when the container goes away. `/dev/bus/usb` is mounted separately
because the device re-enumerates on every reset, the `/dev` copy `--privileged` makes at start
doesn't see that.

### HIL

The AP runs in the test container, a service on the host moves the radio into it on request.
`DEVICE_OS_TEST_RUNNER_WIFI_AP_CLAIM_DIR` enables that, HIL jobs without it exclude `wifi_ap`.

## Options

- `DEVICE_OS_TEST_RUNNER_WIFI_AP_INTERFACE` picks the radio explicitly.
- The AP leaves the TX power to the driver. The RX stress test sets a fixed 5 dBm for its run with
  `iw dev <radio> set txpower fixed` and puts it back to `auto` after, also when the suite fails.
  Some drivers don't report it back (mt7921e always shows 3.00 dBm).
- `DEVICE_OS_TEST_RUNNER_WIFI_AP_COUNTRY` sets the country code. Without it the AP runs in whatever
  regulatory domain the machine is in (channel 6 is allowed everywhere). With it, hostapd switches
  the machine's regulatory domain and leaves it there.

## hostapd for the frame tests

The 802.11k/v frame tests send raw frames with `hostapd_cli raw MGMT_TX`, which needs hostapd built
with `CONFIG_TESTING_OPTIONS=y`. Debian's package (and the HIL image) has it, Fedora's doesn't.
Locally the frame tests are skipped without it, with `DEVICE_OS_TEST_RUNNER_WIFI_AP_HOST` they fail.

On Fedora, rebuild the package with it:

```sh
sudo dnf install -y rpm-build libnl3-devel openssl-devel gcc make
mkdir -p ~/dev/hostapd && cd ~/dev/hostapd
dnf download --source hostapd && rpm2cpio hostapd-*.src.rpm | cpio -idm
sed -i -e 's/^\(Release: *[0-9]*\)/\1.1.testing/' \
    -e "/CONFIG_P2P_MANAGER=y/a\    -e '\$ a CONFIG_TESTING_OPTIONS=y' \\\\\n    -e '\$ a CONFIG_WNM=y' \\\\" hostapd.spec
rpmbuild -bb --nodeps --define "_sourcedir $PWD" --define "_builddir $PWD/build" --define "_rpmdir $PWD/rpms" hostapd.spec
sudo dnf install ./rpms/x86_64/hostapd-2.*.testing.*.rpm
```

Add `excludepkgs=hostapd` to `[main]` in `/etc/dnf/dnf.conf` so an update doesn't bring the stock
package back.

## Passwordless sudo

The suite calls `sudo -n` for `hostapd`, `hostapd_cli`, `dnsmasq`, `ip`, `iptables`, `iw`,
`sysctl`, `kill`, `rm`, plus `nmcli` and `firewall-cmd` when they are installed. On the machine that runs the
AP (local or remote), after installing the packages, run as the user the suite uses:

```sh
tools="true hostapd hostapd_cli dnsmasq ip iptables iw sysctl kill rm nmcli firewall-cmd"
cmds=$(for t in $tools; do
  for d in $(echo "$PATH:/usr/local/sbin:/usr/sbin:/sbin" | tr : ' '); do
    [ -x "$d/$t" ] && { echo "$d/$t"; break; }
  done
done | paste -sd, -)
echo "$(id -un) ALL=(root) NOPASSWD: $cmds" > /tmp/device-os-wifi-ap
sudo visudo -cf /tmp/device-os-wifi-ap && sudo install -m 440 /tmp/device-os-wifi-ap /etc/sudoers.d/device-os-wifi-ap
```

Which gives something like:

```
tester ALL=(root) NOPASSWD: /usr/bin/true,/usr/sbin/hostapd,/usr/sbin/hostapd_cli,/usr/sbin/dnsmasq,/usr/bin/ip,/usr/sbin/iptables,/usr/bin/iw,/usr/sbin/sysctl,/usr/bin/kill,/usr/bin/rm
```

`rm`, `kill` and `ip` as root without a password is as good as root. Use a dedicated test machine
or remove the file when done: `sudo rm /etc/sudoers.d/device-os-wifi-ap`.
