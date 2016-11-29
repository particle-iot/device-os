require 'rspec/matchers'
require 'open3'

module Particle
  CACHE_DIR = File.join(Dir.getwd, '.cache')

  module Util
    class << self
      include RSpec::Matchers
      include ParticleSpec::Utils

      def should(op, val1, val2 = nil)
        case op
        when 'be'
          expect(val1).to eq(val2)
        when 'contain'
          expect(val1).to include(val2)
        when 'match'
          expect(val1).to match(val2)
        when 'start with'
          expect(val1).to start_with(val2)
        when 'end with'
          expect(val1).to end_with(val2)
        when 'be empty'
          expect(val1).to be_empty
        else
          raise "Unknown operation: #{op}"
        end
      end

      def should_not(op, val1, val2 = nil)
        case op
        when 'be'
          expect(val1).not_to eq(val2)
        when 'contain'
          expect(val1).not_to include(val2)
        when 'match'
          expect(val1).not_to match(val2)
        when 'start with'
          expect(val1).not_to start_with(val2)
        when 'end with'
          expect(val1).not_to end_with(val2)
        when 'be empty'
          expect(val1).not_to be_empty
        else
          raise "Unknown operation: #{op}"
        end
      end

      def exec(cmd)
        stdout, stderr, status = Open3.capture3(cmd)
        raise "External command has finished with an error (exit code: #{status.exitstatus})\n#{stderr}" unless status.success?
        stdout.rstrip
      end

      def cache_dir(subdir)
        path = CACHE_DIR
        if subdir
          path = File.join(path, subdir)
        end
        FileUtils.mkdir_p(path)
        path
      end

      def env_var(name)
        val = ENV[name]
        raise "Environment variable is not defined: #{name}" unless val
        val
      end

      def cellular_data_usage(fileSize)
        numChunks = (fileSize / 512).ceil
        perChunkOverhead = (16+29+28)
        controlOverhead = 48 + (6*(29+28))
        totalBytes = (numChunks * perChunkOverhead) + controlOverhead + fileSize

        res = (totalBytes / 1E6)
        return "#{format("%.3f", res)}"
      end
    end
  end
end
