Flash client application to the device (add USE_THREADING=y option to enable threading):
$ cd ~/firmware/modules
$ make -s all program-dfu PLATFORM=photon TEST=wiring/tcp_client

Install server dependencies:
$ cd ~/firmware/user/tests/wiring/tcp_client/server
$ npm install

Start server:
$ ./server
