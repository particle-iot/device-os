Given(/^I wait for (.+) (seconds|second|milliseconds|millisecond)$/) do |val, unit|
  val = val.to_f
  if unit == 'milliseconds' || unit == 'millisecond'
    val /= 1000
  end
  sleep(val)
end

# Convert step arguments to integers where possible
Transform(/^(-?\d+)$/) do |val|
  val.to_i
end

Given(/^I have specified (.*) environmental variable$/) do |var|
  raise "#{var} enironmental variable not set" unless Particle::Util.env_var(var)
end