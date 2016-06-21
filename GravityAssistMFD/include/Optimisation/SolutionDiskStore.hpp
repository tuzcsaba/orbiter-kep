#ifndef GRAVITY_ASSIST_SOLUTION_DISK_H
#define GRAVITY_ASSIST_SOLUTION_DISK_H

#include <Orbitersdk.h>
#include <Windows.h>

#include <Optimisation\MGAPlan.hpp>

class SolutionDiskStore {

public:
	MGAPlan * LoadScenario(FILEHANDLE scn);
	void SaveScenario(FILEHANDLE scn, MGAPlan * plan);

	MGAPlan * LoadPlan(char * filename);

	void SavePlan(char * filename, MGAPlan * plan);
	int SavePlan(MGAPlan * plan);

private:
	MGAPlan * LoadStateFrom(FILE * scn);
	void SaveStateTo(FILE * scn, MGAPlan * plan);
};

#endif