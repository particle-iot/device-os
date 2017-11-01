# NOTE: you probably want to
# specify a version like this: - sudo apt-get install -qq "gcc-arm-none-eabi=4.8.2-14ubuntu1+6"

sudo add-apt-repository -y ppa:terry.guo/gcc-arm-embedded &&
sudo apt-get -qq update &&
sudo apt-get -qq install  "gcc-arm-none-eabi-4_9-2015q3" &&
arm-none-eabi-gcc --version ;
