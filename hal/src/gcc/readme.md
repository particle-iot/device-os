- clone boost and build
```
git clone --recursive http://github.com/boostorg/boost.git boost
cd boost
./bootstrap.sh
sudo ./bjam --install --link=static --runtime-link=static --layout=tagged --with-system threading=single architecture=x86
```

- clone firmware repo https://github.com/spark/firmware
- build
```
make PRODUCT_ID=3 v=1 
```

