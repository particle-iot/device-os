# MAKE ALL
# make_release.sh
#
# MAKE JUST ONE
# make_release.sh electron
# or
# make_release.sh 10

if [ -z $1 ]; then
	MAKE_RELEASE_PLATFORM="all"
else
	MAKE_RELEASE_PLATFORM="$1"
fi

echo attempting build for PLATFORM: $MAKE_RELEASE_PLATFORM

if [ $MAKE_RELEASE_PLATFORM == "photon" ] || [ $MAKE_RELEASE_PLATFORM == "6" ]; then
	./release.sh 6 photon
else if [ $MAKE_RELEASE_PLATFORM == "p1" ] || [ $MAKE_RELEASE_PLATFORM == "8" ]; then
	./release.sh 8 p1
else if [ $MAKE_RELEASE_PLATFORM == "electron" ] || [ $MAKE_RELEASE_PLATFORM == "10" ]; then
	./release.sh 10 electron
else if [ $MAKE_RELEASE_PLATFORM == "core" ] || [ $MAKE_RELEASE_PLATFORM == "0" ]; then
	./release.sh 0 core
else if [ $MAKE_RELEASE_PLATFORM == "all" ]; then
	./release.sh 6 photon
	./release.sh 8 p1
	./release.sh 10 electron
	./release.sh 0 core
else
	echo ERROR, $MAKE_RELEASE_PLATFORM not valid!!
fi
fi
fi
fi
fi
