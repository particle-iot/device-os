Flash checkpoint application to the device:
$ cd ~/firmware/modules
$ make -s all program-dfu PLATFORM=photon TEST=app/checkpoint

Install cli dependencies:
$ cd ~/firmware/user/tests/app/checkpoint/cli
$ npm install

Run client:
$ ./cli

Publish some events that trigger specific scenarios on device:
$ particle publish hardfault --private
$ particle publish panic --private
$ particle publish deadlock --private

$ Run some commands to get last or current diagnostic info or force a full stacktrace dump of currently running threads:
$ ./cli get
$ ./cli getlast
$ ./cli update
