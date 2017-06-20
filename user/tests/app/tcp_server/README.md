Flash server application to the device (add USE_THREADING=y option to enable threading):
$ cd ~/firmware/modules
$ make -s all program-dfu PLATFORM=photon TEST=app/tcp_server

Install client dependencies:
$ cd ~/firmware/user/tests/app/tcp_server/client
$ npm install

Run client:
$ ./client
