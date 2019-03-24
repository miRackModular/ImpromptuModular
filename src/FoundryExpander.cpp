//***********************************************************************************************
//Expander module for Foundry, by Marc Boulé
//
//Based on code from the Fundamental and Audible Instruments plugins by Andrew Belt and graphics  
//  from the Component Library by Wes Milholen. 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//
//Module concept and design by Marc Boulé, Nigel Sixsmith, Xavier Belmont and Steve Baker
//
//***********************************************************************************************


#include "FoundrySequencer.hpp"


struct FoundryExpander : Module {
	enum ParamIds {
		SYNC_SEQCV_PARAM,
		WRITEMODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GATECV_INPUT,
		GATEPCV_INPUT,
		TIEDCV_INPUT,
		SLIDECV_INPUT,
		ENUMS(VEL_INPUTS, Sequencer::NUM_TRACKS),
		TRKCV_INPUT,
		ENUMS(SEQCV_INPUTS, Sequencer::NUM_TRACKS),
		WRITE_SRC_INPUT,
		NUM_INPUTS
	};
	
	enum LightIds {
		ENUMS(WRITECV2_LIGHTS, Sequencer::NUM_TRACKS),
		ENUMS(WRITE_SEL_LIGHTS, 2),
		NUM_LIGHTS
	};
	
	// Expander
	float consumerMessage[1 + 4 + 2] = {};// this module must read from here
	float producerMessage[1 + 4 + 2] = {};// mother will write into here (panelTheme, WRITECV2_LIGHTS, WRITE_SEL_LIGHTS)


	// No need to save
	int panelTheme = 0;


	FoundryExpander() {
		config(NUM_PARAMS, NUM_INPUTS, 0, NUM_LIGHTS);
	
		params[SYNC_SEQCV_PARAM].config(0.0f, 1.0f, 0.0f, "Sync Seq#");// 1.0f is top position
		params[WRITEMODE_PARAM].config(0.0f, 1.0f, 0.0f, "Write mode");
	
		leftProducerMessage = producerMessage;
		leftConsumerMessage = consumerMessage;
	}


	void process(const ProcessArgs &args) override {		
		if (leftModule && leftModule->model == modelClocked) {
			float *producerMessage = reinterpret_cast<float*>(leftModule->rightProducerMessage);
			for (int i = 0; i < 8; i++) {
				producerMessage[i * 2 + 0] = (inputs[i].isConnected() ? 1.0f : 0.0f);
				producerMessage[i * 2 + 1] = inputs[i].getVoltage();
			}
			
			panelTheme = clamp((int)(consumerMessage[0] + 0.5f), 0, 1);
			// TODO read 6 lights from consumerMessage[]
		}		
	}// process()
};


struct FoundryExpanderWidget : ModuleWidget {
	SvgPanel* lightPanel;
	SvgPanel* darkPanel;
	
	FoundryExpanderWidget(FoundryExpander *module) {
		setModule(module);
	
		// Main panels from Inkscape
        lightPanel = new SvgPanel();
        lightPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/FoundryExpander.svg")));
        box.size = lightPanel->box.size;
        addChild(lightPanel);
        darkPanel = new SvgPanel();
		darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/FoundryExpander_dark.svg")));
		darkPanel->visible = false;
		addChild(darkPanel);

		// Screws
		addChild(createDynamicScrew<IMScrew>(Vec(15, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicScrew<IMScrew>(Vec(15, 365), module ? &module->panelTheme : NULL));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));

		// Expansion module
		static const int rowSpacingExp = 49;
		static const int colRulerExp = box.size.x - expWidth / 2;
		static const int colOffsetX = 44;
		static const int se = -10;
		
		// Seq A,B and track row
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp - colOffsetX, rowRulerBHigh - rowSpacingExp * 4 + 2*se), true, module, FoundryExpander::SEQCV_INPUTS + 0, module ? &module->panelTheme : NULL));

		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp, rowRulerBHigh - rowSpacingExp * 4 + 2*se), true, module, FoundryExpander::SEQCV_INPUTS + 2, module ? &module->panelTheme : NULL));
		
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp + colOffsetX, rowRulerBHigh - rowSpacingExp * 4 + 2*se), true, module, FoundryExpander::TRKCV_INPUT, module ? &module->panelTheme : NULL));
		
		// Seq C,D and write source cv 
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp - colOffsetX, rowRulerBHigh - rowSpacingExp * 3 + 2*se), true, module, FoundryExpander::SEQCV_INPUTS + 1, module ? &module->panelTheme : NULL));

		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp, rowRulerBHigh - rowSpacingExp * 3 + 2*se), true, module, FoundryExpander::SEQCV_INPUTS + 3, module ? &module->panelTheme : NULL));

		addParam(createParamCentered<CKSSNotify>(Vec(colRulerExp + colOffsetX, rowRulerBHigh - rowSpacingExp * 3 + 2*se), module, FoundryExpander::SYNC_SEQCV_PARAM));// 1.0f is top position

		
		// Gate, tied, slide
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp - colOffsetX, rowRulerBHigh - rowSpacingExp * 2 + se), true, module, FoundryExpander::GATECV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp, rowRulerBHigh - rowSpacingExp * 2 + se), true, module, FoundryExpander::TIEDCV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp + colOffsetX, rowRulerBHigh - rowSpacingExp * 2 + se), true, module, FoundryExpander::SLIDECV_INPUT, module ? &module->panelTheme : NULL));

		// GateP, left, right
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp - colOffsetX, rowRulerBHigh - rowSpacingExp * 1 + se), true, module, FoundryExpander::GATEPCV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp, rowRulerBHigh - rowSpacingExp * 1 + se), true, module, FoundryExpander::LEFTCV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp + colOffsetX, rowRulerBHigh - rowSpacingExp * 1 + se), true, module, FoundryExpander::RIGHTCV_INPUT, module ? &module->panelTheme : NULL));
	
		
		// before-last row
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp - colOffsetX, rowRulerBHigh), true, module, FoundryExpander::VEL_INPUTS + 0, module ? &module->panelTheme : NULL));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(colRulerExp - colOffsetX + writeLEDoffsetX, rowRulerBHigh + writeLEDoffsetY), module, FoundryExpander::WRITECV2_LIGHTS + 0));
		
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp, rowRulerBHigh), true, module, FoundryExpander::VEL_INPUTS + 2, module ? &module->panelTheme : NULL));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(colRulerExp - writeLEDoffsetX, rowRulerBHigh + writeLEDoffsetY), module, FoundryExpander::WRITECV2_LIGHTS + 2));

		addParam(createDynamicParamCentered<IMPushButton>(Vec(colRulerExp + colOffsetX, rowRulerBHigh + 18), module, FoundryExpander::WRITEMODE_PARAM, module ? &module->panelTheme : NULL));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(colRulerExp + colOffsetX - 12, rowRulerBHigh + 3), module, FoundryExpander::WRITE_SEL_LIGHTS + 0));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(colRulerExp + colOffsetX + 12, rowRulerBHigh + 3), module, FoundryExpander::WRITE_SEL_LIGHTS + 1));
		
		// last row
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp - colOffsetX, rowRulerBLow), true, module, FoundryExpander::VEL_INPUTS + 1, module ? &module->panelTheme : NULL));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(colRulerExp - colOffsetX + writeLEDoffsetX, rowRulerBLow - writeLEDoffsetY), module, FoundryExpander::WRITECV2_LIGHTS + 1));

		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp, rowRulerBLow), true, module, FoundryExpander::VEL_INPUTS + 3, module ? &module->panelTheme : NULL));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(colRulerExp - writeLEDoffsetX, rowRulerBLow - writeLEDoffsetY), module, FoundryExpander::WRITECV2_LIGHTS + 3));
		
		addInput(createDynamicPortCentered<IMPort>(Vec(colRulerExp + colOffsetX, rowRulerBLow), true, module, FoundryExpander::WRITE_SRC_INPUT, module ? &module->panelTheme : NULL));
	}
	
	void step() override {
		if (module) {
			lightPanel->visible = ((((FoundryExpander*)module)->panelTheme) == 0);
			darkPanel->visible  = ((((FoundryExpander*)module)->panelTheme) == 1);
		}
		Widget::step();
	}
};

Model *modelFoundryExpander = createModel<FoundryExpander, FoundryExpanderWidget>("Clocked-Expander");

/*CHANGE LOG

*/
