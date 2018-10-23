
#include <stdint.h>
#include "core_hal.h"
#include "flash_mal.h"
#include "bootloader.h"
#include "module_info.h"
#include "bootloader_hal.h"
#include "ota_flash_hal_impl.h"
#include "miniz.h"
#include "stream.h"
#include "check.h"
#include "ota_flash_hal_impl.h"
#include <memory>


#define BOOTLOADER_ADDR (module_bootloader.start_address)

namespace particle {

class OTAUpdateStream : public OutputStream {
	size_t destAddress_;
	size_t endAddress_;
public:
	int begin(int size) {
		destAddress_ = HAL_OTA_FlashAddress();
		endAddress_ = destAddress_ + size;
		return HAL_FLASH_Begin(destAddress_, size, nullptr);
	}

	int write(const char* data, size_t length) override {
		if (length > endAddress_-destAddress_) {
			length = endAddress_-destAddress_;
		}
		int error = HAL_FLASH_Update((const uint8_t*)data, destAddress_, length, nullptr);
		if (!error) {
			destAddress_ += length;
			return length;
		}
		else {
			return error;
		}
	}

	int waitEvent(unsigned flags, unsigned timeout = 0) override {
		return 0;
	}

	int availForWrite() override {
		return endAddress_-destAddress_;
	}

	int flush() override {
		return HAL_FLASH_End(nullptr)!=HAL_UPDATE_ERROR;
	}
};

class Deflator {

	tinfl_decompressor inflator;

public:
	Deflator() {
		tinfl_init(&inflator);
	}

	/**
	 * @param in_buffer [in] The input (compressed) data
	 * @param in_length [in,out] The number of bytes available in the input stream. When 0, indicates the end of stream. On return, is the number of bytes consumed from the input buffer.
	 * @param out_buffer [in] The output (uncomprssed) data
	 * @param out_length [in,out] The number of bytes available in the output buffer.  On return, it is the number of bytes written to the output buffer.
	 * Returns >0 if more data is needed on input or more data available for output.
	 * Returns 0 when the compression is done
	 * Returns <0 on error
	 */
	tinfl_status write(const char* in_buffer, size_t& in_length, char* out_buffer, char* next_buffer, size_t& out_length, bool hasMore);

};

tinfl_status Deflator::write(const char* in_buffer, size_t& in_length, char* out_buffer, char* next_buffer, size_t& out_length, bool hasMore) {
    const int flags = (hasMore ? TINFL_FLAG_HAS_MORE_INPUT : 0) | 0;
	tinfl_status status = tinfl_decompress(&inflator, (const mz_uint8*)in_buffer, &in_length, (mz_uint8*)out_buffer, (mz_uint8*)next_buffer, &out_length, flags);
	return status;
}


class DecompressStream : public OutputStream {
	Deflator deflator_;
	char* buffer_;
	size_t length_;
	OutputStream& output_;
	size_t out_offset_;

public:

	DecompressStream(char* buffer, size_t length, OutputStream& output)
: buffer_(buffer), length_(length), output_(output), out_offset_(0) {}

	/**
	 * Write to the stream.
	 * @param buffer The compressed data to decompress
	 * @param length	The amount of data to write
	 * @returns The number of bytes written from the buffer.
	 */
	int write(const char* buffer, size_t length) override;

	int flush() override {
		int result = write(buffer_, 0);
		output_.flush();
		return result;
	}

	int availForWrite() override {
		return length_-out_offset_;
	}

	int waitEvent(unsigned flags, unsigned timeout = 0) override {
		return 0;
	}

};

/**
 * Write to the stream.
 * @param buffer The compressed data to decompress
 * @param length	The amount of data to write
 * @returns The number of bytes written from the buffer.
 */
int DecompressStream::write(const char* buffer, const size_t length)  {
	bool hasMore = length;
	size_t consumed = 0;
	tinfl_status status;
	do {
		size_t out_length = length_-out_offset_;
		size_t in_length = length-consumed;
		status = deflator_.write(buffer+consumed, in_length, this->buffer_, this->buffer_+out_offset_, out_length, hasMore);
		if (status<0) {
			return SYSTEM_ERROR_BAD_DATA;
		}
		consumed += in_length;
		if (out_length) {
			output_.writeAll(this->buffer_+out_offset_, out_length);
			out_offset_ = (out_offset_ + out_length) % length_;
		}
	}
	while (status == TINFL_STATUS_HAS_MORE_OUTPUT);

	// exit when Done, or there is an error
	return (status<TINFL_STATUS_DONE) ? status : consumed;
}

} // namespace particle

#ifdef HAL_REPLACE_BOOTLOADER_OTA
int bootloader_update(const void* bootloader_image, unsigned length)
{
    HAL_Bootloader_Lock(false);
    int result = (FLASH_CopyMemory(FLASH_INTERNAL, (uint32_t)bootloader_image,
        FLASH_INTERNAL, BOOTLOADER_ADDR, length, MODULE_FUNCTION_BOOTLOADER,
        MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION));
    HAL_Bootloader_Lock(true);
    return result;
}
#else
int bootloader_update(const void*, unsigned)
{
    return FLASH_ACCESS_RESULT_ERROR;
}
#endif // HAL_REPLACE_BOOTLOADER_OTA

#ifdef HAL_REPLACE_BOOTLOADER

/**
 * Manages upgrading the bootloader.
 */

bool bootloader_requires_update(const uint8_t* bootloader_image, uint32_t length)
{
    if ((bootloader_image == nullptr) || length == 0)
        return false;

    const uint32_t VERSION_OFFSET = 0x200+10;

    uint16_t current_version = *(uint16_t*)(BOOTLOADER_ADDR+VERSION_OFFSET);
    uint16_t available_version = *(uint16_t*)(bootloader_image+VERSION_OFFSET);

    bool requires_update = current_version<available_version;
    return requires_update;
}

using namespace particle;

/**
 * Decompresses the first 1K of the compressed bootloader image.
 * This contains the module info, which conains the bootloader module version.
 * This is compared against the current bootloader, and if newer,
 */
bool bootloader_update_if_needed()
{
    bool updated = false;

    bool requires_update = false;

    uint32_t bootloader_image_size = 0;
    const char* bootloader_image = (const char*)HAL_Bootloader_Image(&bootloader_image_size, nullptr);
	const size_t buffer_size = TINFL_LZ_DICT_SIZE;
	auto output_size = buffer_size;
	size_t input_size = bootloader_image_size;
	auto buffer = std::make_unique<char[]>(buffer_size);

	{
		auto deflator = std::make_unique<Deflator>();
		deflator->write(bootloader_image, input_size, buffer.get(), buffer.get(), output_size, false);
		requires_update = bootloader_requires_update((const uint8_t*)buffer.get(), buffer_size);
	}

    if (requires_update) {
    	// todo - take from the bootloader module bounds
        OTAUpdateStream otaUpdateStream;
        otaUpdateStream.begin(module_bootloader.maximum_size);
        auto decompress = std::make_unique<DecompressStream>(buffer.get(), buffer_size, otaUpdateStream);
        decompress->write((const char*)bootloader_image, bootloader_image_size);
        updated = !decompress->flush();
    }
    return updated;
}

#else

bool bootloader_requires_update()
{
    return false;
}

bool bootloader_update_if_needed()
{
    return false;
}

#endif
