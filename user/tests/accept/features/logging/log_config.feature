@log_config
Feature: Runtime logging configuration. See also user/tests/unit/logging.cpp


Background:
  Given the application log_config_app
  # Reset the device before running a scenario
  And I reset the device


Scenario: Logging is disabled initially
  Given I open Serial
  And I open Serial1
  # Ask the device to generate a log message
  When I send USB request
    """
    {
      "msg": "Test message 1"
    }
    """
  Then USB request should succeed
  And Serial output should be empty
  And Serial1 output should be empty


Scenario Outline: Enable logging on each of the serial interfaces
  # Send configuration request to the device (REQUEST_TYPE_LOG_CONFIG)
  When I send USB request with type 80
    """
    {
      "cmd": "addHandler",
      "id": "handler",
      "hnd": {
        "type": "StreamLogHandler"
      },
      "strm": {
        "type": "<Serial>"
      }
    }
    """
  And I open <Serial>
  And I send USB request
    """
    {
      "msg": "Test message"
    }
    """
  Then <Serial> output should contain
    """
    INFO: Test message
    """
  Examples:
    | Serial     |
    | Serial     |
    | Serial1    |
    | USBSerial1 |


Scenario: Enable multiple log handlers with different filtering settings
  Given I open Serial
  And I open Serial1
  # This handler logs only messages that are at or above warning level
  When I send USB request with type 80
    """
    {
      "cmd": "addHandler",
      "id": "handler1",
      "hnd": {
        "type": "StreamLogHandler"
      },
      "strm": {
        "type": "Serial"
      },
      "lvl": "warn"
    }
    """
  # This handler also logs all application messages regardless of their logging level
  And I send USB request with type 80
    """
    {
      "cmd": "addHandler",
      "id": "handler2",
      "hnd": {
        "type": "StreamLogHandler"
      },
      "strm": {
        "type": "Serial1",
      },
      "filt": [
        {
          "app": "all"
        }
      ],
      "lvl": "warn",
    }
    """
  And I send USB request
    """
    {
      "msg": "Test message 1",
      "level": "info",
      "cat": "system"
    }
    """
  Then USB request should succeed
  And Serial output should be empty
  And Serial1 output should be empty
  When I send USB request
    """
    {
      "msg": "Test message 2",
      "level": "info",
      "cat": "app"
    }
    """
  Then Serial output should be empty
  And Serial1 output should contain
    """
    INFO: Test message 2
    """
  When I send USB request
    """
    {
      "msg": "Test message 3",
      "level": "warn",
      "cat": "app"
    }
    """
  Then Serial output should contain
    """
    WARN: Test message 3
    """
  And Serial1 output should contain
    """
    WARN: Test message 3
    """


Scenario: Add, remove and enumerate active log handlers
  When I send USB request with type 80
    """
    {
      "cmd": "enumHandlers",
    }
    """
  Then USB reply should be
    """
    []
    """
  When I send USB request with type 80
    """
    {
      "cmd": "addHandler",
      "id": "handler1",
      "hnd": {
        "type": "StreamLogHandler"
      },
      "strm": {
        "type": "Serial",
      }
    }
    """
  And I send USB request with type 80
    """
    {
      "cmd": "enumHandlers",
    }
    """
  Then USB reply should be
    """
    ["handler1"]
    """
  When I send USB request with type 80
    """
    {
      "cmd": "addHandler",
      "id": "handler2",
      "hnd": {
        "type": "StreamLogHandler"
      },
      "strm": {
        "type": "Serial1",
      }
    }
    """
  And I send USB request with type 80
    """
    {
      "cmd": "enumHandlers",
    }
    """
  Then USB reply should be
    """
    ["handler1","handler2"]
    """
  When I send USB request with type 80
    """
    {
      "cmd": "removeHandler",
      "id": "handler1"
    }
    """
  And I send USB request with type 80
    """
    {
      "cmd": "enumHandlers",
    }
    """
  Then USB reply should be
    """
    ["handler2"]
    """
  When I send USB request with type 80
    """
    {
      "cmd": "removeHandler",
      "id": "handler2"
    }
    """
  And I send USB request with type 80
    """
    {
      "cmd": "enumHandlers",
    }
    """
  Then USB reply should be
    """
    []
    """
