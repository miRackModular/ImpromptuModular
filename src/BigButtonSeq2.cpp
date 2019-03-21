//***********************************************************************************************
//Six channel 128-step sequencer module for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//
//Based on the BigButton sequencer by Look-Mum-No-Computer
//https://www.youtube.com/watch?v=6ArDGcUqiWM
//https://www.lookmumnocomputer.com/projects/#/big-button/
//
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct BigButtonSeq2 : Module {
	enum ParamIds {
		CHAN_PARAM,
		LEN_PARAM,
		RND_PARAM,
		RESET_PARAM,
		CLEAR_PARAM,
		BANK_PARAM,
		DEL_PARAM,
		FILL_PARAM,
		BIG_PARAM,
		WRITEFILL_PARAM,
		QUANTIZEBIG_PARAM,
		SAMPLEHOLD_PARAM,
		CLOCK_PARAM,
		DISPMODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLK_INPUT,
		CHAN_INPUT,
		BIG_INPUT,
		LEN_INPUT,
		RND_INPUT,
		RESET_INPUT,
		CLEAR_INPUT,
		BANK_INPUT,
		DEL_INPUT,
		FILL_INPUT,
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CHAN_OUTPUTS, 6),
		ENUMS(CV_OUTPUTS, 6),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(CHAN_LIGHTS, 6 * 2),// Room for GreenRed
		BIG_LIGHT,
		BIGC_LIGHT,
		ENUMS(METRONOME_LIGHT, 2),// Room for GreenRed
		WRITEFILL_LIGHT,
		QUANTIZEBIG_LIGHT,
		SAMPLEHOLD_LIGHT,		
		NUM_LIGHTS
	};
	
	// Need to save
	int panelTheme = 0;
	int metronomeDiv = 4;
	bool writeFillsToMemory;
	bool quantizeBig;
	bool sampleAndHold;
	int indexStep;
	int bank[6];
	uint64_t gates[6][2][2];// channel , bank , 64x2 page for 128
	float cv[6][2][128];// channel , bank , indexStep
	
	// No need to save
	long clockIgnoreOnReset;
	double lastPeriod;//2.0 when not seen yet (init or stopped clock and went greater than 2s, which is max period supported for time-snap)
	double clockTime;//clock time counter (time since last clock)
	int pendingOp;// 0 means nothing pending, +1 means pending big button push, -1 means pending del
	float pendingCV;// 
	bool fillPressed;

	unsigned int lightRefreshCounter = 0;	
	float bigLight = 0.0f;
	float metronomeLightStart = 0.0f;
	float metronomeLightDiv = 0.0f;
	int channel = 0;
	int length = 0; 
	float sampleHoldBuf[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	Trigger clockTrigger;
	Trigger resetTrigger;
	Trigger bankTrigger;
	Trigger bigTrigger;
	Trigger clearTrigger;
	Trigger writeFillTrigger;
	Trigger quantizeBigTrigger;
	Trigger sampleHoldTrigger;
	Trigger internalSHTriggers[6];
	//dsp::PulseGenerator outPulse;
	dsp::PulseGenerator outLightPulse;
	dsp::PulseGenerator bigPulse;
	dsp::PulseGenerator bigLightPulse;

	
	inline bool getGate(int chan) {return !((gates[chan][bank[chan]][indexStep >> 6] & (((uint64_t)1) << (uint64_t)(indexStep & 0x3F))) == 0);}
	inline void setGate(int chan) {gates[chan][bank[chan]][indexStep >> 6] |= (((uint64_t)1) << (uint64_t)(indexStep & 0x3F));}
	inline void clearGate(int chan) {gates[chan][bank[chan]][indexStep >> 6] &= ~(((uint64_t)1) << (uint64_t)(indexStep & 0x3F));}
	inline void toggleGate(int chan) {gates[chan][bank[chan]][indexStep >> 6] ^= (((uint64_t)1) << (uint64_t)(indexStep & 0x3F));}
	inline void clearGates(int chan, int bnk) {gates[chan][bnk][0] = 0; gates[chan][bnk][1] = 0;}
	inline void randomizeGates(int chan, int bnk) {gates[chan][bnk][0] = random::u64(); gates[chan][bnk][1] = random::u64();}
	inline void writeCV(int chan, float cvValue) {cv[chan][bank[chan]][indexStep] = cvValue;}
	inline void writeCV(int chan, int bnk, int step, float cvValue) {cv[chan][bnk][step] = cvValue;}
	inline void sampleOutput(int chan) {sampleHoldBuf[chan] = cv[chan][bank[chan]][indexStep];}
	inline int calcChan() {
		float chanInputValue = inputs[CHAN_INPUT].getVoltage() / 10.0f * (6.0f - 1.0f);
		return (int) clamp(roundf(params[CHAN_PARAM].getValue() + chanInputValue), 0.0f, (6.0f - 1.0f));		
	}

	
	BigButtonSeq2() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);		
		
		params[RND_PARAM].config(0.0f, 100.0f, 0.0f, "Random");
		params[CHAN_PARAM].config(0.0f, 6.0f - 1.0f, 0.0f, "Channel");	
		params[LEN_PARAM].config(0.0f, 128.0f - 1.0f, 32.0f - 1.0f, "Length");
		params[DISPMODE_PARAM].config(0.0f, 1.0f, 0.0f, "Display mode");		
		params[WRITEFILL_PARAM].config(0.0f, 1.0f, 0.0f, "Write fill");
		params[BANK_PARAM].config(0.0f, 1.0f, 0.0f, "Bank");	
		params[CLOCK_PARAM].config(0.0f, 1.0f, 0.0f, "Clock step");	
		params[DEL_PARAM].config(0.0f, 1.0f, 0.0f, "Delete");	
		params[RESET_PARAM].config(0.0f, 1.0f, 0.0f, "Reset");	
		params[FILL_PARAM].config(0.0f, 1.0f, 0.0f, "Fill");	
		params[BIG_PARAM].config(0.0f, 1.0f, 0.0f, "Big button");
		params[QUANTIZEBIG_PARAM].config(0.0f, 1.0f, 0.0f, "Quantize big button");
		params[CLEAR_PARAM].config(0.0f, 1.0f, 0.0f, "Clear");	
		params[SAMPLEHOLD_PARAM].config(0.0f, 1.0f, 0.0f, "Sample & hold");
		
		onReset();
	}

	
	void onReset() override {
		writeFillsToMemory = false;
		quantizeBig = true;
		sampleAndHold = false;
		indexStep = 0;
		for (int c = 0; c < 6; c++) {
			bank[c] = 0;
			for (int b = 0; b < 2; b++) {
				clearGates(c, b);
				for (int s = 0; s < 128; s++)
					writeCV(c, b, s, 0.0f);
			}
		}
		clockIgnoreOnReset = (long) (clockIgnoreOnResetDuration * 96000);//args.sampleRate);
		lastPeriod = 2.0;
		clockTime = 0.0;
		pendingOp = 0;
		pendingCV = 0.0f;
		fillPressed = false;
	}


	void onRandomize() override {
		int chanRnd = calcChan();
		randomizeGates(chanRnd, bank[chanRnd]);
		for (int s = 0; s < 128; s++)
			writeCV(chanRnd, bank[chanRnd], s, ((float)(random::u32() % 7)) + ((float)(random::u32() % 12)) / 12.0f - 3.0f);
	}

	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// indexStep
		json_object_set_new(rootJ, "indexStep", json_integer(indexStep));

		// bank
		json_t *bankJ = json_array();
		for (int c = 0; c < 6; c++)
			json_array_insert_new(bankJ, c, json_integer(bank[c]));
		json_object_set_new(rootJ, "bank", bankJ);

		// gates LS64
		json_t *gatesLJ = json_array();
		for (int c = 0; c < 6; c++)
			for (int b = 0; b < 8; b++) {// bank to store is like uint64_t to store, so go to 8
				// first to get stored is 16 lsbits of bank 0, then next 16 bits,... to 16 msbits of bank 1
				unsigned int intValue = (unsigned int) ( (uint64_t)0xFFFF & (gates[c][b/4][0] >> (uint64_t)(16 * (b % 4))) );
				json_array_insert_new(gatesLJ, b + (c << 3) , json_integer(intValue));
			}
		json_object_set_new(rootJ, "gatesL", gatesLJ);
		// gates MS64
		json_t *gatesMJ = json_array();
		for (int c = 0; c < 6; c++)
			for (int b = 0; b < 8; b++) {// bank to store is like uint64_t to store, so go to 8
				// first to get stored is 16 lsbits of bank 0, then next 16 bits,... to 16 msbits of bank 1
				unsigned int intValue = (unsigned int) ( (uint64_t)0xFFFF & (gates[c][b/4][1] >> (uint64_t)(16 * (b % 4))) );
				json_array_insert_new(gatesMJ, b + (c << 3) , json_integer(intValue));
			}
		json_object_set_new(rootJ, "gatesM", gatesMJ);

		// CV bank 0
		json_t *cv0J = json_array();
		for (int c = 0; c < 6; c++) {
			for (int s = 0; s < 128; s++) {
				json_array_insert_new(cv0J, s + c * 128, json_real(cv[c][0][s]));
			}
		}
		json_object_set_new(rootJ, "cv0", cv0J);
		// CV bank 1
		json_t *cv1J = json_array();
		for (int c = 0; c < 6; c++) {
			for (int s = 0; s < 128; s++) {
				json_array_insert_new(cv1J, s + c * 128, json_real(cv[c][1][s]));
			}
		}
		json_object_set_new(rootJ, "cv1", cv1J);

		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// metronomeDiv
		json_object_set_new(rootJ, "metronomeDiv", json_integer(metronomeDiv));

		// writeFillsToMemory
		json_object_set_new(rootJ, "writeFillsToMemory", json_boolean(writeFillsToMemory));

		// quantizeBig
		json_object_set_new(rootJ, "quantizeBig", json_boolean(quantizeBig));

		// sampleAndHold
		json_object_set_new(rootJ, "sampleAndHold", json_boolean(sampleAndHold));

		return rootJ;
	}


	void dataFromJson(json_t *rootJ) override {
		// indexStep
		json_t *indexStepJ = json_object_get(rootJ, "indexStep");
		if (indexStepJ)
			indexStep = json_integer_value(indexStepJ);

		// bank
		json_t *bankJ = json_object_get(rootJ, "bank");
		if (bankJ)
			for (int c = 0; c < 6; c++)
			{
				json_t *bankArrayJ = json_array_get(bankJ, c);
				if (bankArrayJ)
					bank[c] = json_integer_value(bankArrayJ);
			}

		// gates LS64
		json_t *gatesLJ = json_object_get(rootJ, "gatesL");
		uint64_t bank8intsL[8] = {0,0,0,0,0,0,0,0};
		if (gatesLJ) {
			for (int c = 0; c < 6; c++) {
				for (int b = 0; b < 8; b++) {// bank to store is like to uint64_t to store, so go to 8
					// first to get read is 16 lsbits of bank 0, then next 16 bits,... to 16 msbits of bank 1
					json_t *gateLJ = json_array_get(gatesLJ, b + (c << 3));
					if (gateLJ)
						bank8intsL[b] = (uint64_t) json_integer_value(gateLJ);
				}
				gates[c][0][0] = bank8intsL[0] | (bank8intsL[1] << (uint64_t)16) | (bank8intsL[2] << (uint64_t)32) | (bank8intsL[3] << (uint64_t)48);
				gates[c][1][0] = bank8intsL[4] | (bank8intsL[5] << (uint64_t)16) | (bank8intsL[6] << (uint64_t)32) | (bank8intsL[7] << (uint64_t)48);
			}
		}
		// gates MS64
		json_t *gatesMJ = json_object_get(rootJ, "gatesM");
		uint64_t bank8intsM[8] = {0,0,0,0,0,0,0,0};
		if (gatesMJ) {
			for (int c = 0; c < 6; c++) {
				for (int b = 0; b < 8; b++) {// bank to store is like to uint64_t to store, so go to 8
					// first to get read is 16 lsbits of bank 0, then next 16 bits,... to 16 msbits of bank 1
					json_t *gateMJ = json_array_get(gatesMJ, b + (c << 3));
					if (gateMJ)
						bank8intsM[b] = (uint64_t) json_integer_value(gateMJ);
				}
				gates[c][0][1] = bank8intsM[0] | (bank8intsM[1] << (uint64_t)16) | (bank8intsM[2] << (uint64_t)32) | (bank8intsM[3] << (uint64_t)48);
				gates[c][1][1] = bank8intsM[4] | (bank8intsM[5] << (uint64_t)16) | (bank8intsM[6] << (uint64_t)32) | (bank8intsM[7] << (uint64_t)48);
			}
		}
		
		// CV bank 0
		json_t *cv0J = json_object_get(rootJ, "cv0");
		if (cv0J) {
			for (int c = 0; c < 6; c++)
				for (int s = 0; s < 128; s++) {
					json_t *cv0ArrayJ = json_array_get(cv0J, s + c * 128);
					if (cv0ArrayJ)
						cv[c][0][s] = json_number_value(cv0ArrayJ);
				}
		}
		// CV bank 1
		json_t *cv1J = json_object_get(rootJ, "cv1");
		if (cv1J) {
			for (int c = 0; c < 6; c++)
				for (int s = 0; s < 128; s++) {
					json_t *cv1ArrayJ = json_array_get(cv1J, s + c * 128);
					if (cv1ArrayJ)
						cv[c][1][s] = json_number_value(cv1ArrayJ);
				}
		}
		
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);

		// metronomeDiv
		json_t *metronomeDivJ = json_object_get(rootJ, "metronomeDiv");
		if (metronomeDivJ)
			metronomeDiv = json_integer_value(metronomeDivJ);

		// writeFillsToMemory
		json_t *writeFillsToMemoryJ = json_object_get(rootJ, "writeFillsToMemory");
		if (writeFillsToMemoryJ)
			writeFillsToMemory = json_is_true(writeFillsToMemoryJ);
		
		// quantizeBig
		json_t *quantizeBigJ = json_object_get(rootJ, "quantizeBig");
		if (quantizeBigJ)
			quantizeBig = json_is_true(quantizeBigJ);

		// sampleAndHold
		json_t *sampleAndHoldJ = json_object_get(rootJ, "sampleAndHold");
		if (sampleAndHoldJ)
			sampleAndHold = json_is_true(sampleAndHoldJ);
}

	
	void process(const ProcessArgs &args) override {
		double sampleTime = 1.0 / args.sampleRate;
		static const float lightTime = 0.1f;
		
		
		//********** Buttons, knobs, switches and inputs **********
		
		// Length
		length = (int) clamp(roundf( params[LEN_PARAM].getValue() + ( inputs[LEN_INPUT].isConnected() ? (inputs[LEN_INPUT].getVoltage() / 10.0f * (128.0f - 1.0f)) : 0.0f ) ), 0.0f, (128.0f - 1.0f)) + 1;	

		// Channel
		channel = calcChan();		
		
		if ((lightRefreshCounter & userInputsStepSkipMask) == 0) {		
		
			// Big button
			if (bigTrigger.process(params[BIG_PARAM].getValue() + inputs[BIG_INPUT].getVoltage())) {
				bigLight = 1.0f;
				if (quantizeBig && (clockTime > (lastPeriod / 2.0)) && (clockTime <= (lastPeriod * 1.01))) {// allow for 1% clock jitter
					pendingOp = 1;
					pendingCV = inputs[CV_INPUT].getVoltage();
				}
				else {
					if (!getGate(channel)) {
						setGate(channel);// bank and indexStep are global
						bigPulse.trigger(0.001f);
					}
					writeCV(channel, inputs[CV_INPUT].getVoltage());
					bigLightPulse.trigger(lightTime);
				}
			}

			// Bank button
			if (bankTrigger.process(params[BANK_PARAM].getValue() + inputs[BANK_INPUT].getVoltage()))
				bank[channel] = 1 - bank[channel];
			
			// Clear button
			if (clearTrigger.process(params[CLEAR_PARAM].getValue() + inputs[CLEAR_INPUT].getVoltage())) {
				clearGates(channel, bank[channel]);
				for (int s = 0; s < 128; s++)
					cv[channel][bank[channel]][s] = 0.0f;
			}
			
			// Write fill to memory
			if (writeFillTrigger.process(params[WRITEFILL_PARAM].getValue()))
				writeFillsToMemory = !writeFillsToMemory;

			// Quantize big button (aka snap)
			if (quantizeBigTrigger.process(params[QUANTIZEBIG_PARAM].getValue()))
				quantizeBig = !quantizeBig;

			// Sample and hold
			if (sampleHoldTrigger.process(params[SAMPLEHOLD_PARAM].getValue()))
				sampleAndHold = !sampleAndHold;
			
			// Del button
			if (params[DEL_PARAM].getValue() + inputs[DEL_INPUT].getVoltage() > 0.5f) {
				if (quantizeBig && (clockTime > (lastPeriod / 2.0)) && (clockTime <= (lastPeriod * 1.01)))// allow for 1% clock jitter
					pendingOp = -1;// overrides the pending write if it exists
				else {
					clearGate(channel);// bank and indexStep are global
					cv[channel][bank[channel]][indexStep] = 0.0f;
				}
			}

			// Pending timeout (write/del current step)
			if (pendingOp != 0 && clockTime > (lastPeriod * 1.01) ) 
				performPending(channel, lightTime);
			
		}// userInputs refresh
		
		
		
		//********** Clock and reset **********
		
		// Clock
		if (clockIgnoreOnReset == 0l) {			
			if (clockTrigger.process(inputs[CLK_INPUT].getVoltage() + params[CLOCK_PARAM].getValue())) {
				if ((++indexStep) >= length) indexStep = 0;
				
				// Fill button
				fillPressed = (params[FILL_PARAM].getValue() + inputs[FILL_INPUT].getVoltage()) > 0.5f;// used in clock block and others
				if (fillPressed && writeFillsToMemory) {
					setGate(channel);// bank and indexStep are global
					writeCV(channel, sampleHoldBuf[channel]);
				}
				
				//outPulse.trigger(0.001f);
				outLightPulse.trigger(lightTime);
				
				if (pendingOp != 0)
					performPending(channel, lightTime);// Proper pending write/del to next step which is now reached
				
				if (indexStep == 0)
					metronomeLightStart = 1.0f;
				metronomeLightDiv = ((indexStep % metronomeDiv) == 0 && indexStep != 0) ? 1.0f : 0.0f;
				
				// Random (toggle gate according to probability knob)
				float rnd01 = params[RND_PARAM].getValue() / 100.0f + inputs[RND_INPUT].getVoltage() / 10.0f;
				if (rnd01 > 0.0f) {
					if (random::uniform() < rnd01)// random::uniform is [0.0, 1.0), see include/util/common.hpp
						toggleGate(channel);
				}
				lastPeriod = clockTime > 2.0 ? 2.0 : clockTime;
				clockTime = 0.0;
			}
		}
			
		
		// Reset
		if (resetTrigger.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
			indexStep = 0;
			//outPulse.trigger(0.001f);
			outLightPulse.trigger(0.02f);
			metronomeLightStart = 1.0f;
			metronomeLightDiv = 0.0f;
			clockIgnoreOnReset = (long) (clockIgnoreOnResetDuration * args.sampleRate);
			clockTrigger.reset();
		}		
		
		
		
		//********** Outputs and lights **********
		
		
		// Gate outputs
		bool bigPulseState = bigPulse.process((float)sampleTime);
		bool outPulseState = clockTrigger.isHigh();
		bool retriggingOnReset = (clockIgnoreOnReset != 0l && retrigGatesOnReset);
		for (int i = 0; i < 6; i++) {
			bool gate = getGate(i);
			bool outSignal = ( ((gate || (i == channel && fillPressed)) && outPulseState) || (gate && bigPulseState && i == channel) );
			float outGateValue = outSignal ? 10.0f : 0.0f;
			if (internalSHTriggers[i].process(outGateValue))
				sampleOutput(i);
			outputs[CHAN_OUTPUTS + i].setVoltage((retriggingOnReset ? 0.0f : outGateValue));
			outputs[CV_OUTPUTS + i].setVoltage(sampleAndHold ? sampleHoldBuf[i] : cv[i][bank[i]][indexStep]);
		}

		
		lightRefreshCounter++;
		if (lightRefreshCounter >= displayRefreshStepSkips) {
			lightRefreshCounter = 0;

			// Gate light outputs
			bool bigLightPulseState = bigLightPulse.process((float)sampleTime * displayRefreshStepSkips);
			bool outLightPulseState = outLightPulse.process((float)sampleTime * displayRefreshStepSkips);
			float deltaTime = APP->engine->getSampleTime() * displayRefreshStepSkips;
			for (int i = 0; i < 6; i++) {
				bool gate = getGate(i);
				bool outLight  = (((gate || (i == channel && fillPressed)) && outLightPulseState) || (gate && bigLightPulseState && i == channel));
				lights[(CHAN_LIGHTS + i) * 2 + 1].setSmoothBrightness(outLight ? 1.0f : 0.0f, deltaTime);
				lights[(CHAN_LIGHTS + i) * 2 + 0].value = (i == channel ? (1.0f - lights[(CHAN_LIGHTS + i) * 2 + 1].value) / 2.0f : 0.0f);
			}

			// Big button lights
			lights[BIG_LIGHT].value = bank[channel] == 1 ? 1.0f : 0.0f;
			lights[BIGC_LIGHT].value = bigLight;
			
			// Metronome light
			lights[METRONOME_LIGHT + 1].value = metronomeLightStart;
			lights[METRONOME_LIGHT + 0].value = metronomeLightDiv;
		
			// Other push button lights
			lights[WRITEFILL_LIGHT].value = writeFillsToMemory ? 1.0f : 0.0f;
			lights[QUANTIZEBIG_LIGHT].value = quantizeBig ? 1.0f : 0.0f;
			lights[SAMPLEHOLD_LIGHT].value = sampleAndHold ? 1.0f : 0.0f;
		
			bigLight -= (bigLight / lightLambda) * (float)sampleTime * displayRefreshStepSkips;	
			metronomeLightStart -= (metronomeLightStart / lightLambda) * (float)sampleTime * displayRefreshStepSkips;	
			metronomeLightDiv -= (metronomeLightDiv / lightLambda) * (float)sampleTime * displayRefreshStepSkips;
		}
		
		clockTime += sampleTime;
		
		if (clockIgnoreOnReset > 0l)
			clockIgnoreOnReset--;
	}// step()
	
	
	inline void performPending(int chan, float lightTime) {
		if (pendingOp == 1) {
			if (!getGate(chan)) {
				setGate(chan);// bank and indexStep are global
				bigPulse.trigger(0.001f);
			}
			writeCV(chan, pendingCV);
			bigLightPulse.trigger(lightTime);
		}
		else {
			clearGate(chan);// bank and indexStep are global
		}
		pendingOp = 0;
	}
};


struct BigButtonSeq2Widget : ModuleWidget {


	struct ChanDisplayWidget : TransparentWidget {
		BigButtonSeq2 *module;
		std::shared_ptr<Font> font;
		
		ChanDisplayWidget() {
			font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		}

		void draw(const DrawArgs &args) override {
			NVGcolor textColor = prepareDisplay(args.vg, &box, 18);
			nvgFontFaceId(args.vg, font->handle);
			//nvgTextLetterSpacing(args.vg, 2.5);

			Vec textPos = Vec(6, 24);
			nvgFillColor(args.vg, nvgTransRGBA(textColor, displayAlpha));
			nvgText(args.vg, textPos.x, textPos.y, "~", NULL);
			nvgFillColor(args.vg, textColor);
			char displayStr[2];
			unsigned int channel = (unsigned)(module ? module->channel : 0);
			snprintf(displayStr, 2, "%1u", (unsigned) (channel + 1) );
			nvgText(args.vg, textPos.x, textPos.y, displayStr, NULL);
		}
	};

	struct StepsDisplayWidget : TransparentWidget {
		BigButtonSeq2 *module;
		std::shared_ptr<Font> font;
		
		StepsDisplayWidget() {
			font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		}

		void draw(const DrawArgs &args) override {
			NVGcolor textColor = prepareDisplay(args.vg, &box, 18);
			nvgFontFaceId(args.vg, font->handle);
			//nvgTextLetterSpacing(args.vg, 2.5);

			Vec textPos = Vec(6, 24);
			nvgFillColor(args.vg, nvgTransRGBA(textColor, displayAlpha));
			nvgText(args.vg, textPos.x, textPos.y, "~~~", NULL);
			nvgFillColor(args.vg, textColor);
			char displayStr[4];
			unsigned dispVal = 128;
			if (module)
				dispVal = (unsigned)(module->params[BigButtonSeq2::DISPMODE_PARAM].getValue() < 0.5f ?  module->length : module->indexStep + 1);
			snprintf(displayStr, 4, "%3u",  dispVal);
			nvgText(args.vg, textPos.x, textPos.y, displayStr, NULL);
		}
	};
	
	struct PanelThemeItem : MenuItem {
		BigButtonSeq2 *module;
		int theme;
		void onAction(const widget::ActionEvent &e) override {
			module->panelTheme = theme;
		}
		void step() override {
			rightText = (module->panelTheme == theme) ? "✔" : "";
		}
	};
	struct MetronomeItem : MenuItem {
		BigButtonSeq2 *module;
		int div;
		void onAction(const widget::ActionEvent &e) override {
			module->metronomeDiv = div;
		}
		void step() override {
			rightText = (module->metronomeDiv == div) ? "✔" : "";
		}
	};
	void appendContextMenu(Menu *menu) override {
		MenuLabel *spacerLabel = new MenuLabel();
		menu->addChild(spacerLabel);

		BigButtonSeq2 *module = dynamic_cast<BigButtonSeq2*>(this->module);
		assert(module);

		MenuLabel *themeLabel = new MenuLabel();
		themeLabel->text = "Panel Theme";
		menu->addChild(themeLabel);

		PanelThemeItem *lightItem = new PanelThemeItem();
		lightItem->text = lightPanelID;// ImpromptuModular.hpp
		lightItem->module = module;
		lightItem->theme = 0;
		menu->addChild(lightItem);

		PanelThemeItem *darkItem = new PanelThemeItem();
		std::string hotRodLabel = " Hot-rod";
		hotRodLabel.insert(0, darkPanelID);// ImpromptuModular.hpp
		darkItem->text = hotRodLabel;
		darkItem->module = module;
		darkItem->theme = 1;
		menu->addChild(darkItem);

		menu->addChild(new MenuLabel());// empty line
		
		MenuLabel *metronomeLabel = new MenuLabel();
		metronomeLabel->text = "Metronome light";
		menu->addChild(metronomeLabel);

		MetronomeItem *met1Item = createMenuItem<MetronomeItem>("Every clock", CHECKMARK(module->metronomeDiv == 1));
		met1Item->module = module;
		met1Item->div = 1;
		menu->addChild(met1Item);

		MetronomeItem *met2Item = createMenuItem<MetronomeItem>("/2", CHECKMARK(module->metronomeDiv == 2));
		met2Item->module = module;
		met2Item->div = 2;
		menu->addChild(met2Item);

		MetronomeItem *met4Item = createMenuItem<MetronomeItem>("/4", CHECKMARK(module->metronomeDiv == 4));
		met4Item->module = module;
		met4Item->div = 4;
		menu->addChild(met4Item);

		MetronomeItem *met8Item = createMenuItem<MetronomeItem>("/8", CHECKMARK(module->metronomeDiv == 8));
		met8Item->module = module;
		met8Item->div = 8;
		menu->addChild(met8Item);

		MetronomeItem *met1000Item = createMenuItem<MetronomeItem>("Full length", CHECKMARK(module->metronomeDiv == 1000));
		met1000Item->module = module;
		met1000Item->div = 1000;
		menu->addChild(met1000Item);
	}	
	
	
	BigButtonSeq2Widget(BigButtonSeq2 *module) {
		setModule(module);
		// Main panel from Inkscape
        DynamicSVGPanel *panel = new DynamicSVGPanel();
        panel->addPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/BigButtonSeq2.svg")));
        panel->addPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/BigButtonSeq2_dark.svg")));
        box.size = panel->box.size;
        panel->mode = module ? &module->panelTheme : NULL;
        addChild(panel);

		// Screws
		addChild(createDynamicScrew<IMScrew>(Vec(15, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicScrew<IMScrew>(Vec(15, 365), module ? &module->panelTheme : NULL));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));

		
		
		// Column rulers (horizontal positions)
		static const int colRulerCenter = 129;// not real center, center of big button
		static const int offsetChanOutX = 20;
		static const int rowRuler0 = 52;// chan and len knobs+displays
		static const int rowRuler1 = rowRuler0 + 48;// chan and len CVs, display switch
		static const int rowRuler2 = rowRuler1 + 40;// fillmem led button and CV in
		static const int rowRuler3 = rowRuler1 + 21;// bank
		static const int rowRuler4 = rowRuler3 + 29;// clear and del
		static const int rowRuler5 = rowRuler4 + 45;// reset and fill
		static const int colRulerT0 = colRulerCenter - offsetChanOutX * 5;
		static const int colRulerT1 = colRulerCenter - offsetChanOutX * 3;
		static const int colRulerT2 = colRulerCenter - offsetChanOutX * 1;
		static const int colRulerT3 = colRulerCenter + offsetChanOutX * 1;
		static const int colRulerT4 = colRulerCenter + offsetChanOutX * 3;
		static const int colRulerT5 = colRulerCenter + offsetChanOutX * 5;
		static const int colRulerT6 = colRulerT5 + 42;
		static const int clearAndDelButtonOffsetX = (colRulerCenter - colRulerT0) / 2 + 4;
		static const int knobCVjackOffsetY = 36;
		static const int lengthDisplayOffsetX = 20;
		
		
		// Rnd knob
		addParam(createDynamicParamCentered<IMSmallSnapKnob>(Vec(colRulerT0, rowRuler0), module, BigButtonSeq2::RND_PARAM, module ? &module->panelTheme : NULL));
		// Channel knob
		addParam(createDynamicParamCentered<IMSixPosBigKnob>(Vec(colRulerCenter - clearAndDelButtonOffsetX, rowRuler0), module, BigButtonSeq2::CHAN_PARAM, module ? &module->panelTheme : NULL));	
		// Channel display
		ChanDisplayWidget *displayChan = new ChanDisplayWidget();
		displayChan->box.pos = Vec(colRulerCenter - 12, rowRuler0 - 15);
		displayChan->box.size = Vec(24, 30);// 1 character
		displayChan->module = module;
		addChild(displayChan);	
		// Len knob
		addParam(createDynamicParamCentered<IMBigSnapKnob>(Vec(colRulerCenter + clearAndDelButtonOffsetX, rowRuler0), module, BigButtonSeq2::LEN_PARAM, module ? &module->panelTheme : NULL));
		// Length display
		StepsDisplayWidget *displaySteps = new StepsDisplayWidget();
		displaySteps->box.pos = Vec(colRulerT5 - 27 + lengthDisplayOffsetX, rowRuler0 - 15);
		displaySteps->box.size = Vec(55, 30);// 3 characters
		displaySteps->module = module;
		addChild(displaySteps);
		
		// Rnd jack
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerT0, rowRuler1), true, module, BigButtonSeq2::RND_INPUT, module ? &module->panelTheme : NULL));
		// Channel jack
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerCenter - clearAndDelButtonOffsetX, rowRuler1), true, module, BigButtonSeq2::CHAN_INPUT, module ? &module->panelTheme : NULL));
		// Length jack
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerCenter + clearAndDelButtonOffsetX, rowRuler1), true, module, BigButtonSeq2::LEN_INPUT, module ? &module->panelTheme : NULL));
		// Display switch
		addParam(createParamCentered<CKSSHNoRandom>(Vec(colRulerT5 + lengthDisplayOffsetX, rowRuler1 - 12), module, BigButtonSeq2::DISPMODE_PARAM));		


		// Metronome light
		addChild(createLightCentered<MediumLight<GreenRedLight>>(Vec(colRulerT0, rowRuler2), module, BigButtonSeq2::METRONOME_LIGHT + 0));
		// Mem fill LED button
		addParam(createParamCentered<LEDButton>(Vec(colRulerT5, rowRuler2), module, BigButtonSeq2::WRITEFILL_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(colRulerT5, rowRuler2), module, BigButtonSeq2::WRITEFILL_LIGHT));
		// CV Input
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerT6, rowRuler2), true, module, BigButtonSeq2::CV_INPUT, module ? &module->panelTheme : NULL));		

		
		// Bank button and jack
		addParam(createDynamicParamCentered<IMBigPushButton>(Vec(colRulerCenter, rowRuler3), module, BigButtonSeq2::BANK_PARAM, module ? &module->panelTheme : NULL));	
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerCenter, rowRuler3 + knobCVjackOffsetY), true, module, BigButtonSeq2::BANK_INPUT, module ? &module->panelTheme : NULL));
		// Clock button and jack
		addParam(createDynamicParamCentered<IMBigPushButton>(Vec(colRulerCenter - clearAndDelButtonOffsetX, rowRuler4), module, BigButtonSeq2::CLOCK_PARAM, module ? &module->panelTheme : NULL));	
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerCenter - clearAndDelButtonOffsetX, rowRuler4 + knobCVjackOffsetY), true, module, BigButtonSeq2::CLK_INPUT, module ? &module->panelTheme : NULL));
		// Del button and jack
		addParam(createDynamicParamCentered<IMBigPushButton>(Vec(colRulerCenter + clearAndDelButtonOffsetX, rowRuler4), module, BigButtonSeq2::DEL_PARAM, module ? &module->panelTheme : NULL));	
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerCenter + clearAndDelButtonOffsetX, rowRuler4 + knobCVjackOffsetY), true, module, BigButtonSeq2::DEL_INPUT, module ? &module->panelTheme : NULL));
		// Reset button and jack
		addParam(createDynamicParamCentered<IMBigPushButton>(Vec(colRulerT0, rowRuler5), module, BigButtonSeq2::RESET_PARAM, module ? &module->panelTheme : NULL));	
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerT0, rowRuler5 + knobCVjackOffsetY), true, module, BigButtonSeq2::RESET_INPUT, module ? &module->panelTheme : NULL));
		// Fill button and jack
		addParam(createDynamicParamCentered<IMBigPushButton>(Vec(colRulerT5, rowRuler5), module, BigButtonSeq2::FILL_PARAM,  module ? &module->panelTheme : NULL));	
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerT5, rowRuler5 + knobCVjackOffsetY), true, module, BigButtonSeq2::FILL_INPUT, module ? &module->panelTheme : NULL));

		// And now time for... BIG BUTTON!
		addChild(createLightCentered<GiantLight<RedLight>>(Vec(colRulerCenter, rowRuler5 + 22), module, BigButtonSeq2::BIG_LIGHT));
		addParam(createParamCentered<LEDBezelBig>(Vec(colRulerCenter, rowRuler5 + 22), module, BigButtonSeq2::BIG_PARAM));
		addChild(createLightCentered<GiantLight2<RedLight>>(Vec(colRulerCenter, rowRuler5 + 22), module, BigButtonSeq2::BIGC_LIGHT));
		// Big CV input
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerCenter - clearAndDelButtonOffsetX, rowRuler5 + knobCVjackOffsetY), true, module, BigButtonSeq2::BIG_INPUT, module ? &module->panelTheme : NULL));
		// Big snap
		addParam(createParamCentered<LEDButton>(Vec(colRulerCenter + clearAndDelButtonOffsetX, rowRuler5 + knobCVjackOffsetY), module, BigButtonSeq2::QUANTIZEBIG_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(colRulerCenter + clearAndDelButtonOffsetX, rowRuler5 + knobCVjackOffsetY), module, BigButtonSeq2::QUANTIZEBIG_LIGHT));

		// Clear button and jack
		addParam(createDynamicParamCentered<IMPushButton>(Vec(colRulerT6, rowRuler5), module, BigButtonSeq2::CLEAR_PARAM, module ? &module->panelTheme : NULL));	
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerT6, rowRuler5 + knobCVjackOffsetY), true, module, BigButtonSeq2::CLEAR_INPUT, module ? &module->panelTheme : NULL));

		
		// LEDs
		static const int rowRuler10 = 288;
		static const int ledOffsetY = -28;
		static const int gateOffsetY = 42;
		addChild(createLightCentered<MediumLight<GreenRedLight>>(Vec(colRulerT0, rowRuler10 + ledOffsetY), module, BigButtonSeq2::CHAN_LIGHTS + 0));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(Vec(colRulerT1, rowRuler10 + ledOffsetY), module, BigButtonSeq2::CHAN_LIGHTS + 2));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(Vec(colRulerT2, rowRuler10 + ledOffsetY), module, BigButtonSeq2::CHAN_LIGHTS + 4));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(Vec(colRulerT3, rowRuler10 + ledOffsetY), module, BigButtonSeq2::CHAN_LIGHTS + 6));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(Vec(colRulerT4, rowRuler10 + ledOffsetY), module, BigButtonSeq2::CHAN_LIGHTS + 8));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(Vec(colRulerT5, rowRuler10 + ledOffsetY), module, BigButtonSeq2::CHAN_LIGHTS + 10));
		// CV Outputs
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT0, rowRuler10), false, module, BigButtonSeq2::CV_OUTPUTS + 0, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT1, rowRuler10), false, module, BigButtonSeq2::CV_OUTPUTS + 1, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT2, rowRuler10), false, module, BigButtonSeq2::CV_OUTPUTS + 2, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT3, rowRuler10), false, module, BigButtonSeq2::CV_OUTPUTS + 3, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT4, rowRuler10), false, module, BigButtonSeq2::CV_OUTPUTS + 4, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT5, rowRuler10), false, module, BigButtonSeq2::CV_OUTPUTS + 5, module ? &module->panelTheme : NULL));
		// Gate outputs
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT0, rowRuler10 + gateOffsetY), false, module, BigButtonSeq2::CHAN_OUTPUTS + 0, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT1, rowRuler10 + gateOffsetY), false, module, BigButtonSeq2::CHAN_OUTPUTS + 1, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT2, rowRuler10 + gateOffsetY), false, module, BigButtonSeq2::CHAN_OUTPUTS + 2, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT3, rowRuler10 + gateOffsetY), false, module, BigButtonSeq2::CHAN_OUTPUTS + 3, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT4, rowRuler10 + gateOffsetY), false, module, BigButtonSeq2::CHAN_OUTPUTS + 4, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPortCentered<IMPort>(Vec(colRulerT5, rowRuler10 + gateOffsetY), false, module, BigButtonSeq2::CHAN_OUTPUTS + 5, module ? &module->panelTheme : NULL));
		
		// S&H LED button
		addParam(createParamCentered<LEDButton>(Vec(colRulerT6 + 2, rowRuler10 + gateOffsetY / 2), module, BigButtonSeq2::SAMPLEHOLD_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(colRulerT6 + 2, rowRuler10 + gateOffsetY / 2), module, BigButtonSeq2::SAMPLEHOLD_LIGHT));
	}
};

// Model *modelBigButtonSeq2 = createModel<BigButtonSeq2, BigButtonSeq2Widget>("Impromptu Modular", "Big-Button-Seq2", "SEQ - BigButtonSeq2", SEQUENCER_TAG);
Model *modelBigButtonSeq2 = createModel<BigButtonSeq2, BigButtonSeq2Widget>("Big-Button-Seq2");

/*CHANGE LOG

*/