# ensure bytes are treated as bytes by tr or /xFF is output as a UTF-8 byte pair
export LC_CTYPE=C

gnumake -f combined.mk $*
