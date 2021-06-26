//***********************************************************************************************
//Blank Panel for VCV Rack by Marc Boulé
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct BlankPanel : Module {

	int panelTheme = 1;


	BlankPanel() : Module(0, 0, 0, 0) {
		onReset();
	}

	void onReset() override {
	}

	void onRandomize() override {
	}

	json_t *toJson() override {
		json_t *rootJ = json_object();

		// panelTheme
		// json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		// panelTheme
		// json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		// if (panelThemeJ)
			// panelTheme = json_integer_value(panelThemeJ);
	}

	
	// Advances the module by 1 audio frame with duration 1.0 / engineGetSampleRate()
	void step() override {		
	}
};


struct BlankPanelWidget : ModuleWidget {

	BlankPanelWidget(BlankPanel *module) : ModuleWidget(module) {
        setPanel(SVG::load(assetPlugin(plugin, "res/light/BlankPanel.svg")));

		// Screws
		addChild(createDynamicScrew<IMScrew>(Vec(15, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(15, 365), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 365), &module->panelTheme));
		
	}
};

Model *modelBlankPanel = Model::create<BlankPanel, BlankPanelWidget>("Impromptu Modular", "Blank-Panel", "BlankPanel", BLANK_TAG);
