This is the code structure used in an Impromptu sequencer, to show how resets, clocks and run states are implemented.

```
void initRun() {
	// reposition run head to first step
	// ...
}


void Module::onReset() override {
	// initialize sequencer variables
	// ...
	running = true;
	initRun();
	clockIgnoreOnReset = (long) (0.001f * engineGetSampleRate());
}


void Module::fromJson(json_t *rootJ) override {
	// load saved sequencer variables
	// ...
	initRun();
}


void Module::step() override {
	// Run button
	if (runningTrigger.process(params[RUN_PARAM].value + inputs[RUNCV_INPUT].value)) {
		running = !running;
		if (running) {
			if (resetOnRun)// this is an option offered in the right-click menu in the sequencer
				initRun();
			if (resetOnRun || clockIgnoreOnRun)
				clockIgnoreOnReset = (long) (0.001f * engineGetSampleRate());
		}
		// ...
	}

	// Process user interactions (buttons, knobs, etc)
	// ...
	
	// Clock
	if (running && clockIgnoreOnReset == 0l) {
		if (clockTrigger.process(inputs[CLOCK_INPUT].value)) {
			// advance the sequencer
			// ...
		}
	}	
	
	// Reset
	if (resetTrigger.process(inputs[RESET_INPUT].value + params[RESET_PARAM].value)) {
		initRun();
		clockIgnoreOnReset = (long) (0.001f * engineGetSampleRate());
		clockTrigger.reset();
	}
	
	// Set the lights and the outputs of the sequencer (CV, gate, etc)
	// ...
			
	
	if (clockIgnoreOnReset > 0l)
		clockIgnoreOnReset--;
}// step()

```