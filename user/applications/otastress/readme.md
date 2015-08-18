
Application to stress-test OTA updates.

- publishes a variable: status=ready when ready for an OTA update
- stores the OTA count and the number of failures and number of successes (a reset in between will cause this figure to not equal the total)
- uses system event for start of OTA and end of OTA to write OTA stats to EEPROM
- function cmd=reset to reset EEPROM stats
- function cmd=print to print EEPROM stats to serial


Serial output:

- output stats on startup.
- output stats when function cmd(reset) is executed (stats set to 0)
- output stats when function cmd=print is e

- when an OTA is started
- time and OTA start
- time and OTA end status (success failure)
- accumulative stats
