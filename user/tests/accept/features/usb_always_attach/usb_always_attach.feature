@usb_always_attach
Feature: USB always attaches
Background:
  Given the application usb_always_attach_app
  # Reset the device before running a scenario
  And I reset the device

  Scenario: Device with all USB classes (Serial, USBSerial1, Mouse/Keyboard) disabled still attaches to host and has Control Interface active
      When I send USB request
      """
      {
        "test": 1
      }
      """
      Then USB reply should contain
      """
      It works!
      """
