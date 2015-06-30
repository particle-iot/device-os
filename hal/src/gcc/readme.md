- clone boost and build
```
git clone --recursive http://github.com/boostorg/boost.git boost
export BOOST_ROOT=/path/to/boost-dir/boost
cd boost
./bootstrap.sh
./b2


alternatively (for windows?):
```
sudo ./bjam --install --link=static --runtime-link=static --layout=tagged --with-system threading=single architecture=x86
```



```
git clone repo https://github.com/spark/firmware
git checkout develop
cd main
make PRODUCT_ID=3 v=1
```

