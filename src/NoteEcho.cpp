//***********************************************************************************************
//CV-Gate shift register (a.k.a. note delay) for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and Audible Instruments plugins by Andrew Belt and graphics  
//  from the Component Library by Pyer. 
//See ./LICENSE.md for all licenses
//See ./res/fonts/ for font licenses
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct NoteEcho : Module {	
	enum ParamIds {
		ENUMS(TAP_PARAMS, 3),
		ENUMS(ST_PARAMS, 3),
		ENUMS(VEL_PARAMS, 3),
		SD_PARAM,
		POLY_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		GATE_INPUT,
		VEL_INPUT,
		CLK_INPUT,
		TAPCV_INPUT,
		STCV_INPUT,
		VELCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		GATE_OUTPUT,
		VEL_OUTPUT,
		CLK_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		SD_LIGHT,
		ENUMS(GATE_LIGHTS, 16),
		NUM_LIGHTS
	};
	
	
	// Expander
	// none

	// Constants
	static const int length = 32;// must be at least 16 given tap init values
	static constexpr float delayInfoTime = 3.0f;// seconds
	
	
	// Need to save, no reset
	int panelTheme;
	float panelContrast;
	
	// Need to save, with reset
	int sampDelayClk;

	// No need to save, with reset
	int notifySource[3] = {0, 0, 0};// 0 (not really used), 1 (semi) 2 (vel)
	long notifyInfo[3] = {0l, 0l, 0l};// downward step counter when semi or vel to be displayed, 0 when normal tap display
	
	// No need to save, no reset
	RefreshCounter refresh;
	Trigger clkTrigger;
	Trigger sdTrigger;


	int getTapValue(int tapNum) {
		return (int)(params[TAP_PARAMS + tapNum].getValue() + 0.5f);
	}
	int getSemiValue(int tapNum) {
		return (int)(std::round(params[ST_PARAMS + tapNum].getValue()));
	}
	
	NoteEcho() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(POLY_PARAM, 1, 4, 1, "Input polyphony");
		paramQuantities[POLY_PARAM]->snapEnabled = true;
		configParam(SD_PARAM, 0.0f, 1.0f, 0.0f, "1 sample delay clk");
		
		char strBuf[32];
		for (int i = 0; i < 3; i++) {
			// tap knobs
			snprintf(strBuf, 32, "Tap %i position", i + 1);
			configParam(TAP_PARAMS + i, 0, (float)length, ((float)i) * 4.0f + 4.0f, strBuf);		
			paramQuantities[TAP_PARAMS + i]->snapEnabled = true;
			// tap knobs
			snprintf(strBuf, 32, "Tap %i semitone offset", i + 1);
			configParam(ST_PARAMS + i, -48.0f, 48.0f, 0.0f, strBuf);		
			paramQuantities[ST_PARAMS + i]->snapEnabled = true;
			// vel knobs
			snprintf(strBuf, 32, "Tap %i velocity percent", i + 1);
			configParam(VEL_PARAMS + i, 0.0f, 1.0f, 0.0f, strBuf, " %", 0.0f, 100.0f);		
		}
		
		configInput(CV_INPUT, "CV");
		configInput(GATE_INPUT, "Gate");
		configInput(VEL_INPUT, "Velocity");
		configInput(CLK_INPUT, "Clock");
		configInput(TAPCV_INPUT, "Tap CV");
		configInput(STCV_INPUT, "Semitone offset CV");
		configInput(VEL_INPUT, "Velocity percent CV");

		configOutput(CV_OUTPUT, "CV");
		configOutput(GATE_OUTPUT, "Gate");
		configOutput(VEL_OUTPUT, "Velocity");
		configOutput(CLK_OUTPUT, "Clock");
		
		configBypass(CV_INPUT, CV_OUTPUT);
		configBypass(GATE_INPUT, GATE_OUTPUT);
		configBypass(VEL_INPUT, VEL_OUTPUT);
		configBypass(CLK_INPUT, CLK_OUTPUT);

		onReset();
		
		loadThemeAndContrastFromDefault(&panelTheme, &panelContrast);
	}


	void onReset() override final {
		sampDelayClk = 0;
		resetNonJson(false);
	}
	void resetNonJson(bool delayed) {
	}

	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		
		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// panelContrast
		json_object_set_new(rootJ, "panelContrast", json_real(panelContrast));

		// sampDelayClk
		json_object_set_new(rootJ, "sampDelayClk", json_integer(sampDelayClk));

		return rootJ;
	}


	void dataFromJson(json_t *rootJ) override {
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);

		// panelContrast
		json_t *panelContrastJ = json_object_get(rootJ, "panelContrast");
		if (panelContrastJ)
			panelContrast = json_number_value(panelContrastJ);

		// sampDelayClk
		json_t *sampDelayClkJ = json_object_get(rootJ, "sampDelayClk");
		if (sampDelayClkJ)
			sampDelayClk = json_integer_value(sampDelayClkJ);

		resetNonJson(true);
	}


	void process(const ProcessArgs &args) override {
		// sample delay clk
		if (sdTrigger.process(params[SD_PARAM].getValue())) {
			sampDelayClk = sampDelayClk + 1;
			if (sampDelayClk > 1) {
				sampDelayClk = 0;
			}
		}


		if (refresh.processInputs()) {
		}// userInputs refresh
	
	
	
		// main code
		
		// outputs
		outputs[CV_OUTPUT].setVoltage(0.0f);
			
		
		// lights
		if (refresh.processLights()) {
			// sampDelayClk light
			lights[SD_LIGHT].setBrightness(sampDelayClk != 0 ? 1.0f : 0.0f);

			// info notification counters
			for (int i = 0; i < 3; i++) {
				notifyInfo[i]--;
				if (notifyInfo[i] < 0l)
					notifyInfo[i] = 0l;
			}
		}// lightRefreshCounter
	}// process()
};


struct NoteEchoWidget : ModuleWidget {
	struct TapKnob : IMMediumKnob {
		NoteEcho *module = nullptr;
		int tapNum = 0;
		void onDragMove(const event::DragMove &e) override {
			if (module) {
				module->notifyInfo[tapNum] = 0l;
			}
			IMMediumKnob::onDragMove(e);
		}
	};
	struct SemitoneKnob : IMMediumKnob {
		NoteEcho *module = nullptr;
		int tapNum = 0;
		void onDragMove(const event::DragMove &e) override {
			if (module) {
				module->notifySource[tapNum] = 1l;
				module->notifyInfo[tapNum] = (long) (NoteEcho::delayInfoTime * APP->engine->getSampleRate() / RefreshCounter::displayRefreshStepSkips);
			}
			IMMediumKnob::onDragMove(e);
		}
	};
	struct VelKnob : IMMediumKnob {
		NoteEcho *module = nullptr;
		int tapNum = 0;
		void onDragMove(const event::DragMove &e) override {
			if (module) {
				module->notifySource[tapNum] = 2l;
				module->notifyInfo[tapNum] = (long) (NoteEcho::delayInfoTime * APP->engine->getSampleRate() / RefreshCounter::displayRefreshStepSkips);
			}
			IMMediumKnob::onDragMove(e);
		}
	};
	
	struct TapDisplayWidget : TransparentWidget {// a centered display, must derive from this
		NoteEcho *module = nullptr;
		int tapNum = 0;
		std::shared_ptr<Font> font;
		std::string fontPath;
		char displayStr[16] = {};
		static const int textFontSize = 15;
		static constexpr float textOffsetY = 19.9f; // 18.2f for 14 pt, 19.7f for 15pt
		
		TapDisplayWidget(int _tapNum, Vec _pos, Vec _size, NoteEcho *_module) {
			tapNum = _tapNum;
			box.size = _size;
			box.pos = _pos.minus(_size.div(2));
			module = _module;
			fontPath = std::string(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		}
		
		void drawLayer(const DrawArgs &args, int layer) override {
			if (layer == 1) {
				if (!(font = APP->window->loadFont(fontPath))) {
					return;
				}
				nvgFontSize(args.vg, textFontSize);
				nvgFontFaceId(args.vg, font->handle);
				nvgTextLetterSpacing(args.vg, -0.4);

				Vec textPos = VecPx(5.7f, textOffsetY);
				nvgFillColor(args.vg, nvgTransRGBA(displayColOn, 23));
				std::string initString(3,'~');
				nvgText(args.vg, textPos.x, textPos.y, initString.c_str(), NULL);
				
				nvgFillColor(args.vg, displayColOn);
				if (!module || module->getTapValue(tapNum) < 1) {
					snprintf(displayStr, 4, " - ");
				}
				else if (module->notifyInfo[tapNum] > 0l) {
					if (module->notifySource[tapNum] == 1) {
						// semitone
						int ist = module->getSemiValue(tapNum);
						snprintf(displayStr, 4, " %2u", (unsigned)(std::abs(ist)));
						if (ist != 0) {
							displayStr[0] = ist > 0 ? '+' : '-';
						}
					}
					else {
						// vel
						float vel = module->params[NoteEcho::VEL_PARAMS + tapNum].getValue();
						unsigned int ivel100 = (unsigned)(std::round(vel * 100.0f));
						snprintf(displayStr, 4, "%3u", ivel100);
					}
				}
				else {
					snprintf(displayStr, 4, "T%2u", (unsigned)(module->getTapValue(tapNum)));	
				}
				nvgText(args.vg, textPos.x, textPos.y, displayStr, NULL);
			}
		}
	};
	
	

	void appendContextMenu(Menu *menu) override {
		NoteEcho *module = static_cast<NoteEcho*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		
		createPanelThemeMenu(menu, &(module->panelTheme), &(module->panelContrast), static_cast<SvgPanel*>(getPanel()));

		// menu->addChild(new MenuSeparator());
		// menu->addChild(createMenuLabel("Settings"));
	}	
	
	
	NoteEchoWidget(NoteEcho *module) {
		setModule(module);
		int* mode = module ? &module->panelTheme : NULL;
		float* cont = module ? &module->panelContrast : NULL;
		
		// Main panel from Inkscape
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/NoteEcho.svg")));
		SvgPanel* svgPanel = static_cast<SvgPanel*>(getPanel());
		svgPanel->fb->addChildBottom(new PanelBaseWidget(svgPanel->box.size, cont));
		svgPanel->fb->addChild(new InverterWidget(svgPanel, mode));	
		
		// Screws
		svgPanel->fb->addChild(createDynamicScrew<IMScrew>(VecPx(15, 0), mode));
		svgPanel->fb->addChild(createDynamicScrew<IMScrew>(VecPx(15, 365), mode));
		svgPanel->fb->addChild(createDynamicScrew<IMScrew>(VecPx(box.size.x-30, 0), mode));
		svgPanel->fb->addChild(createDynamicScrew<IMScrew>(VecPx(box.size.x-30, 365), mode));


		static const float col5d = 16.0f;
		static const float col51 = 8.64f;// CV in/out
		static const float col52 = col51 + col5d;// Gate in/out
		static const float col53 = col52 + col5d;// Vel in/out
		static const float col54 = col53 + col5d;// Clk in/out
		static const float col55 = col54 + col5d;// sd and gateLightMatrix
		
		static const float col54d = 2.0f;// CV in/out
		static const float col4d = (4.0f * col5d - 2.0f * col54d) / 3.0f;// CV in/out
		static const float col41 = col51 + col54d;// Tap
		static const float col42 = col41 + col4d;// Displays
		static const float col43 = col42 + col4d;// Semitone
		static const float col44 = col55 - col54d;// Vel
		
		static const float row0 = 21.0f;// CV, Gate, Vel, Clk inputs, and sampDelayClk
		static const float row1 = row0 + 20.0f;// tap 1
		static const float row23d = 15.0f;// taps 2 and 3
		static const float row5 = 110.5f;// CV, Gate, Vel, Clk outputs
		static const float row4 = row5 - 20.0f;// CV inputs and poly knob
		
		static const float displayWidths = 48; // 43 for 14pt, 46 for 15pt
		static const float displayHeights = 24; // 22 for 14pt, 24 for 15pt

		
		// row 0
		addInput(createDynamicPortCentered<IMPort>(mm2px(Vec(col51, row0)), true, module, NoteEcho::CV_INPUT, mode));
		addInput(createDynamicPortCentered<IMPort>(mm2px(Vec(col52, row0)), true, module, NoteEcho::GATE_INPUT, mode));
		addInput(createDynamicPortCentered<IMPort>(mm2px(Vec(col53, row0)), true, module, NoteEcho::VEL_INPUT, mode));
		addInput(createDynamicPortCentered<IMPort>(mm2px(Vec(col54, row0)), true, module, NoteEcho::CLK_INPUT, mode));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(col55, row0)), module, NoteEcho::SD_PARAM));
		addChild(createLightCentered<MediumLight<GreenLightIM>>(mm2px(Vec(col55, row0)), module, NoteEcho::SD_LIGHT));
		
		// rows 1-3
		for (int i = 0; i < 3; i++) {
			TapKnob* tapKnobP;
			addParam(tapKnobP = createDynamicParamCentered<TapKnob>(mm2px(Vec(col41, row1 + i * row23d)), module, NoteEcho::TAP_PARAMS + i, mode));	
			tapKnobP->module = module;
			tapKnobP->tapNum = i;
			
			// displays
			TapDisplayWidget* tapDisplayWidget = new TapDisplayWidget(i, mm2px(Vec(col42, row1 + i * row23d)), VecPx(displayWidths, displayHeights), module);
			addChild(tapDisplayWidget);
			svgPanel->fb->addChild(new DisplayBackground(tapDisplayWidget->box.pos, tapDisplayWidget->box.size, mode));
			
			SemitoneKnob* stKnobP;
			addParam(stKnobP = createDynamicParamCentered<SemitoneKnob>(mm2px(Vec(col43, row1 + i * row23d)), module, NoteEcho::ST_PARAMS + i, mode));	
			stKnobP->module = module;
			stKnobP->tapNum = i;
			
			VelKnob* velKnobP;			
			addParam(velKnobP = createDynamicParamCentered<VelKnob>(mm2px(Vec(col44, row1 + i * row23d)), module, NoteEcho::VEL_PARAMS + i, mode));	
			velKnobP->module = module;
			velKnobP->tapNum = i;
		}
		
		// row 4
		addInput(createDynamicPortCentered<IMPort>(mm2px(Vec(col41, row4)), true, module, NoteEcho::TAPCV_INPUT, mode));
		addParam(createDynamicParamCentered<IMFourPosSmallKnob>(mm2px(Vec(col42, row4)), module, NoteEcho::POLY_PARAM, mode));
		addInput(createDynamicPortCentered<IMPort>(mm2px(Vec(col43, row4)), true, module, NoteEcho::STCV_INPUT, mode));
		addInput(createDynamicPortCentered<IMPort>(mm2px(Vec(col44, row4)), true, module, NoteEcho::VELCV_INPUT, mode));
		
		// row 5
		addOutput(createDynamicPortCentered<IMPort>(mm2px(Vec(col51, row5)), false, module, NoteEcho::CV_OUTPUT, mode));
		addOutput(createDynamicPortCentered<IMPort>(mm2px(Vec(col52, row5)), false, module, NoteEcho::GATE_OUTPUT, mode));
		addOutput(createDynamicPortCentered<IMPort>(mm2px(Vec(col53, row5)), false, module, NoteEcho::VEL_OUTPUT, mode));
		addOutput(createDynamicPortCentered<IMPort>(mm2px(Vec(col54, row5)), false, module, NoteEcho::CLK_OUTPUT, mode));

		// gate light matrix
		static const float gldx = 2.0f;
		static const float gldy = 2.0f;
		float posx = col55 - gldx * 1.5f - 1.0f;
		for (int i = 0; i < 4; i++) {
			float posy = row5 - gldy * 1.5f;
			for (int j = 0; j < 4; j++) {
				addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(posx, posy)), module, NoteEcho::GATE_LIGHTS + i));
				posy += gldy;
			}
			posx += gldx;
		}
	}
};

Model *modelNoteEcho = createModel<NoteEcho, NoteEchoWidget>("NoteEcho");
