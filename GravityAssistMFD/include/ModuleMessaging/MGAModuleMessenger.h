#ifndef MGA_MODULE_MESSENGER_H
#define MGA_MODULE_MESSENGER_H

#include <EnjoLib/ModuleMessagingExtPut.hpp>

#include <orbiterkep/proto/solution.pb-c.h>

class MGAModuleMessenger : EnjoLib::ModuleMessagingExtPut
{
public:
	const char* ModuleMessagingGetModuleName() const { return "GravityAssistModule"; }
	
	void PutSolution(const Orbiterkep__TransXSolution &solution) const;
};

#endif

