usb = Particle::Usb.instance

When(/^I send USB request(?: with type (\d+))?$/) do |type, data|
  type ||= Particle::Usb::DEFAULT_REQUEST_TYPE
  usb.send_request(type, data)
end

Then(/^USB reply should( not)? (be|contain|match|start with|end with)$/) do |neg, op, data|
  d = usb.take_reply
  if neg
    Particle::Util.should_not(op, d, data)
  else
    Particle::Util.should(op, d, data)
  end
end

Then(/^USB reply should( not)? (be empty)$/) do |neg, op|
  d = usb.take_reply
  if neg
    Particle::Util.should_not(op, d)
  else
    Particle::Util.should(op, d)
  end
end

Then(/^USB request should( not)? (fail|succeed)$/) do |neg, op|
  status = if op == 'fail'
    !!neg
  else
    !neg
  end
  e = usb.last_error
  raise e if e && status
  expect(!e).to eq(status)
end
