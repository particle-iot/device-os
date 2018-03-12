# ensure bytes are treated as bytes by tr or /xFF is output as a UTF-8 byte pair
export LC_CTYPE=C

gnumake -f wiced_test_mfg_combined.mk FIRMWARE=/spark/firmware WICED_SDK=/spark/photon-wiced  $*
