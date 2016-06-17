#ifndef GRAVITY_ASSIST_MODULE_H
#define GRAVITY_ASSIST_MODULE_H

#include <Windows.h>

class MGAFinder;
class Optimization;

class GravityAssistModule {

public:
	GravityAssistModule(HINSTANCE hDLL, Optimization * optimizer);

	/// Destructor
	~GravityAssistModule();

	Optimization &optimizer() { return *m_optimizer; }

protected:
	MGAFinder *m_mgaFinder;
	Optimization *m_optimizer;
	HINSTANCE hDLL;

};

#endif
