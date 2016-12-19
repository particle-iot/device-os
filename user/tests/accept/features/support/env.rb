require 'bundler/setup'
require 'aruba/cucumber'
require 'concurrent/atomics'
require 'em/pure_ruby' # Pure implementation is required by em-rubyserial
require 'particle_spec'

# Runs EventMachine's event loop in background thread
class EventLoop
  def initialize
    started = Concurrent::Event.new
    @thread = Thread.new do
      EM.run do
        started.set
      end
    end
    started.wait
  end

  def shutdown
    EM.stop_event_loop
    @thread.join
  end
end

event_loop = EventLoop.new

at_exit do
  event_loop.shutdown
end

Before do |scenario|
  @current_scenario = scenario
end

After do |scenario|
  Particle::Serial.instance.reset
  Particle::Usb.instance.reset
end
