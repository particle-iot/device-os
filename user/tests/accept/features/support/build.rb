module Particle
  module Build
    class << self
      # Last application path is cached in order to not flash the same application for every scenario
      @last_app_path = nil

      def flash_app(app_path)
        if app_path != @last_app_path
          target_path = Util.cache_dir(File.join('build_target', app_path))
          Util.exec("flash_app -t #{target_path} #{app_path}")
          @last_app_path = app_path
          # Give the device some time to boot into the application
          # TODO: Wait for specific device state instead of fixed delay
          sleep(2)
        end
      end
    end
  end
end
