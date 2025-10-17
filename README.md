# ESP-React-Clean

static StateUpdateResult updateSta(JsonObject& root, LightState& lightState) {
    bool stateChanged = false;
    Serial.println("Received WS object:");
    serializeJsonPretty(root, Serial);
    stateChanged |= FormBuilder::updateValue(root, "test_text", lightState.testText);
    stateChanged |= FormBuilder::updateValue(root, "monthly_consumption_limit", lightState.monthlyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit", lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on", lightState.ledOn);
    stateChanged |= FormBuilder::updateValue(root, "test_number", lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown", lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea", lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit", lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on", lightState.ledOn);
    stateChanged |= FormBuilder::updateValue(root, "test_number", lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown", lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea", lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "gain", lightState.gain);
    return stateChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
  }

  static void read(LightState& s, JsonObject& root) {
    JsonArray sta = FormBuilder::createForm(root,"status","Status Form");
    FormBuilder::addTrendField(sta,"trend_data",AF::RW,lines(line("key1",hidden,"#8884d8","monotone"),line("key2",hidden,"#FF0000","monotone"),line("key3",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("lineChart"));
    FormBuilder::addTrendField(sta,"trend_data",AF::RW,lines(line("key1",hidden,"#8884d8","monotone"),line("key2",hidden,"#FF0000","monotone"),line("key3",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("barChart"));
    FormBuilder::addTrendField(sta,"trend_data",AF::RW,lines(line("key1",visible,"#8884d8","monotone"),line("key2",hidden,"#FF0000","step"),line("key3",visible,"#FF00FF","monotone"),line("key21",visible,"#556B2F","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("pieChart"));
    FormBuilder::addSwitchField(sta,"led_on",AF::RW,s.ledOn);          
    FormBuilder::addSwitchField(sta,"led_on",AF::R,s.ledOn);
    FormBuilder::addTextField (sta,"test_text",AF::RW,"Sample text from Medved"); 
    FormBuilder::addTextField(sta,"test_text",AF::R,"Sample text from Medved");
    FormBuilder::addNumberField(sta,"test_number",AF::RW,s.dailyConsumptionLimit,minVal(0),maxVal(100),format("0.00")); 
    FormBuilder::addNumberField(sta,"test_number",AF::R,s.dailyConsumptionLimit,minVal(0),maxVal(100),format("0.00"));
    FormBuilder::addCheckboxField(sta,"test_checkbox",AF::RW,s.ledOn); 
    FormBuilder::addCheckboxField(sta,"test_checkbox",AF::R,s.ledOn);
    FormBuilder::addSwitchField(sta,"test_switch",AF::RW,s.ledOn);     
    FormBuilder::addSwitchField(sta,"test_switch",AF::R,s.ledOn);
    FormBuilder::addDropdownField(sta,"test_dropdown",AF::RW,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option",3)); 
    FormBuilder::addDropdownField(sta,"test_dropdown",AF::R,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option",3));
    FormBuilder::addRadioField(sta,"test_dropdown",AF::RW,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option bla bla bla",3)); 
    FormBuilder::addRadioField(sta,"test_dropdown",AF::R,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option bla bla bla",3));
    FormBuilder::addTextareaField(sta,"test_textarea",AF::RW,s.textArea); 
    FormBuilder::addTextareaField(sta,"test_textarea",AF::R,s.textArea);
    FormBuilder::addSliderField(sta, "gain", AF::RW, s.gain, minVal(10), maxVal(60));
    FormBuilder::addSliderField(sta, "gain", AF::R, s.gain, minVal(10), maxVal(60));

    JsonArray set = FormBuilder::createForm(root,"settings","Settings Form");
    FormBuilder::addTrendField(set,"trend_data",AF::RW,lines(line("key1",hidden,"#8884d8","monotone"),line("key2",hidden,"#FF0000","monotone"),line("key3",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("lineChart"));
    FormBuilder::addTrendField(set,"trend_data",AF::RW,lines(line("key1",hidden,"#8884d8","monotone"),line("key2",hidden,"#FF0000","monotone"),line("key3",hidden,"#FF00FF","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("barChart"));
    FormBuilder::addTrendField(set,"trend_data",AF::RW,lines(line("key1",visible,"#8884d8","monotone"),line("key2",hidden,"#FF0000","step"),line("key3",visible,"#FF00FF","monotone"),line("key21",visible,"#556B2F","monotone")),xAxis("timestamp"),legend(true),tooltip(true),trendMaxPoints(120),mode("pieChart"));
    FormBuilder::addSwitchField(set,"led_on",AF::RW,s.ledOn);          
    FormBuilder::addSwitchField(set,"led_on",AF::R,s.ledOn);
    FormBuilder::addTextField (set,"test_text",AF::RW,"Sample text from Medved"); 
    FormBuilder::addTextField(set,"test_text",AF::R,"Sample text from Medved");
    FormBuilder::addNumberField(set,"test_number",AF::RW,s.dailyConsumptionLimit,minVal(0),maxVal(100),format("0.00")); 
    FormBuilder::addNumberField(set,"test_number",AF::R,s.dailyConsumptionLimit,minVal(0),maxVal(100),format("0.00"));
    FormBuilder::addCheckboxField(set,"test_checkbox",AF::RW,s.ledOn); 
    FormBuilder::addCheckboxField(set,"test_checkbox",AF::R,s.ledOn);
    FormBuilder::addSwitchField(set,"test_switch",AF::RW,s.ledOn);     
    FormBuilder::addSwitchField(set,"test_switch",AF::R,s.ledOn);
    FormBuilder::addDropdownField(set,"test_dropdown",AF::RW,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option",3)); 
    FormBuilder::addDropdownField(set,"test_dropdown",AF::R,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option",3));
    FormBuilder::addRadioField(set,"test_dropdown",AF::RW,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option bla bla bla",3)); 
    FormBuilder::addRadioField(set,"test_dropdown",AF::R,s.testDropdown,opt("Option1",1),opt("Option2",2),opt("Third option bla bla bla",3));
    FormBuilder::addTextareaField(set,"test_textarea",AF::RW,s.textArea); 
    FormBuilder::addTextareaField(set,"test_textarea",AF::R,s.textArea);
    FormBuilder::addSliderField(set, "gain", AF::RW, s.gain, minVal(10), maxVal(60));
    FormBuilder::addSliderField(set, "gain", AF::R, s.gain, minVal(10), maxVal(60));
}

  static StateUpdateResult update(JsonObject& root, LightState& lightState) {
    bool stateChanged = false;
    Serial.println("Received REST object :");
    serializeJsonPretty(root, Serial);
    stateChanged |= FormBuilder::updateValue(root, "test_text", lightState.testText);
    stateChanged |= FormBuilder::updateValue(root, "monthly_consumption_limit", lightState.monthlyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit", lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on", lightState.ledOn);
    stateChanged |= FormBuilder::updateValue(root, "test_number", lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown", lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea", lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "daily_consumption_limit", lightState.dailyConsumptionLimit);
    stateChanged |= FormBuilder::updateValue(root, "led_on", lightState.ledOn);
    stateChanged |= FormBuilder::updateValue(root, "test_number", lightState.testNumber);
    stateChanged |= FormBuilder::updateValue(root, "test_dropdown", lightState.testDropdown);
    stateChanged |= FormBuilder::updateValue(root, "test_textarea", lightState.textArea);
    stateChanged |= FormBuilder::updateValue(root, "gain", lightState.gain);

    return stateChanged ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
  }
