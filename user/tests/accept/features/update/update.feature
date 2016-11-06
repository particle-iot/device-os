@update_result
Feature: Update result

  Scenario: Particle CLI is available
    When I run `which particle`
    Then the output should match /particle/

  Scenario: Flashing a correct binary OTA
    Given tinker application binary
    And I request the device id
    And I flash application binary OTA
    Then Flashing operation should have succeeded
    And I subscribe to device events
    And I wait for 30 seconds
    And I send the signal "INT" to the command started last
    Then the output should match /.*{"name":"spark.device.ota_result","data":"{\"r\":\"ok\".*/

  Scenario: Flashing a binary built for a different platform OTA
    Given tinker application binary built for wrong platform
    And I request the device id
    And I flash application binary OTA
    Then Flashing operation should have succeeded
    And I subscribe to device events
    And I wait for 30 seconds
    And I send the signal "INT" to the command started last
    Then the output should match /.*{"name":"spark.device.ota_result","data":"{\"r\":\"error\".*/

  Scenario: Flashing a correct binary over YModem
    Given tinker application binary
    And I put the device into listening mode
    And I flash application binary using Serial
    Then Flashing operation should have succeeded

  Scenario: Flashing a binary built for a different platform over YModem
    Given tinker application binary built for wrong platform
    And I put the device into listening mode
    And I flash application binary using Serial
    Then Flashing operation should have failed
    And I reset the device
