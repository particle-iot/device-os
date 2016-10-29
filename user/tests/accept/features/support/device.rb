module Particle
  module DeviceControl
    class << self
      def reset
        Util.exec("device_ctl reset")
        # Give the device some time to boot into the application
        # TODO: Wait for specific device state instead of fixed delay
        sleep(2)
      end

      def mode(mod)
        Util.exec("device_ctl mode #{mod}")
        sleep(2)
      end

      def device_id()
        @devid ||= Util.exec("device_ctl id")
        raise 'Failed to retrieve device id' unless @devid
        puts "Device ID: #{@devid}"
        return @devid
      end
    end
  end
end
