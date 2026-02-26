#pragma once

namespace particle::esim {

/**
 * Command APDU (C-APDU).
 */
class ApduCommand {
public:
	ApduCommand& cla(int cla);
	ApduCommand& ins(int ins);
	ApduCommand& p1(int p1);
	ApduCommand& p2(int p2);
	ApduCommand& le(int le);

	ApduCommand& data(const char* data, size_t size); // Sets `Lc` to `size`
};

/**
 * Response APDU (R-APDU).
 */
class ApduResponse {
public:
	int sw1() const;
	int sw2() const;
	int sw() const;

	const char* data() const;
	size_t size() const;
};

/**
 * Base class for an APDU transport channel.
 */
class ApduChannel {
public:
	virtual ~ApduChannel() = default;

	virtual int command(const ApduCommand& cmd, ApduResponse& resp) = 0;
	virtual void close() = 0;
};

} // namespace particle::esim
