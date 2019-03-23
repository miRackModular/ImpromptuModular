//***********************************************************************************************
//Expander module for Clocked, by Marc Boulé
//
//Based on code from the Fundamental and Audible Instruments plugins by Andrew Belt and graphics  
//  from the Component Library by Wes Milholen. 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//
//Module concept and design by Marc Boulé, Nigel Sixsmith, Xavier Belmont and Steve Baker
//
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct ClockedExpander : Module {
	enum InputIds {
		ENUMS(PW_INPUTS, 4),// master is index 0
		ENUMS(SWING_INPUTS, 4),// master is index 0
		NUM_INPUTS
	};


	// Need to save
	int panelTheme = 0;


	ClockedExpander() {
		config(0, NUM_INPUTS, 0, 0);
	
		onReset();
	}
	

	void onReset() override {
	}
	
	
	void onRandomize() override {
	}

	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		
		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		return rootJ;
	}


	void dataFromJson(json_t *rootJ) override {
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);
	}


	void process(const ProcessArgs &args) override {		
		if (leftModule && leftModule->model == modelClocked) {
			float *producerMessage = reinterpret_cast<float*>(leftModule->rightProducerMessage);
			for (int i = 0; i < 8; i++) {
				producerMessage[i * 2 + 0] = (inputs[i].isConnected() ? 1.0f : 0.0f);
				producerMessage[i * 2 + 1] = inputs[i].getVoltage();
			}
		}
		
	}// process()
};


struct ClockedExpanderWidget : ModuleWidget {

	struct PanelThemeItem : MenuItem {
		ClockedExpander *module;
		int theme;
		void onAction(const widget::ActionEvent &e) override {
			module->panelTheme = theme;
		}
		void step() override {
			rightText = (module->panelTheme == theme) ? "✔" : "";
		}
	};

	void appendContextMenu(Menu *menu) override {
		MenuLabel *spacerLabel = new MenuLabel();
		menu->addChild(spacerLabel);

		ClockedExpander *module = dynamic_cast<ClockedExpander*>(this->module);
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
	
	ClockedExpanderWidget(ClockedExpander *module) {
		setModule(module);
		
		// Main panel from Inkscape
        DynamicSVGPanel *panel = new DynamicSVGPanel();
        panel->mode = module ? &module->panelTheme : NULL;
        panel->addPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/ClockedExpander.svg")));
        panel->addPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/ClockedExpander_dark.svg")));
        box.size = panel->box.size;
        addChild(panel);		
		
		// Screws
		addChild(createDynamicScrew<IMScrew>(Vec(panel->box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicScrew<IMScrew>(Vec(panel->box.size.x-30, 365), module ? &module->panelTheme : NULL));

		// Expansion module
		static const int rowRulerExpTop = 60;
		static const int rowSpacingExp = 50;
		static const int colRulerExp = 497 - 30 -150 - 300;// Clocked is (2+10)HP less than PS32
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 0), true, module, ClockedExpander::PW_INPUTS + 0, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 1), true, module, ClockedExpander::PW_INPUTS + 1, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 2), true, module, ClockedExpander::PW_INPUTS + 2, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 3), true, module, ClockedExpander::SWING_INPUTS + 0, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 4), true, module, ClockedExpander::SWING_INPUTS + 1, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 5), true, module, ClockedExpander::SWING_INPUTS + 2, module ? &module->panelTheme : NULL));
	}
};

Model *modelClockedExpander = createModel<ClockedExpander, ClockedExpanderWidget>("Clocked-Expander");

/*CHANGE LOG

*/
