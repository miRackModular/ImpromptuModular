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

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// panelTheme
		// json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
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

	// struct PanelThemeItem : MenuItem {
		// BlankPanel *module;
		// int theme;
		// void onAction(EventAction &e) override {
			// module->panelTheme = theme;
		// }
		// void step() override {
			// rightText = (module->panelTheme == theme) ? "✔" : "";
		// }
	// };

	void appendContextMenu(Menu *menu) override {

		BlankPanel *module = dynamic_cast<BlankPanel*>(this->module);
		assert(module);

		// MenuLabel *spacerLabel = new MenuLabel();
		// menu->addChild(spacerLabel);

		// MenuLabel *themeLabel = new MenuLabel();
		// themeLabel->text = "Panel Theme";
		// menu->addChild(themeLabel);

		// PanelThemeItem *lightItem = new PanelThemeItem();
		// lightItem->text = lightPanelID;// ImpromptuModular.hpp
		// lightItem->module = module;
		// lightItem->theme = 0;
		// menu->addChild(lightItem);

		// PanelThemeItem *darkItem = new PanelThemeItem();
		// darkItem->text = darkPanelID;// ImpromptuModular.hpp
		// darkItem->module = module;
		// darkItem->theme = 1;
		// menu->addChild(darkItem);
	}	


	BlankPanelWidget(BlankPanel *module) : ModuleWidget(module) {
		// Main panel from Inkscape
        DynamicSVGPanel *panel = new DynamicSVGPanel();
        //panel->addPanel(APP->window->loadSvg(asset::plugin(plugin, "res/light/BlankPanel.svg")));
        panel->addPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/BlankPanel_dark.svg")));
        box.size = panel->box.size;
        //panel->mode = &module->panelTheme;
        addChild(panel);


		// Screws
		addChild(createDynamicScrew<IMScrew>(Vec(15, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(15, 365), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 365), &module->panelTheme));
		
	}
};

//Model *modelBlankPanel = createModel<BlankPanel, BlankPanelWidget>("Impromptu Modular", "Blank-Panel", "MISC - BlankPanel", BLANK_TAG);
Model *modelBlankPanel = createModel<BlankPanel, BlankPanelWidget>("Blank-Panel");
