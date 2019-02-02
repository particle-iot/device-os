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
	./release.sh --platform photon
elif [ $MAKE_RELEASE_PLATFORM == "p1" ] || [ $MAKE_RELEASE_PLATFORM == "8" ]; then
	./release.sh --platform p1
elif [ $MAKE_RELEASE_PLATFORM == "electron" ] || [ $MAKE_RELEASE_PLATFORM == "10" ]; then
	./release.sh --platform electron
elif [ $MAKE_RELEASE_PLATFORM == "core" ] || [ $MAKE_RELEASE_PLATFORM == "0" ]; then
	./release.sh --platform core
elif [ $MAKE_RELEASE_PLATFORM == "all" ]; then
	./release.sh --platform photon
	./release.sh --platform p1
	./release.sh --platform electron
	./release.sh --platform core
else
	echo ERROR, $MAKE_RELEASE_PLATFORM not valid!!
fi
