module Particle
  module Device
    class << self
      def reset
        Util.exec("device_ctl reset")
        # Give the device some time to boot into the application
        # TODO: Wait for specific device state instead of fixed delay
        sleep(2)
      end
    end
  end
end
