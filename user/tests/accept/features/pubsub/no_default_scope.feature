Feature: pubsub requires a scope and visibility for 0.8.0 firmware

  Scenario: publish API fails with a specific error message when visibility is not given
    When I compile the block:
    """c++
    Particle.publish("name", "value");
    """
    Then the compile fails with error "Please specify PUBLIC or PRIVATE"

  Scenario: subscribe API fails with a specific error message when scope is not given
    When I compile the block:
    """c++
    const (void* handler)(const char* topic, const char* data);
    Particle.subscribe("topic", handler);
    """
    Then the compile fails with error "Please specify MY_DEVICES or ALL_DEVICES"
