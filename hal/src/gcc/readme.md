- clone boost and build
```
git clone --recursive http://github.com/boostorg/boost.git boost
export BOOST_ROOT=/path/to/boost-dir/boost
cd boost
./bootstrap.sh
./b2 (--prefix=/path/to/usr-bin-dir)


alternatively, for windows: (Nothing here right now. Recommendations welcome) 
```

- install boost
```
sudo ./bjam --install --link=static --runtime-link=static --layout=tagged --with-system-threading=single architecture=x86
```


- clone photon firmware and build (and flash it if you have dfu-util installed)
```
git clone repo https://github.com/spark/firmware
git checkout develop
cd main
make PRODUCT_ID=6 v=1 (clean flash-dfu)
```

