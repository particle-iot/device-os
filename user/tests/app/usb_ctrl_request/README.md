Build the test application and flash it to the device:
```
$ cd ~/firmware/modules
$ make -s all program-dfu PLATFORM=photon TEST=app/usb_ctrl_request/app
```

Install dependencies and run the test suite:
```
$ cd ~/firmware/user/tests/app/usb_ctrl_request/test
$ npm install
$ npm test
```
