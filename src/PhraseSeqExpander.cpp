//***********************************************************************************************
//Expander module for PhraseSeq16/32, by Marc Boulé
//
//Based on code from the Fundamental and Audible Instruments plugins by Andrew Belt and graphics  
//  from the Component Library by Wes Milholen. 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//
//Module concept and design by Marc Boulé, Nigel Sixsmith
//
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct PhraseSeqExpander : Module {
	enum InputIds {
		GATE1CV_INPUT,
		GATE2CV_INPUT,
		TIEDCV_INPUT,
		SLIDECV_INPUT,
		MODECV_INPUT,// needs connected
		NUM_INPUTS
	};


	// Expander
	float consumerMessage[1] = {};// this module must read from here
	float producerMessage[1] = {};// mother will write into here


	// No need to save
	int panelTheme = 0;


	PhraseSeqExpander() {
		config(0, NUM_INPUTS, 0, 0);
		
		leftProducerMessage = producerMessage;
		leftConsumerMessage = consumerMessage;
	}


	void process(const ProcessArgs &args) override {		
		bool motherPresent = leftModule && (leftModule->model == modelPhraseSeq16 || leftModule->model == modelPhraseSeq32);
		if (motherPresent) {
			// To Mother
			float *producerMessage = reinterpret_cast<float*>(leftModule->rightProducerMessage);
			int i = 0;
			for (; i < NUM_INPUTS - 1; i++) {
				producerMessage[i] = inputs[i].getVoltage();
			}
			producerMessage[i] = (inputs[i].isConnected() ? inputs[i].getVoltage() : std::numeric_limits<float>::quiet_NaN());
		}		
			
		// From Mother
		panelTheme = (motherPresent ? clamp((int)(consumerMessage[0] + 0.5f), 0, 1) : 0);
	}// process()
};


struct PhraseSeqExpanderWidget : ModuleWidget {
	SvgPanel* darkPanel;
	
	PhraseSeqExpanderWidget(PhraseSeqExpander *module) {
		setModule(module);
	
		// Main panels from Inkscape
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/light/PhraseSeqExpander.svg")));
        if (module) {
			darkPanel = new SvgPanel();
			darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/dark/PhraseSeqExpander_dark.svg")));
			darkPanel->visible = false;
			addChild(darkPanel);
		}
		
		// Screws
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 0), module ? &module->panelTheme : NULL));
		addChild(createDynamicWidget<IMScrew>(Vec(box.size.x-30, 365), module ? &module->panelTheme : NULL));

		// Expansion module
		static const int rowRulerExpTop = 65;
		static const int rowSpacingExp = 60;
		static const int colRulerExp = 497 - 30 - 450;// PS16 is 2HP less than PS32
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 0), true, module, PhraseSeqExpander::GATE1CV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 1), true, module, PhraseSeqExpander::GATE2CV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 2), true, module, PhraseSeqExpander::TIEDCV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 3), true, module, PhraseSeqExpander::SLIDECV_INPUT, module ? &module->panelTheme : NULL));
		addInput(createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 4), true, module, PhraseSeqExpander::MODECV_INPUT, module ? &module->panelTheme : NULL));
	}
	
	void step() override {
		if (module) {
			panel->visible = ((((PhraseSeqExpander*)module)->panelTheme) == 0);
			darkPanel->visible  = ((((PhraseSeqExpander*)module)->panelTheme) == 1);
		}
		Widget::step();
	}
};

Model *modelPhraseSeqExpander = createModel<PhraseSeqExpander, PhraseSeqExpanderWidget>("Phrase-Seq-Expander");

/*CHANGE LOG

*/
