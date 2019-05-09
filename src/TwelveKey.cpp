//***********************************************************************************************
//Chain-able keyboard module for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and Audible Instruments plugins by Andrew Belt and graphics  
//  from the Component Library by Wes Milholen. 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//
//Module inspired by:
//  * the Autodafe keyboard by Antonio Grazioli 
//  * the cf mixer by Clément Foulc
//  * Twisted Electrons' KeyChain 
//
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct TwelveKey : Module {
	enum ParamIds {
		OCTINC_PARAM,
		OCTDEC_PARAM,
		ENUMS(KEY_PARAMS, 12),
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		CV_INPUT,	
		OCT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		CV_OUTPUT,	
		OCT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		PRESS_LIGHT,// no longer used
		ENUMS(KEY_LIGHTS, 12),
		NUM_LIGHTS
	};
	
	// Need to save
	int panelTheme = 0;
	int octaveNum;// 0 to 9
	float cv;
	bool stateInternal;// false when pass through CV and Gate, true when CV and gate from this module
	
	// No need to save
	unsigned long noteLightCounter;// 0 when no key to light, downward step counter timer when key lit
	int lastKeyPressed;// 0 to 11

	
	unsigned int lightRefreshCounter = 0;
	//float gateLight = 0.0f;
	Trigger keyTriggers[12];
	Trigger gateInputTrigger;
	Trigger octIncTrigger;
	Trigger octDecTrigger;
	

	TwelveKey() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(KEY_PARAMS + 1, 0.0, 1.0, 0.0, "C# key");
		configParam(KEY_PARAMS + 3, 0.0, 1.0, 0.0, "D# key");
		configParam(KEY_PARAMS + 6, 0.0, 1.0, 0.0, "F# key");
		configParam(KEY_PARAMS + 8, 0.0, 1.0, 0.0, "G# key");
		configParam(KEY_PARAMS + 10, 0.0, 1.0, 0.0, "A# key");

		configParam(KEY_PARAMS + 0, 0.0, 1.0, 0.0, "C key");
		configParam(KEY_PARAMS + 2, 0.0, 1.0, 0.0, "D key");
		configParam(KEY_PARAMS + 4, 0.0, 1.0, 0.0, "E key");
		configParam(KEY_PARAMS + 5, 0.0, 1.0, 0.0, "F key");
		configParam(KEY_PARAMS + 7, 0.0, 1.0, 0.0, "G key");
		configParam(KEY_PARAMS + 9, 0.0, 1.0, 0.0, "A key");
		configParam(KEY_PARAMS + 11, 0.0, 1.0, 0.0, "B key");

		configParam(OCTDEC_PARAM, 0.0, 1.0, 0.0, "Oct down");
		configParam(OCTINC_PARAM, 0.0, 1.0, 0.0, "Oct up");
		
		onReset();
	}

	void onReset() override {
		octaveNum = 4;
		cv = 0.0f;
		stateInternal = inputs[GATE_INPUT].isConnected() ? false : true;
		noteLightCounter = 0ul;
		lastKeyPressed = 0;
	}

	void onRandomize() override {
		octaveNum = random::u32() % 10;
		cv = ((float)(octaveNum - 4)) + ((float)(random::u32() % 12)) / 12.0f;
		stateInternal = inputs[GATE_INPUT].isConnected() ? false : true;
		noteLightCounter = 0ul;
		lastKeyPressed = 0;
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		
		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// cv
		json_object_set_new(rootJ, "cv", json_real(cv));
		
		// octave
		json_object_set_new(rootJ, "octave", json_integer(octaveNum));
		
		// stateInternal
		json_object_set_new(rootJ, "stateInternal", json_boolean(stateInternal));

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);

		// cv
		json_t *cvJ = json_object_get(rootJ, "cv");
		if (cvJ)
			cv = json_number_value(cvJ);
		
		// octave
		json_t *octaveJ = json_object_get(rootJ, "octave");
		if (octaveJ)
			octaveNum = json_integer_value(octaveJ);

		// stateInternal
		json_t *stateInternalJ = json_object_get(rootJ, "stateInternal");
		if (stateInternalJ)
			stateInternal = json_is_true(stateInternalJ);
	}

	
	void process(const ProcessArgs &args) override {		
		static const float noteLightTime = 0.5f;// seconds
		
		//********** Buttons, knobs, switches and inputs **********
		
		bool upOctTrig = false;
		bool downOctTrig = false;
		
		if ((lightRefreshCounter & userInputsStepSkipMask) == 0) {
		
			// Octave buttons and input
			upOctTrig = octIncTrigger.process(params[OCTINC_PARAM].getValue());
			downOctTrig = octDecTrigger.process(params[OCTDEC_PARAM].getValue());
				
			// Keyboard buttons and gate input
			for (int i = 0; i < 12; i++) {
				if (keyTriggers[i].process(params[KEY_PARAMS + i].getValue())) {
					cv = ((float)(octaveNum - 4)) + ((float) i) / 12.0f;
					stateInternal = true;
					noteLightCounter = (unsigned long) (noteLightTime * args.sampleRate / displayRefreshStepSkips);
					lastKeyPressed = i;
				}
			}
		
		}// userInputs refresh
		
		
		if (inputs[OCT_INPUT].isConnected())
			octaveNum = ((int) std::floor(inputs[OCT_INPUT].getVoltage()));
		else if (upOctTrig)
			octaveNum++;
		else if (downOctTrig)
			octaveNum--;
		if (octaveNum > 9) octaveNum = 9;
		if (octaveNum < 0) octaveNum = 0;
		
		if (gateInputTrigger.process(inputs[GATE_INPUT].getVoltage())) {// no input refresh here, don't want propagation lag in long 12-key chain
			cv = inputs[CV_INPUT].getVoltage();			
			stateInternal = false;
		}
		
		
		//********** Outputs and lights **********
		
		// cv output
		outputs[CV_OUTPUT].setVoltage(cv);
		
		// gate output
		if (stateInternal == false) {// if receiving a key from left chain
			outputs[GATE_OUTPUT].setVoltage(inputs[GATE_INPUT].getVoltage());
		}
		else {// key from this
			outputs[GATE_OUTPUT].setVoltage((params[KEY_PARAMS + lastKeyPressed].getValue() > 0.5f) ? 10.0f : 0.0f);
		}
		
		// Octave output
		outputs[OCT_OUTPUT].setVoltage(std::round( (float)(octaveNum + 1) ));
		
		lightRefreshCounter++;
		if (lightRefreshCounter >= displayRefreshStepSkips) {
			lightRefreshCounter = 0;

			// Key lights
			for (int i = 0; i < 12; i++)
				lights[KEY_LIGHTS + i].setBrightness(( i == lastKeyPressed && (noteLightCounter > 0ul || params[KEY_PARAMS + i].getValue() > 0.5f)) ? 1.0f : 0.0f);
			
			if (noteLightCounter > 0ul)
				noteLightCounter--;
		}
	}
};


struct TwelveKeyWidget : ModuleWidget {
	SvgPanel* lightPanel;
	SvgPanel* darkPanel;

	struct OctaveNumDisplayWidget : TransparentWidget {
		int *octaveNum;
		std::shared_ptr<Font> font;
		
		OctaveNumDisplayWidget() {
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
			displayStr[0] = 0x30 + (char) (octaveNum != NULL ? *octaveNum : 4);
			displayStr[1] = 0;
			nvgText(args.vg, textPos.x, textPos.y, displayStr, NULL);
		}
	};
	
	
	struct PanelThemeItem : MenuItem {
		TwelveKey *module;
		int theme;
		void onAction(const event::Action &e) override {
			module->panelTheme = theme;
		}
		void step() override {
			rightText = (module->panelTheme == theme) ? "✔" : "";
		}
	};
	void appendContextMenu(Menu *menu) override {
		MenuLabel *spacerLabel = new MenuLabel();
		menu->addChild(spacerLabel);

		TwelveKey *module = dynamic_cast<TwelveKey*>(this->module);
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
		darkItem->text = darkPanelID;// ImpromptuModular.hpp
		darkItem->module = module;
		darkItem->theme = 1;
		menu->addChild(darkItem);
	}	
	
	
	TwelveKeyWidget(TwelveKey *module) {
		setModule(module);
		
		// Main panels from Inkscape
        lightPanel = new SvgPanel();
        lightPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/TwelveKey.svg")));
        box.size = lightPanel->box.size;
        addChild(lightPanel);
        darkPanel = new SvgPanel();
		darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/TwelveKey_dark.svg")));
		darkPanel->visible = false;
		addChild(darkPanel);

		// Screws
		addChild(createDynamicWidget<IMScrew>(Vec(15, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(15, 365), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));



		// ****** Top portion (keys) ******

		static const int offsetKeyLEDx = 12;
		static const int offsetKeyLEDy = 41;

		// Black keys
		addParam(createParam<InvisibleKey>(Vec(30, 40), module, TwelveKey::KEY_PARAMS + 1));
		addChild(createLight<MediumLight<GreenLight>>(Vec(30+offsetKeyLEDx, 40+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 1));
		addParam(createParam<InvisibleKey>(Vec(71, 40), module, TwelveKey::KEY_PARAMS + 3));
		addChild(createLight<MediumLight<GreenLight>>(Vec(71+offsetKeyLEDx, 40+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 3));
		addParam(createParam<InvisibleKey>(Vec(154, 40), module, TwelveKey::KEY_PARAMS + 6));
		addChild(createLight<MediumLight<GreenLight>>(Vec(154+offsetKeyLEDx, 40+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 6));
		addParam(createParam<InvisibleKey>(Vec(195, 40), module, TwelveKey::KEY_PARAMS + 8));
		addChild(createLight<MediumLight<GreenLight>>(Vec(195+offsetKeyLEDx, 40+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 8));
		addParam(createParam<InvisibleKey>(Vec(236, 40), module, TwelveKey::KEY_PARAMS + 10));
		addChild(createLight<MediumLight<GreenLight>>(Vec(236+offsetKeyLEDx, 40+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 10));

		// White keys
		addParam(createParam<InvisibleKey>(Vec(10, 112), module, TwelveKey::KEY_PARAMS + 0));
		addChild(createLight<MediumLight<GreenLight>>(Vec(10+offsetKeyLEDx, 112+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 0));
		addParam(createParam<InvisibleKey>(Vec(51, 112), module, TwelveKey::KEY_PARAMS + 2));
		addChild(createLight<MediumLight<GreenLight>>(Vec(51+offsetKeyLEDx, 112+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 2));
		addParam(createParam<InvisibleKey>(Vec(92, 112), module, TwelveKey::KEY_PARAMS + 4));
		addChild(createLight<MediumLight<GreenLight>>(Vec(92+offsetKeyLEDx, 112+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 4));
		addParam(createParam<InvisibleKey>(Vec(133, 112), module, TwelveKey::KEY_PARAMS + 5));
		addChild(createLight<MediumLight<GreenLight>>(Vec(133+offsetKeyLEDx, 112+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 5));
		addParam(createParam<InvisibleKey>(Vec(174, 112), module, TwelveKey::KEY_PARAMS + 7));
		addChild(createLight<MediumLight<GreenLight>>(Vec(174+offsetKeyLEDx, 112+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 7));
		addParam(createParam<InvisibleKey>(Vec(215, 112), module, TwelveKey::KEY_PARAMS + 9));
		addChild(createLight<MediumLight<GreenLight>>(Vec(215+offsetKeyLEDx, 112+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 9));
		addParam(createParam<InvisibleKey>(Vec(256, 112), module, TwelveKey::KEY_PARAMS + 11));
		addChild(createLight<MediumLight<GreenLight>>(Vec(256+offsetKeyLEDx, 112+offsetKeyLEDy), module, TwelveKey::KEY_LIGHTS + 11));
		
		
		// ****** Bottom portion ******

		// Column rulers (horizontal positions)
		static const int columnRulerL = 30;
		static const int columnRulerR = box.size.x - 25 - columnRulerL;
		static const int columnRulerM = box.size.x / 2 - 14;
		
		// Row rulers (vertical positions)
		static const int rowRuler0 = 220;
		static const int rowRulerStep = 49;
		static const int rowRuler1 = rowRuler0 + rowRulerStep;
		static const int rowRuler2 = rowRuler1 + rowRulerStep;
		
		// Left side inputs
		
		
		addInput(createDynamicPort<IMPort>(Vec(columnRulerL, rowRuler0), true, module, TwelveKey::CV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerL, rowRuler1), true, module, TwelveKey::GATE_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerL, rowRuler2), true, module, TwelveKey::OCT_INPUT, module ? &module->panelTheme : NULL));

		// Middle
		// Press LED (moved other controls below up by 16 px when removed, to better center)
		//addChild(createLight<MediumLight<GreenLight>>(Vec(columnRulerM + offsetMediumLight, rowRuler0 - 31 + offsetMediumLight), module, TwelveKey::PRESS_LIGHT));
		// Octave display
		OctaveNumDisplayWidget *octaveNumDisplay = new OctaveNumDisplayWidget();
		octaveNumDisplay->box.pos = Vec(columnRulerM + 2, rowRuler1 - 27 + vOffsetDisplay);
		octaveNumDisplay->box.size = Vec(24, 30);// 1 character
		octaveNumDisplay->octaveNum = module ? &module->octaveNum : NULL;
		addChild(octaveNumDisplay);
		
		// Octave buttons
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerM - 20 + offsetCKD6b, rowRuler2 - 26 + offsetCKD6b), module, TwelveKey::OCTDEC_PARAM, module ? &module->panelTheme : NULL));
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerM + 22 + offsetCKD6b, rowRuler2 - 26 + offsetCKD6b), module, TwelveKey::OCTINC_PARAM, module ? &module->panelTheme : NULL));
		
		// Right side outputs
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerR, rowRuler0), false, module, TwelveKey::CV_OUTPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerR, rowRuler1), false, module, TwelveKey::GATE_OUTPUT, module ? &module->panelTheme : NULL));
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerR, rowRuler2), false, module, TwelveKey::OCT_OUTPUT, module ? &module->panelTheme : NULL));
	}
	
	void step() override {
		if (module) {
			lightPanel->visible = ((((TwelveKey*)module)->panelTheme) == 0);
			darkPanel->visible  = ((((TwelveKey*)module)->panelTheme) == 1);
		}
		Widget::step();
	}
};

Model *modelTwelveKey = createModel<TwelveKey, TwelveKeyWidget>("Twelve-Key");

/*CHANGE LOG

*/