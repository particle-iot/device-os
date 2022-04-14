serial = Particle::Serial.instance

Given(/^I open (.*Serial.*)(?: with baud rate (\d+))?$/) do |port, baud|
  baud ||= 9600
  serial.open(port, baud)
end

When(/^I write to (.*Serial.*)$/) do |port, data|
  serial.write(port, data)
end

Then(/^(.*Serial.*) output should( not)? (be|contain|match|start with|end with)$/) do |port, neg, op, data|
  if neg
    serial.wait_while(port) do |d|
      Particle::Util.should_not(op, d, data)
    end
  else
    serial.wait_until(port) do |d|
      Particle::Util.should(op, d, data)
    end
  end
end

Then(/^(.*Serial.*) output should( not)? (be empty)$/) do |port, neg, op|
  if neg
    serial.wait_until(port) do |d|
      Particle::Util.should_not(op, d)
    end
  else
    serial.wait_while(port) do |d|
      Particle::Util.should(op, d)
    end
  end
end
