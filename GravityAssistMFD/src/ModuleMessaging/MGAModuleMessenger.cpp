#include "ModuleMessaging/MGAModuleMessenger.h"

void MGAModuleMessenger::PutSolution(const Orbiterkep__TransXSolution &solution) const {
	char buf[100];
	auto times = solution.times;
	auto escape = solution.escape;
	auto arrival = solution.arrival;

	if (oapiGetTimeAcceleration() == 0 || oapiGetSimTime() < 5.0) return;

	ModMsgPut("escape.mjd", escape->mjd);
	ModMsgPut("escape.prograde", escape->prograde);
	ModMsgPut("escape.outward", escape->outward);
	ModMsgPut("escape.plane", escape->plane);

	ModMsgPut("n_dsm", (int)solution.n_dsms);
	ModMsgPut("n_flyby", (int)solution.n_flybyes);
	for (int i = 0; i < solution.n_dsms; ++i) {
		sprintf(buf, "dsm[%d].mjd", i);
		ModMsgPut(buf, solution.dsms[i]->mjd);
		sprintf(buf, "dsm[%d].prograde", i);
		ModMsgPut(buf, solution.dsms[i]->prograde);
		sprintf(buf, "dsm[%d].outward", i);
		ModMsgPut(buf, solution.dsms[i]->outward);
		sprintf(buf, "dsm[%d].plane", i);
		ModMsgPut(buf, solution.dsms[i]->plane);
	}

	for (int i = 0; i < solution.n_flybyes; ++i) {
		sprintf(buf, "flyby[%d].mjd", i);
		ModMsgPut(buf, solution.flybyes[i]->mjd);
		sprintf(buf, "flyby[%d].prograde", i);
		ModMsgPut(buf, solution.flybyes[i]->prograde);
		sprintf(buf, "flyby[%d].outward", i);
		ModMsgPut(buf, solution.flybyes[i]->outward);
		sprintf(buf, "flyby[%d].plane", i);
		ModMsgPut(buf, solution.flybyes[i]->plane);
	}

	ModMsgPut("arrival.mjd", arrival->mjd);
}