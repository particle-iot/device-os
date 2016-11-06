When(/^I reset the device$/) do
  Particle::DeviceControl.reset
end

When(/^I put the device into (dfu|listening|safe) mode$/) do |mode|
  Particle::DeviceControl.mode(mode)
end

When(/^I request the device id$/) do
  @device_id = Particle::DeviceControl.device_id
  raise 'Failed to get the device id from the device' unless @device_id
end

When(/^I subscribe to device events$/) do
    raise 'Device id unknown' unless @device_id
    step "I run `particle subscribe mine #{@device_id}` in background"
end