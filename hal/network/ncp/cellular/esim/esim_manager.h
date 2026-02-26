#pragma once

namespace particle::esim {

class Modem;

class EsimProvisioning {
public:
	int getEuiccChallenge(/* ... */);
	int getEuiccInfo1(/* ... */);
	int authenticateServer(/* ... */);
	int prepareDownload(/* ... */);
	int loadBoundProfilePackage(/* ... */); // Chunked transfer from a file?

	int finish();
	void cancel();
};

class EsimManager {
public:
	EsimManager();
	~EsimManager();

	int init(Modem* modem);
	void destroy();

	int startProvisioning(EsimProvisioning& ctx);

	int enableProfile(/* ... */);
	int disableProfile(/* ... */);
	int deleteProfile(/* ... */);
	int getProfilesInfo(/* ... */);
};

} // namespace particle::esim
