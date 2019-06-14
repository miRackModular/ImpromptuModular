//***********************************************************************************************
//Four channel note viewer module for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct FourView : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CV_INPUTS, 4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CV_OUTPUTS, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// Need to save
	int panelTheme;
	bool showSharp = true;

	// No need to save
	// none

	
	inline float quantize(float cv, bool enable) {
		return enable ? (std::round(cv * 12.0f) / 12.0f) : cv;
	}
	
	
	FourView() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();
		
		panelTheme = (loadDarkAsDefault() ? 1 : 0);
	}
	

	void onReset() override {
	}

	
	void onRandomize() override {
	}

	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// showSharp
		json_object_set_new(rootJ, "showSharp", json_boolean(showSharp));
		
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);

		// showSharp
		json_t *showSharpJ = json_object_get(rootJ, "showSharp");
		if (showSharpJ)
			showSharp = json_is_true(showSharpJ);
	}

	
	void process(const ProcessArgs &args) override {
		for (int i = 0; i < 4; i++)
			outputs[CV_OUTPUTS + i].setVoltage(inputs[CV_INPUTS + i].getVoltage());
	}
};


struct FourViewWidget : ModuleWidget {
	SvgPanel* darkPanel;

	struct NotesDisplayWidget : TransparentWidget {
		FourView* module;
		int baseIndex;
		std::shared_ptr<Font> font;
		char text[4];

		NotesDisplayWidget(Vec _pos, Vec _size, FourView* _module, int _baseIndex) {
			box.size = _size;
			box.pos = _pos.minus(_size.div(2));
			module = _module;
			baseIndex = _baseIndex;
			font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		}
		
		void cvToStr(int index2) {
			if (module != NULL && module->inputs[FourView::CV_INPUTS + baseIndex + index2].isConnected()) {
				float cvVal = module->inputs[FourView::CV_INPUTS + baseIndex + index2].getVoltage();
				printNote(cvVal, text, module->showSharp);
			}
			else
				snprintf(text, 4," - ");
		}

		void draw(const DrawArgs &args) override {
			NVGcolor textColor = prepareDisplay(args.vg, &box, 17);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextLetterSpacing(args.vg, -1.5);

			for (int i = 0; i < 2; i++) {
				Vec textPos = Vec(7.0f + i * 46.0f, 23.4f);
				nvgFillColor(args.vg, nvgTransRGBA(textColor, displayAlpha));
				nvgText(args.vg, textPos.x, textPos.y, "~~~", NULL);
				nvgFillColor(args.vg, textColor);
				cvToStr(i);
				nvgText(args.vg, textPos.x, textPos.y, text, NULL);
			}
		}
	};


	struct PanelThemeItem : MenuItem {
		FourView *module;
		void onAction(const event::Action &e) override {
			module->panelTheme ^= 0x1;
		}
	};
	struct SharpItem : MenuItem {
		FourView *module;
		void onAction(const event::Action &e) override {
			module->showSharp = !module->showSharp;
		}
	};
	void appendContextMenu(Menu *menu) override {
		MenuLabel *spacerLabel = new MenuLabel();
		menu->addChild(spacerLabel);

		FourView *module = dynamic_cast<FourView*>(this->module);
		assert(module);

		MenuLabel *themeLabel = new MenuLabel();
		themeLabel->text = "Panel Theme";
		menu->addChild(themeLabel);

		PanelThemeItem *darkItem = createMenuItem<PanelThemeItem>(darkPanelID, CHECKMARK(module->panelTheme));
		darkItem->module = module;
		menu->addChild(darkItem);
		
		menu->addChild(createMenuItem<DarkDefaultItem>("Dark as default", CHECKMARK(loadDarkAsDefault())));

		menu->addChild(new MenuLabel());// empty line
		
		MenuLabel *settingsLabel = new MenuLabel();
		settingsLabel->text = "Settings";
		menu->addChild(settingsLabel);
		
		SharpItem *shrpItem = createMenuItem<SharpItem>("Sharp (unchecked is flat)", CHECKMARK(module->showSharp));
		shrpItem->module = module;
		menu->addChild(shrpItem);
	}	
	
	
	FourViewWidget(FourView *module) {
		setModule(module);
		
		// Main panels from Inkscape
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/FourView.svg")));
        if (module) {
			darkPanel = new SvgPanel();
			darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/FourView_dark.svg")));
			darkPanel->visible = false;
			addChild(darkPanel);
		}

		// Screws
		addChild(createDynamicWidget<IMScrew>(Vec(15, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(15, 365), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));

		const int centerX = box.size.x / 2;

		// Notes display
		NotesDisplayWidget *displayNotes12 = new NotesDisplayWidget(Vec(centerX, 66), Vec(99, 29), module, 0);
		addChild(displayNotes12);
		NotesDisplayWidget *displayNotes34 = new NotesDisplayWidget(Vec(centerX, 122), Vec(99, 29), module, 2);
		addChild(displayNotes34);

		static const int rowRulerPort = 193;
		static const int portSpacingY = 45;
		static const int portOffsetX = 28;
		
		for (int i = 0; i < 4; i++) {
			addInput(createDynamicPortCentered<IMPort>(Vec(centerX - portOffsetX, rowRulerPort + i * portSpacingY), true, module, FourView::CV_INPUTS + i, module ? &module->panelTheme : NULL));	
			addOutput(createDynamicPortCentered<IMPort>(Vec(centerX + portOffsetX, rowRulerPort + i * portSpacingY), false, module, FourView::CV_OUTPUTS + i, module ? &module->panelTheme : NULL));
		}
	}
	
	void step() override {
		if (module) {
			panel->visible = ((((FourView*)module)->panelTheme) == 0);
			darkPanel->visible  = ((((FourView*)module)->panelTheme) == 1);
		}
		Widget::step();
	}
};

Model *modelFourView = createModel<FourView, FourViewWidget>("Four-View");

/*CHANGE LOG

*/