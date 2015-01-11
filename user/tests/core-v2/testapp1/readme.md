## Flashing the core-v2/testapp1 files to the BM-14

```
cd firmware-private/main
make v=1 SPARK_PRODUCT_ID=5 TEST=core-v2/testapp1
cd firmware-private/build/target/main
dfu-util -d 1d50:607f -a 0 -s 0x08020000:leave -D prod-5/tests/core-v2/testapp1/testapp1.bin
```


