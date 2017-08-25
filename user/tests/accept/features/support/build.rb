module Particle
  module Build
    class << self
      # Last application path is cached in order to not flash the same application for every scenario
      @last_app_path = nil
      @last_bin_path = nil

      attr_reader :last_app_path

      def flash_app(app_path, platform=nil)
        if app_path != @last_app_path
          target_path = Util.cache_dir(File.join('build_target', app_path))
          if platform != nil
            Util.exec("flash_app -t #{target_path} -p #{platform} #{app_path}")
          else
            Util.exec("flash_app -t #{target_path} #{app_path}")
          end
          @last_app_path = app_path
          # Give the device some time to boot into the application
          # TODO: Wait for specific device state instead of fixed delay
          sleep(2)
        end
      end

      def build_app(app_path, platform=nil)
        target_path = Util.cache_dir(File.join('build_target', app_path))
        if platform != nil
          a = Util.exec("build_app -t #{target_path} -p #{platform} #{app_path}")
        else
          a = Util.exec("build_app -t #{target_path} #{app_path}")
        end
        @last_bin_path = File.join(target_path, File.basename(app_path) + '.bin')
        puts @last_bin_path
        puts target_path
        return @last_bin_path
      end

      def flash_app_cli(bin_path, flasher='dfu', devid=nil)
        incmd = 'yes'
        if flasher == 'ota' and !devid.nil?
          flasher = devid
          # todo - we can use the --force flag to skip the interrogation over bandwidth use
          if Particle::Util.env_var('PLATFORM') == 'electron'
            bytes = Particle::Util.cellular_data_usage(File.size(bin_path))
            incmd = "echo #{bytes}"
          end
        else
          flasher = "--#{flasher}"
        end
        cmd = Util.shell_out("#{incmd} | particle flash #{flasher} #{bin_path}")
        cmd.run_command
        @last_flash_result = cmd.error?
        puts cmd.stdout
        puts cmd.stderr
        sleep(2)
        return @last_flash_result
      end

      def compile_block(block, feature_path, platform)
        path = File.join(feature_path, 'block.cpp')
        content = %(
        #include "Particle.h"
        void block() {
          #{block}
        }
      )
        main_dir = File.join(__dir__, '..', '..', '..', '..','..','main')
        makefile = File.join(main_dir, 'makefile')
        File.write(path, content)
        tmpdir =
        cmd = Util.shell_out("make -f #{makefile} -C #{feature_path} APPDIR=#{feature_path} TARGETDIR=. PLATFORM=#{platform}")
        cmd.run_command
        puts(cmd.command)
        cmd
      end
    end
  end
end
