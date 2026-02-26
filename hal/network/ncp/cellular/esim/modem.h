#pragma once

#include <memory>

namespace particle::esim {

class ApduChannel;

/**
 * Modem interface for eSIM management.
 */
class Modem {
public:
	virtual ~Modem() = default;

	virtual int openApduChannel() = 0;
	virtual int closeApduChannel() = 0;
	virtual int sendApduCommand(const ApduCommand& cmd, ApduResponse& resp) = 0;

	// Operation-specific hooks

	virtual int beforeProvisioning();
	virtual int afterProvisioning(int result);

	virtual int beforeEnableProfile();
	virtual int afterEnableProfile(int result);

	virtual int beforeDisableProfile();
	virtual int afterDisableProfile(int result);

	virtual int beforeDeleteProfile();
	virtual int afterDeleteProfile(int result);

	virtual int beforeGetProfilesInfo();
	virtual int afterGetProfilesInfo(int result);

	// ...
};

} // namespace particle::esim
