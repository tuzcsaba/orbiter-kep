#include "GravityAssistModule.h"

GravityAssistModule::GravityAssistModule(HINSTANCE _hDLL, Optimization * optimizer) : m_optimizer(optimizer) {
	hDLL = _hDLL;
}

GravityAssistModule::~GravityAssistModule() {
}
