require 'singleton'
require 'hex_string'

module Particle
  class Usb
    include Singleton

    DEFAULT_REQUEST_TYPE = 10 # USBRequestType::REQUEST_TYPE_CUSTOM

    def initialize
      @reply = nil
      @except = nil
    end

    def send_request(type, data = '')
      begin
        data = data.to_hex_string(false)
        data = Util.exec("send_usb_req -r 80 -v 0 -i #{type} -x #{data}")
        @reply = data.to_byte_string
        @except = nil
      rescue Exception => e
        @reply = nil
        @except = e
      end
    end

    def take_reply
      raise @except if @except # Rethrow last exception
      raise "No reply data available" unless @reply
      r = @reply
      @reply = nil
      r
    end

    def last_error
      @except
    end

    def reset
      @reply = nil
      @except = nil
    end

    def wait_until_request_fails(type, data='', timeout=60)
      Timeout::timeout(timeout) do
        loop do
            sleep(0.1)
            send_request(type, data)
            break if @except
          end
      end
    end

    def wait_until_request_succeeds(type, data='', timeout=60)
      Timeout::timeout(timeout) do
        loop do
            sleep(0.1)
            send_request(type, data)
            break unless @except
          end
      end
    end
  end
end
