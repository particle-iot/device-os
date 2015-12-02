
## Provisioning Keys

### Upload the Server public Key to the Device

Download the server public key from https://s3.amazonaws.com/spark-website/cloud_public.der

and upload this to the device.

First, place the device in [DFU mode](http://docs.particle.io/photon/modes/#photon-modes-dfu-mode-device-firmware-upgrade),
then run this command on the command line in the same directory where the certificate was downloaded to.

```
dfu-util -d 2b04:d006 -a 1 -s 2082 -D cloud_public.der
```
