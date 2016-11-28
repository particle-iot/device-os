Given(/^the application (.+)$/) do |app_dir|
  feature_path = File.dirname(@current_scenario.location.file)
  app_path = File.join(feature_path, app_dir)
  Particle::Build.flash_app(app_path)
end
