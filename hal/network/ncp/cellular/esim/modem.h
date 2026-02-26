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

	virtual int openApduChannel(std::unique_ptr<ApduChannel>& channel) = 0;

	// Operation-specific hooks

	virtual int beforeProvision();
	virtual int afterProvision(int result);

	virtual int beforeEnableProfile();
	virtual int afterEnableProfile(int result);

	virtual int beforeDisableProfile();
	virtual int afterDisableProfile(int result);

	virtual int beforeDeleteProfile();
	virtual int afterDeleteProfile(int result);

	virtual int beforeGetProfilesInfo();
	virtual int afterGetProfilesInfo(int result);
};

} // namespace particle::esim
