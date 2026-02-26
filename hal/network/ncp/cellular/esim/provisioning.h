#pragma once

namespace particle {

class InputStream;

namespace esim {

class Provisioning {
public:
	// Initiates an RSP session and returns a random challenge attached to this session
	int getEuiccChallenge(char* resp, size_t respSize);

 	// Gets an EUICCInfo1 for forwarding to the SM-DP+
	int getEuiccInfo1(char* resp, size_t respSize);

	// Authenticates the SM-DP+
	int authenticateServer(const char* req, size_t reqSize, char* resp, size_t respSize);

	// Initiates a profile download
	int prepareDownload(const char* req, size_t reqSize, char* resp, size_t respSize);

	// Transfers the profile data to the eUICC, e.g. from a file
	int loadBoundProfilePackage(InputStream* stream);

	// Cancels the provisioning
	void cancel();
};

} // namespace esim

} // namespace particle
