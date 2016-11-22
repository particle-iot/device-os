require 'concurrent/synchronization'
require 'concurrent/utility/monotonic_time'
require 'em-rubyserial'
require 'singleton'

module Particle
  class SerialPort < EM::Connection
    def initialize
      @data = ''
      @lock = Concurrent::Synchronization::Lock.new
    end

    def close
      close_connection_after_writing
    end

    def write(data)
      send_data(to_serial(data))
    end

    def wait_until(timeout, condition)
      last_except = nil
      # Keep calling provided block until it returns 'true' for current data
      ok = @lock.wait_until(timeout) do
        begin
          condition.call(from_serial(@data))
        rescue Exception => e
          last_except = e
          false
        end
      end
      unless ok
        raise last_except if last_except # Rethrow last exception
        raise "Serial output doesn't match expected data" # Generic error message
      end
    end

    def wait_while(timeout, condition)
      # Keep calling provided block until it returns 'false' for current data
      failed = @lock.wait_until(timeout) do
        !condition.call(from_serial(@data))
      end
      raise "Serial output doesn't match expected data" if failed # Generic error message
    end

    # Called by EventMachine
    def receive_data(data)
      @lock.synchronize do
        @data.concat(data)
        @lock.broadcast # Notify waiters
      end
    end

    private

    def to_serial(data)
      if data.ascii_only?
        d = data.gsub("\n", "\r\n") # LF -> CRLF
        d.concat("\r\n") unless d.end_with("\r\n") # Append new line sequence
        d
      else
        data
      end
    end

    def from_serial(data)
      if data.ascii_only?
        d = data.gsub("\r\n", "\n") # CRLF -> LF
        d.rstrip! # Strip trailing whitespace characters
        d
      else
        data
      end
    end
  end

  class Serial
    include Singleton

    def initialize
      @ports = {
        'Serial' => Util.env_var('PARTICLE_SERIAL_DEV'),
        'Serial1' => Util.env_var('PARTICLE_SERIAL1_DEV'),
        'USBSerial1' => Util.env_var('PARTICLE_USB_SERIAL1_DEV')
      }

      @active_ports = {}
    end

    def open(port, baud = 9600, data_bits = 8)
      dev = @ports[port]
      raise "Unknown serial port: #{port}" unless dev
      p = @active_ports[port]
      if p
        p.close
        @active_ports.delete(port)
      end
      # Certain ports, such as USBSerial1, may take some time to get registered at the host
      wait_until_accessible(dev)
      p = EM.open_serial(dev, baud, data_bits, SerialPort)
      @active_ports[port] = p
    end

    def close(port)
      active_port(port).close
      @active_ports.delete(port)
    end

    def write(port, data)
      active_port(port).write(data)
    end

    def wait_until(port, timeout = 3, &condition)
      active_port(port).wait_until(timeout, condition)
    end

    def wait_while(port, timeout = 1, &condition)
      active_port(port).wait_while(timeout, condition)
    end

    def reset
      # Close all ports
      @active_ports.each_value do |p|
        p.close
      end
      @active_ports.clear
    end

    private

    def wait_until_accessible(file, timeout = 3)
      if !File.exist?(file) || !File.readable?(file) || !File.writable?(file)
        Timeout::timeout(timeout) do
          loop do
            sleep(0.1)
            break if File.exist?(file) && File.readable?(file) && File.writable?(file)
          end
        end
      end
    end

    def active_port(port)
      p = @active_ports[port]
      raise "Serial port is not open: #{port}" unless p
      p
    end
  end
end
