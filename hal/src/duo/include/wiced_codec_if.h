/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "wiced_result.h"
/**
 * @file wiced_codec_if.h
 *
 * The following code describes a generic codec interface so that any codec can be
 * "plugged into" the system.
 * This would cover both the encoders and decoders.
 *
 */
/******************************************************
 *                      Forward Declarations
 ******************************************************/
struct wiced_codec_data_transfer_cb;
typedef struct wiced_codec_data_transfer_cb wiced_codec_data_transfer_api_t;

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/
/**
 * Codec Ids
 * NOTE: Add new codec definitions to this list.
 */
typedef enum  {
     WICED_CODEC_SBC = 0, //!< SBC
     WICED_CODEC_MAX//!< CODEC_MAX
}  wiced_codec_type_t;

/**
 * Number of channels. If more, then add to this list,preferably after DUAL_CHANNEL.
 */
typedef enum {
    WICED_CODEC_CHANNEL_MONO = 0,    //!< MONO
    WICED_CODEC_CHANNEL_STEREO,      //!< STEREO
    WICED_CODEC_CHANNEL_JOINT_STEREO,//!< JOINT_STEREO
    WICED_CODEC_CHANNEL_DUAL_CHANNEL,//!< DUAL_CHANNEL
    WICED_CODEC_CHANNEL_MAX_CHANNELS //!< MAX_CHANNELS
} wiced_codec_channels_t;

typedef enum {
    WICED_CODEC_SAMPLING_FREQUENCY_32K = 0,
    WICED_CODEC_SAMPLING_FREQUENCY_44K,
    WICED_CODEC_SAMPLING_FREQUENCY_48K,
    WICED_CODEC_SAMPLING_FREQUENCY_92K,
    WICED_MAX_SAMPLING_FREQ
} wiced_codec_sampling_frequency_t;


/******************************************************
 *                 Type Definitions
 ******************************************************/
// TODO: remove codec_status, replace by result_t
typedef wiced_result_t wiced_codec_status_t ;

/**
 * NOTE
 *
 * The decoder/encoder in the following api list take no arguments.
 * The flow is as follows.
 *
 * 1. Initialize using the init function, pass in a struct with pointers to functions that can allocate
 * memory for output data.
 * 2. Get the capabilities of the codec using the get capabilities function implementation.
 * 3. Use the decode function for decoding data.
 *    3.1 Use the codec_alloc_output_buffer_cb to allocate data for the output. This depends on
 *    the user of the API, as per platform programming requirements.
 *    3.2 The decoder implementation must read in data (from for example, a queue) in the implementation
 *    of codec_read_encoded_data_cb by the api user. API user will not directly pass data to the decoder.
 *    3.3 The decoder implementation writes decoded data (for example, to a queue) in the implementation
 *    of codec_write_decoded_data_cb. There is no direct retrieval of pcm data from the interface.
 */



/**
 * @typedef codec_if_api_init
 * @brief Initialize the codec with mandatory memory management and data arguments. If the memory management
 * parameter is not provided then the codec implementation will not fit into the rest of the platform structure.
 * The memory management functions must fetch the raw/encoded data, allocate memory for the output as per platform
 * specifications and provide the data to a location using the mechanism provided by the memory management function.
 * This allows for maximum flexibility for the application developer.
 *
 * @param [in] data transfer methods that need to be passed to the codec for platform/app specific memory management.
 * @param [in] data Any data that a particular codec implementation might require. The interpretation of the data
 * must be provided by the codec parameter file (wiced_codec_xxx_params.h) and must be shared between the codec
 * implementation and the application.
 * @return wiced_codec_status_t 0 if success
 */
typedef wiced_codec_status_t (*codec_if_api_init)(wiced_codec_data_transfer_api_t* data_transfer_methods, void* data);


/**
 * @typedef codec_if_api_close
 * @brief De-initialize the code.
 *
 * @return wiced_codec_status_t 0 if success.
 */
typedef wiced_codec_status_t (*codec_if_api_close)(void);


/**
 * @typedef codec_if_api_get_capabilities
 * @brief Get the configurations supported by the implementation of the codec. The exact form of the
 * parameters must be provided in a global wiced_codec_xx_params.h file, and used in the application as well as
 * the codec implementation.For example, look at the default platform provided file wiced_codec_sbc_params.h.
 *
 * @param [out] codec_capabilities All the capabilities supported by the codec. This is codec-dependent; the
 * interpretation of this is as per the param file mentioned in the description above.
 * @param [out, optional] codec_preferred_params The codec parameters that indicate the preferred configuration of the codec. This
 * is still a subset of the codec capabilities. Note that the interpretation of this struct is as per comment for
 * codec_capabilities. This can be set to NULL if the user does not need the preferred subset of capabilities to be used.
 */
typedef void (*codec_if_api_get_capabilities)(void* codec_capabilities, void* codec_preferred_params);


/**
 * @typedef codec_if_api_setconfig
 * @brief Set the desired configuration of the codec.The exact form of the parameters must be provided
 * in a global wiced_codec_xx_params.h file, and used in the application as well as the codec implementation.
 * For example, look at the default platform provided file wiced_codec_sbc_params.h.
 * @param [in] config The desired codec configuration. This is codec-dependent; the
 * interpretation of this is as per the param file mentioned in the description above.
 * @return wiced_codec_status_t 0 if SUCCESS
 */
typedef wiced_codec_status_t (*codec_if_api_set_configuration)(void* config);


/**
 * @typedef codec_if_api_encode
 * @brief Encode the given data in pcm (length pcm_len). Make sure that codec_init with
 * the data transfer methods structure has been called before using this function. The encode function
 * must use the data transfer methods to read the raw data, allocate space for the output and transfer the
 * data to a system-specified location.
 * The encoder implementation shall use the callbacks codec_alloc_output_buffer_cb, codec_read_encoded_data_cb
 * and codec_write_decoded_data_cb respectively to allocate memory for the output buffer, reading pcm
 * data into the codec and writing the encoded data.
 * @return wiced_codec_status_t 0 if successful.
 */
typedef wiced_codec_status_t (*codec_if_api_encode)(void);


/**
 * @typedef codec_if_api_decode
 * @brief Decode the data in src. Make sure that codec_init with
 * the memory management structure has been called before using this function.
 * The decode function must use the data transfer methods to read the encoded data, allocate space for the
 * output and transfer the pcm data to a system-specified location.
 *
 * The decoder implementation shall use the callbacks codec_alloc_output_buffer_cb, codec_read_encoded_data_cb
 * and codec_write_decoded_data_cb respectively to allocate memory for the output buffer, reading encoded
 * data into the codec and writing the decoded data.
 * @return wiced_codec_status_t 0 if successful.
 */
typedef wiced_codec_status_t (*codec_if_api_decode)(void);


/**
 * @typedef codec_alloc_output_buffer
 * @brief Allocate the memory for the output buffer. This pointer points to a function provided by the system (not the codec).
 * The codec calls this function to allocate memory for the output.
 * The decoder needs this function callback, provided by the user of the decoder, to be able to
 * allocate memory for the output.
 *
 * @param [in] buffer The buffer for the output.
 * @param [in] length The buffer length requested by the decoder for output.
 * @return unsigned length of the space allocated.
 */
typedef unsigned short (*codec_alloc_output_buffer_cb)(int16_t** buffer, uint16_t length);


/**
 * @typedef codec_read_encoded_data
 * @brief Get the encoded data from the system memory location.Make sure that codec_init with
 * the memory management structure has been called before using this function.
 * Note that the memory management functions will also fetch the raw data for the decoder.
 * Either bytes_count or the available number of bytes less than bytes_count must be
 * copied into data.
 * This callback will be used by the decoder to read the encoded data using the mechanism
 * decided by the user of the decoder. The implementation of the callback must be provided by
 * the user of the codec interface.
 *
 * @param [in] data  The encoded data
 * @param [in] nBytes The number of bytes copies
 * @return unsigned short length of data read in bytes
 */
typedef unsigned short (*codec_read_encoded_data_cb)(uint8_t* data, uint16_t bytes_count);


/**
 * @typedef codec_write_decoded_data
 * @brief Write the decoded data to a system memory location. The number of samples to be written
 * is provided as the second argument. To get the bytes, multiply this by the size in bits of
 * a left/right sample.
 * This callback will be used by the decoder to put the decoded pcm samples in a location decided
 * by the API user. The implementation of the callback is provided by the API user.
 *
 * @param length The length of the data in number of pcm samples.
 * @return unsigned short 1 if SUCCESS, 0 for FAILURE
 */
typedef unsigned short (*codec_write_decoded_data_cb)(int16_t* data, uint16_t pcm_samples_count);

/**
 * @typedef codec_if_get_decoded_output_size
 * @brief For cbr codecs, get the number of bytes that would be needed for a single decoded pcm block.
 * @return int32_t Number of bytes that are required for a single block of PCM decoded data.
 */
typedef int32_t (*codec_if_get_decoded_output_size)(void);

/******************************************************
 *                    Structures
 ******************************************************/

struct wiced_codec_data_transfer_cb {
    /**
     * @fn alloc_output_buffer_fp
     * @brief alloc_buffer_fp is the playback system's output buffer allocation method. This function
     * must be typically provided by the underlying technology. For example, BT/A2DP will
     * provide its own alloc_buffer_fp, while Airplay would have its own. This <br>must</br> be
     * provided when <codec_if_instance>->init() is invoked.
     */
     codec_alloc_output_buffer_cb alloc_output_buffer_fp;

     /**
      * @fn read_encoded_data_fp
      * @brief read_encoded_data_fp is used to read in encoded data from the input queue. As in alloc_buffer_fp,
      * this function <br>must</br> be provided by the underlying playback system as an argument to
      * <codec_if_instance> -> init(). Note that the method name is from the viewpoint of the codec;
      * the codec would need this function to read data from the system using the codec.
      */
     codec_read_encoded_data_cb read_encoded_data_fp;

     /**
      * @fn write_decoded_data_fp
      * @brief write_decoded_data_fp is used to write decoded data back to the playback system. As in alloc_buffer_fp,
      * this function <br>must</br> be provided by the underlying playback system in <codec_if_instance> -> init().
      */
     codec_write_decoded_data_cb write_decoded_data_fp;
} ;


/**
 * codec_interface_t: each supported codec must provide this interface.
 *
 */
typedef struct codec_interface
{
    /**
     * @brief The major version of the interface.
     */
    int version_major;


    /**
     * @brief The minor version of the interface.
     */
    int version_minor;


    /**
     * @brief Indicates whether the interface implementation has been configured or not.
     */
    int configured;


    /**
     * @brief Indicates the codec type. Should be filled by the codec.
     */
    wiced_codec_type_t type;

    /**
     * @fn init
     * Initialize the codec with the required parameters.
     */
    codec_if_api_init init;

    /**
     * @close
     * De-initialize the codec.
     */
    codec_if_api_close close;

    /**
     * @fn getconfig
     * getconfig is an [in,out] function. It takes an argument void* and fills
     * it up with the supported codec information element in A2DP format.
     * This data will be sent to the SRC over AVDTP.
     */
    codec_if_api_get_capabilities get_capabilities;

    /**
     * @fn setconfig
     * setconfig takes the config requested by the SRC and uses the settings
     * in the codec.
     */
    codec_if_api_set_configuration set_configuration;

    /**
     * @fn encode
     * encode will encode the given audio samples and provide an encoded frame.
     */
    codec_if_api_encode encode;

    /**
     * @fn decode
     * decode will convert an encoded audio frame and provide PCM samples.
     */
    codec_if_api_decode decode;

    /**
     * @fn get_decoded_output_size
     * This returns the size of the array with decoded pcm samples from the decoder in bytes.
     * This is required when the size of the output is required and the decoder cannot just
     * yet be started after configuring the codec.
     */
    codec_if_get_decoded_output_size get_decoded_output_size;
} wiced_codec_interface_t;

typedef struct codec_interface* wiced_codec_handle_t;


/******************************************************
 *               Codec Factory Function Declarations
 ******************************************************/
/**
 * @fn wiced_codec_interface_init
 * @brief Initialize the codec interface from the API user. The api user can specify the codecs supported/visible to the
 * codec interface. If this function is not called or a null pointer provided, then the default table is used.
 * @param codec_table The table of codecs.
 */
void wiced_codec_interface_init(wiced_codec_handle_t* codec_table);


/**
 * @fn get_supported_codecs
 * @brief Get an array of codec ids supported on the platform.
 *
 * Implemented by API user if a different way of handling this is needed.
 * Look for an implementation of this function in wiced_codec_if.c.
 *
 * @param [in,out] codec_type_array The pointer to an array of size CODEC_MAX. This must be pre-
 * allocated by the caller of this function.
 * @param [out]     size The number of entries in the codec_type_array.
 * @return wiced_codec_status_t 0 if successful
 */
wiced_codec_status_t wiced_get_supported_codecs(uint8_t* codec_type_array, int16_t* size);


/**
 * @fn wiced_set_current_codec_type
 * @brief Register the codec selection with the framework.
 *
 * The chosen codec needs to be registered with the framework. If registered,
 * then the API user can retrieve the current codec's interface using the
 * wiced_get_selected_codec method. Note that registration merely implies that
 * the framework is notified of the intended usage of one of the codecs that is
 * already supported by it.
 *
 * @param codec The codec that the user wants to register with the framework for
 * immediate use.
 */
void wiced_register_selected_codec(wiced_codec_type_t codec);


/**
 * @fn wiced_get_registered_codec
 * @brief Get the codec that is currently in use and has been registered
 *
 * The codec in use should have been registered with the framework through the
 * wiced_set_current_codec_type api.
 * @return codec_interface_t* Pointer to the currently selected codec's interface.
 */
wiced_codec_interface_t* wiced_get_registered_codec(void);


/**
 * @fn wiced_get_codec_by_type
 * @brief Get the interface object for a given codec by specifying the codec type.
 *
 * @param type The codec type from the enum list in this file.
 * @return codec_interface_t* Pointer to the codec of type codec_type_t if supported
 *                            NULL if not supported.
 */
wiced_codec_interface_t* wiced_get_codec_by_type(wiced_codec_type_t type);

#ifdef __cplusplus
}
#endif

