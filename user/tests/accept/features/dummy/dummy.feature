Feature: A dummy firmware feature

  Scenario: Particle CLI is available
    When I run `which particle`
    Then the output should match /particle/