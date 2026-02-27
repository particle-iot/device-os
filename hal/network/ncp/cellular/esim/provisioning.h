#pragma once

namespace particle {

class InputStream;
class OutputStream;

namespace esim {

/**
 * Profile provisioning context.
 */
class Provisioning {
public:
	/**
	 * Initiate a provisioning session.
	 *
	 * @param out Stream for encoding `GetEuiccChallengeResponse` (SGP 22, 5.7.7).
	 * @return 0 on success, otherwise an error code defined by `system_error_t`.
	 */
	int getEuiccChallenge(OutputStream* out);

 	/**
 	 * Get the eUUIC info for forwarding to the SM-DP+.
 	 *
	 * @param out Stream for encoding `EUICCInfo1` (SGP 22, 5.7.8).
	 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 	 */
	int getEuiccInfo1(OutputStream* out);

	/**
	 * Authenticate the SM-DP+.
	 *
	 * @param in Stream for decoding `AuthenticateServerRequest` (SGP 22, 5.7.13).
	 * @param out Stream for encoding `AuthenticateServerResponse`.
	 * @return 0 on success, otherwise an error code defined by `system_error_t`.
	 */
	int authenticateServer(InputStream* in, OutputStream* out);

	/**
	 * Initiate a profile download.
	 *
	 * @param in Stream for decoding `PrepareDownloadRequest` (SGP 22, 5.7.5).
	 * @param out Stream for encoding `PrepareDownloadResponse`.
	 * @return 0 on success, otherwise an error code defined by `system_error_t`.
	 */
	int prepareDownload(InputStream* in, OutputStream* out);

	/**
	 * Transfer the profile data to the eUICC.
	 *
	 * Ends the provisioning session.
	 *
	 * @param in Stream for reading the contents of a profile package.
	 * @return 0 on success, otherwise an error code defined by `system_error_t`.
	 */
	int loadBoundProfilePackage(InputStream* in);

	/**
	 * Cancel the provisioning session.
	 *
	 * @param error Error code defined by `system_error_t`.
	 */
	void cancel(int error);
};

} // namespace esim

} // namespace particle
