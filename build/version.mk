VERSION_STRING = 3.2.1-p2.2

# PRODUCT_FIRMWARE_VERSION reported by default
# FIXME: Unclear if this is used, PRODUCT_FIRMWARE_VERSION defaults to 65535 every release
VERSION = 3211

CFLAGS += -DSYSTEM_VERSION_STRING=$(VERSION_STRING)
