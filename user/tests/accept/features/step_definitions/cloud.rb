When(/^I publish a? (private )?event "([^"]*)"(?: with data "(.*)")?$/) do |priv, event, data|
    data ||= ''
    if priv
        priv = "--private"
    else
        priv = ''
    end
    step "I run `particle publish '#{event}' '#{data}' #{priv}`"
end
