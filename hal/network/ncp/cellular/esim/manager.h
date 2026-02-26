#pragma once

namespace particle::esim {

class Provisioning;
class Modem;

class Manager {
public:
	Manager();
	~Manager();

	int init(Modem* modem);
	void destroy();

	int startProvisioning(Provisioning& prov);

	int enableProfile(/* ... */);
	int disableProfile(/* ... */);
	int deleteProfile(/* ... */);
	int getProfilesInfo(/* ... */);
};

} // namespace particle::esim
