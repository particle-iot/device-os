Given(/^the application ([^ ]+)$/) do |app_dir|
  feature_path = File.dirname(@current_scenario.location.file)
  app_path = File.join(feature_path, app_dir)
  if app_path != Particle::Build.last_app_path
    step 'I put the device into dfu mode'
    step 'I wait for 5 seconds'
    Particle::Build.flash_app(app_path)
  end
end

Given(/^([^ ]+) application binary built for (.*)$/) do |app_dir, platform|
  feature_path = File.dirname(@current_scenario.location.file)
  app_path = File.join(feature_path, app_dir)
  if platform == 'wrong platform'
    # FIXME
    case Particle::Util.env_var('PLATFORM')
    when 'photon'
      platform = 'electron'
    when 'electron'
      platform = 'photon'
    when 'p1'
      platform = 'photon'
    else
      platform = 'photon'
    end
  end
  @last_bin_path = Particle::Build.build_app(app_path, platform.downcase)
end

Given(/^([^ ]+) application binary$/) do |app_dir|
  feature_path = File.dirname(@current_scenario.location.file)
  app_path = File.join(feature_path, app_dir)
  @last_bin_path = Particle::Build.build_app(app_path)
end

When(/^I flash application binary (?:using |over )?(Serial|OTA)$/) do |flasher|
  flasher = flasher.downcase
  if flasher == 'ota'
    devid = Particle::DeviceControl.device_id
    @last_flash_result = Particle::Build.flash_app_cli(@last_bin_path, flasher.downcase, devid)
  else
    @last_flash_result = Particle::Build.flash_app_cli(@last_bin_path, flasher.downcase)
  end
end

Then(/^Flashing operation should( not)? have (succeeded|failed)$/) do |neg, pass_fail|
    if pass_fail == 'succeeded' && neg
      pass_fail = 'failed'
    elsif pass_fail == 'failed' && neg
      pass_fail = 'succeeded'
    end
    if pass_fail == 'succeeded'
      raise 'Application binary was not flashed' if @last_flash_result
    else
      raise 'Application binary was flashed' unless @last_flash_result
    end
end
