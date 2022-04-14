@electron_modem_pause
Feature: Electron modem pause with SLEEP_NETWORK_STANDBY
Background:
  Given the application electron_modem_pause_app
  # Reset the device before running a scenario
  And I reset the device

  Scenario: Particle CLI is available
    When I run `which particle`
    Then the output should match /particle/

  Scenario: Sleepy device receives publishes after waking up
      When I wait for the device to reply to USB request
      And I send USB request
        """
        {
          "sleep": 1
        }
        """
      And I wait for the device to stop replying to USB request
      And I publish a private event "emdmpause" with data "All work and no play makes Jack a dull boy"
      And I wait for the device to reply to USB request
      And I wait for 5 seconds
      And I send USB request
        """
        {
          "data": 1
        }
        """
      Then USB reply should contain
        """
        All work and no play makes Jack a dull boy
        """
