#!/usr/bin/env bats
# Acceptance test for Tinker
#
# Checks the functionality across a few pins

# Prerequisites
# - Particle CLI
# - BATS https://github.com/sstephenson/bats
# - A device claimed to your account with Tinker flashed
# - A wire between 2 pins on the device

if [ -z "$DEVICE" -o -z "$PIN_WRITE" -o -z "$PIN_READ" ]; then
  echo "Usage: DEVICE=abc PIN_WRITE=x PIN_READ=y accept.bats"
  echo "DEVICE is the device name or ID to test"
  echo "PIN_WRITE is the output pin. Connect it with a wire to PIN_READ"
  echo "PIN_READ is the input pin"
  false
fi

@test "Write a pin and read the result" {
  # Configure pin as input
  run particle call ${DEVICE} digitalRead ${PIN_READ}

  # Test write high
  run particle call ${DEVICE} digitalWrite ${PIN_WRITE}:HIGH
  run particle call ${DEVICE} digitalRead ${PIN_READ}
  [ "$output" = "1" ]

  # Test write low
  run particle call ${DEVICE} digitalWrite ${PIN_WRITE}:LOW
  run particle call ${DEVICE} digitalRead ${PIN_READ}
  [ "$output" = "0" ]

  # Configure pin as input
  run particle call ${DEVICE} digitalRead ${PIN_WRITE}
}

