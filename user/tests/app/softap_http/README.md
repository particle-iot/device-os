Flash softap application to the device:
$ cd ~/firmware/modules
$ make -s all program-dfu PLATFORM=photon TEST=app/softap_http

Install client dependencies:
$ cd ~/firmware/user/tests/app/softap_http/client
$ npm install

Run client:
$ ./client
