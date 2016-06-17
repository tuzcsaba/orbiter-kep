#include "Optimisation/Optimizer.hpp"

#include <Orbitersdk.h>
#include <orbiterkep/opt/optimise-c.h>
#include <orbiterkep/proto/ext-c.h>

#include "ModuleMessaging\MGAModuleMessenger.h"
#include "Dialog/MGAFinder.hpp"

#include <ppltasks.h>
#include <iostream>
#include <string>


struct OptimThreadParam {
	HWND hDlg;
	Optimization * opt;
	Orbiterkep__Parameters * param;
};


char ** allocate_string_array(int n_str, int max_l) {
	char ** result = (char **)malloc(n_str * sizeof(char *));
	for (int i = 0; i < n_str; ++i) {
		result[i] = (char *)malloc((max_l + 1) * sizeof(char));
	}
	return result;
}

void deallocate_string_array(char * strings[], int n_str) {
	for (int i = 0; i < n_str; ++i) {
		free(strings[i]);
	}
	free(strings);
}

Optimization::~Optimization()
{
	Cancel();

	ResetSolutions();
	ResetParam();

	free(m_solutions);
	for (int i = 0; i < max_pareto; ++i) free(m_pareto[i]);
	free(m_pareto);
}

Optimization::Optimization(const MGAModuleMessenger &messenger) : m_messenger(messenger)
{
	m_computing = false;
	n_solutions = 0;
	m_n_pareto = 0;
	m_best_solution = 0;
	m_solutions = (Orbiterkep__TransXSolution **)malloc(25 * sizeof(Orbiterkep__TransXSolution *));
	m_pareto = (double **)malloc(sizeof(double*) * max_pareto);
	for (int i = 0; i < max_pareto; ++i) m_pareto[i] = (double *)malloc(sizeof(double) * 2);

	InitializeDefaultParam();
}

void Optimization::InitializeDefaultParam() {
	m_param = (Orbiterkep__Parameters *)malloc(sizeof(Orbiterkep__Parameters));
	orbiterkep__parameters__init(m_param);
	char ** planets = (char **)malloc(sizeof(char *) * 3);
	char * p = "earth";
	planets[0] = (char *)malloc(sizeof(char) * (strlen(p) + 1));
	strcpy_s(planets[0], strlen(p) + 1, p);
	p = "venus";
	planets[1] = (char *)malloc(sizeof(char) * (strlen(p) + 1));
	strcpy_s(planets[1], strlen(p) + 1, p);
	p = "mercury";
	planets[2] = (char *)malloc(sizeof(char) * (strlen(p) + 1));
	strcpy_s(planets[2], strlen(p) + 1, p);
	m_param->n_planets = 3;
	m_param->planets = planets;
	char ** single_obj_algos = (char **)malloc(sizeof(char *));
	single_obj_algos[0] = (char *)malloc(sizeof(char) * 4);
	strcpy_s(single_obj_algos[0], 4, "jde");
	m_param->n_single_objective_algos = 1;
	m_param->single_objective_algos = single_obj_algos;

	char ** multi_obj_algos = (char **)malloc(sizeof(char *));
	multi_obj_algos[0] = (char *)malloc(sizeof(char) * 6);
	strcpy_s(multi_obj_algos[0], 6, "nsga2");
	m_param->n_multi_objective_algos = 1;
	m_param->multi_objective_algos = multi_obj_algos;

	m_param->problem = (char *)malloc(sizeof(char) * 4);
	strcpy_s(m_param->problem, 4, "MGA");

	Orbiterkep__ParamBounds * t0 = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(t0);
	t0->has_lb = 1; t0->has_ub = 1;
	t0->lb = 51000; t0->ub = 56000;
	m_param->t0 = t0;

	Orbiterkep__ParamBounds * tof = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(tof);
	tof->has_lb = 1; tof->has_ub = 1;
	tof->lb = 0.1; tof->ub = 5.0;
	m_param->tof = tof;

	Orbiterkep__ParamBounds * vinf = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(vinf);
	vinf->has_lb = 1; vinf->has_ub = 1;
	vinf->lb = 0.1; vinf->ub = 12.0;
	m_param->vinf = vinf;

	Orbiterkep__ParamPaGMO * pagmo = (Orbiterkep__ParamPaGMO *)malloc(sizeof(Orbiterkep__ParamPaGMO));
	orbiterkep__param_pa_gmo__init(pagmo);
	pagmo->has_n_isl = 1; pagmo->n_isl = 8;
	pagmo->has_n_gen = 1; pagmo->n_gen = 100000;
	pagmo->has_population = 1; pagmo->population = 60;
	pagmo->has_mf = 1; pagmo->mf = 150;
	pagmo->has_mr = 1; pagmo->mr = 0.15;
	m_param->pagmo = pagmo;

	m_param->has_circularize = 1;
	m_param->circularize = 1;

	m_param->has_add_arr_vinf = 1;
	m_param->add_arr_vinf = 1;

	m_param->has_add_dep_vinf = 1;
	m_param->add_dep_vinf = 1;

	m_param->has_n_trials = 1;
	m_param->n_trials = 1;

	m_param->has_max_deltav = 1;
	m_param->max_deltav = 24000;

	m_param->has_dep_altitude = 1;
	m_param->dep_altitude = 300;

	m_param->has_arr_altitude = 1;
	m_param->arr_altitude = 300;

	m_param->has_multi_objective = 1;
	m_param->multi_objective = 0;

	m_param->has_use_db = 1;
	m_param->use_db = 0;

	m_param->has_use_spice = 1;
	m_param->use_spice = 0;

	m_param_unpacked = true;

	int hash = param_hash(*m_param);

	char cacheFile[40];
	sprintf_s(cacheFile, "cache/%ld", (unsigned int)hash);

	LoadPlan(cacheFile);
}

void Optimization::free_manual_param()
{
	for (unsigned int i = 0; i < m_param->n_planets; ++i) {
		free(m_param->planets[i]);
	}
	free(m_param->planets);

	for (unsigned int i = 0; i < m_param->n_single_objective_algos; ++i) {
		free(m_param->single_objective_algos[i]);
	}
	free(m_param->single_objective_algos);

	for (unsigned int i = 0; i < m_param->n_multi_objective_algos; ++i) {
		free(m_param->multi_objective_algos[i]);
	}
	free(m_param->multi_objective_algos);

	free(m_param->t0);
	free(m_param->tof);
	free(m_param->vinf);
	free(m_param->pagmo);

	free(m_param);
}

std::string Optimization::get_solution_times() const
{
	char result_buf[10000];
	int len = sprintf_transx_times(result_buf, m_best_solution->times);

	return std::string(result_buf, len);
}

std::string Optimization::get_solution_str_current_stage() const
{
	if (!has_solution()) return "";

	char result_buf[10000];
	int len = 0;


	double currentTime = oapiGetSimMJD();
	if (currentTime <= m_best_solution->escape->mjd + 1) { // if we are less than 1 day after planned launch, current stage is ESCAPE
		len += sprintf_transx_escape(result_buf + len, m_best_solution->escape);
	} else {
		int i = 0; int j = 0;
		int n_dsm = m_best_solution->n_dsms; int n_flyby = m_best_solution->n_flybyes;
		bool found = false;
		while (i < n_dsm || j < n_flyby) {
			if (i < n_dsm) {
				if (currentTime < m_best_solution->dsms[i]->mjd + 1) {
					len += sprintf_transx_dsm(result_buf + len, m_best_solution->dsms[i]);
					found = true;
					break;
				}
				++i;
			}
			if (j < n_flyby) {
				if (currentTime < m_best_solution->flybyes[j]->mjd + 1) {
					len += sprintf_transx_flyby(result_buf + len, m_best_solution->flybyes[j]);
					found = true;
					break;
				}
				++j;
			}
		}

		if (!found) {
			len += sprintf_transx_arrival(result_buf + len, m_best_solution->arrival);
		}
	}
	return std::string(result_buf, len);
}

void Optimization::Cancel() {
	m_cancel = true;
}

void Optimization::ResetSolutions() {
	for (int i = 0; i < n_solutions; ++i) {
		orbiterkep__trans_xsolution__free_unpacked(m_solutions[i], NULL);
		m_solutions[i] = 0;
	}
	n_solutions = 0;
	m_best_solution = 0;
}

void RunOptimization_thread(std::shared_ptr<OptimThreadParam> param) {

	char buf[2048];
	int i = 0;
	int len = orbiterkep__parameters__get_packed_size(param->param);
	orbiterkep__parameters__pack(param->param, (uint8_t *)buf);

	char sol_buf[16000];
	int sol_len = 0;

	param->opt->set_computing(true);
	int c = 0;
	while (c < 1 || (!param->opt->opt_found() && !param->opt->cancelled())) {
		sol_len = orbiterkep_optimize((const uint8_t *)buf, len, (uint8_t *)sol_buf);

		Orbiterkep__TransXSolution * solution = orbiterkep__trans_xsolution__unpack(NULL, sol_len, (uint8_t *)sol_buf);

		param->opt->AddSolution(solution);
		param->opt->Signal();
		++c;
	}
	param->opt->Signal();
	param->opt->set_computing(false);
};

void RunOptimization_pareto_thread(std::shared_ptr<OptimThreadParam> param) {

	char buf[2048];
	int i = 0;
	int len = orbiterkep__parameters__get_packed_size(param->param);
	orbiterkep__parameters__pack(param->param, (uint8_t *)buf);

	param->opt->set_computing(true);
	int c = 0;
	int n = 10000;
	orbiterkep_optimize_multi((const uint8_t *)buf, len, param->opt->pareto_buffer(), &n);
	*(param->opt->n_pareto()) = n;
	param->opt->Signal();
	param->opt->set_computing(false);
};

void Optimization::Signal() {
	Update(hDlg);
}

void Optimization::RunOptimization(HWND hDlg)
{
	m_cancel = false;
	
	OptimThreadParam _threadParam;
	auto threadParam = std::make_shared<OptimThreadParam>(_threadParam);
	threadParam->opt = this;
	threadParam->hDlg = hDlg;
	threadParam->param = m_param;

	m_optimization_task = concurrency::create_task([threadParam] {
		RunOptimization_thread(threadParam);
	});	
}

void Optimization::RunPareto(HWND hDlg) {
	m_cancel = false;

	OptimThreadParam _threadParam;
	auto threadParam = std::make_shared<OptimThreadParam>(_threadParam);
	threadParam->opt = this;
	threadParam->hDlg = hDlg;
	threadParam->param = m_param;

	m_optimization_task = concurrency::create_task([threadParam] {
		RunOptimization_pareto_thread(threadParam);
	});
}

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

unsigned int base64_encode(const unsigned char* bytes_to_encode, unsigned int in_len, unsigned char* encoded_buffer, unsigned int& out_len)
{
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3] = { 0, 0, 0 };
	unsigned char char_array_4[4] = { 0, 0, 0, 0 };

	out_len = 0;
	while (in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; i < 4; i++)
			{
				encoded_buffer[out_len++] = base64_chars[char_array_4[i]];
			}
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
		{
			char_array_3[j] = '\0';
		}

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; j < (i + 1); j++)
		{
			encoded_buffer[out_len++] = base64_chars[char_array_4[j]];
		}

		while (i++ < 3)
		{
			encoded_buffer[out_len++] = '=';
		}
	}

	return out_len;
}

unsigned int base64_decode(const unsigned char* encoded_string, unsigned int in_len, unsigned char* decoded_buffer, unsigned int& out_len)
{
	size_t i = 0;
	size_t j = 0;
	int in_ = 0;
	unsigned char char_array_3[3] = { 0, 0, 0 };
	unsigned char char_array_4[4] = { 0, 0, 0, 0 };

	out_len = 0;
	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
			{
				char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
			}

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; i < 3; i++)
			{
				decoded_buffer[out_len++] = char_array_3[i];
			}
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
		{
			char_array_4[j] = 0;
		}

		for (j = 0; j < 4; j++)
		{
			char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
		}

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
		{
			decoded_buffer[out_len++] = char_array_3[j];
		}
	}
	return out_len;
}

int readBase64FromFile(FILE * scn, char * key, unsigned char * outputBuf, unsigned int &out_l) {
	char buf[10000];
	char * str;

		if (!fgets(buf, 9999, scn)) {
			return -1;
		}
		if (!_stricmp(buf, key)) {
			return -1;
		}
		str = buf + strlen(key) + 1;

	int len = strlen((char *)str);
	base64_decode((unsigned char *)str, len, outputBuf, out_l);

	return 0;
}

void writeBase64ToFile(FILE * scn, char * key, const unsigned char * buf, int size)
{
	unsigned char base64[10000];
	unsigned int out_l;
	base64_encode(buf, size, base64, out_l);
	base64[out_l] = 0;
	
	char str[10000];
	sprintf_s(str, "%s %s\n", key, base64);
	fputs(str, scn);
}

int readIntFromFile(FILEHANDLE scn, char * key, int & result, bool scenario) {
	char *buf;
	if (scenario) {
		if (!oapiReadScenario_nextline(scn, buf)) {
			return -1;
		}
		if (_strnicmp(buf, key, strlen(key))) {
			return -1;
		}
		sscanf_s(buf + strlen(key) + 1, "%d", &result);
	} else {
		char b[10000];
		if (!fgets(b, 9999, (FILE *)scn)) {
			return -1;
		}
		if (!_stricmp(b, key)) {
			return -1;
		}
		if (0 == sscanf_s(b + strlen(key) + 1, "%ld", &result)) {
			return -1;
		}
	}	

	return 0;
}

void Optimization::ResetParam() {
	if (m_param) {
		if (m_param_unpacked) {
			orbiterkep__parameters__free_unpacked(m_param, NULL);
		} else {
			free_manual_param();
		}
		m_param = 0;
	}
}

void Optimization::LoadScenario(FILEHANDLE scn) {
	int hash;
	if (readIntFromFile(scn, "Hash", hash, true)) {
		return;
	}

	char cacheFile[40];
	sprintf_s(cacheFile, "cache/%ld", (unsigned int)hash);

	LoadPlan(cacheFile);
}

void Optimization::SaveScenario(FILEHANDLE scn) {

	int hash = SaveCurrentPlan();
	
	oapiWriteScenario_int(scn, "Hash", hash);
}

void Optimization::LoadStateFrom(FILE * scn)
{
	/** Load parameters */	
	unsigned char out[2048];
	unsigned int out_l;
	if (readBase64FromFile(scn, "Parameters", out, out_l)) {
		return;
	}

	Cancel();

	update_parameters(orbiterkep__parameters__unpack(NULL, out_l, out), true);

	/** Load solutions */
	int nSol;
	if (readIntFromFile(scn, "NSolutions", nSol, false)) {
		return;
	}

	char key[50];
	unsigned char buf[16000];
	for (int i = 0; i < nSol; ++i) {
		sprintf_s(key, "Solution%02d", i);
		unsigned int l;
		if (readBase64FromFile(scn, key, buf, l)) {
			return;
		}

		AddSolution(orbiterkep__trans_xsolution__unpack(NULL, l, buf));
	}

	double x;
	double y;
	int n = 0;
	if (2 == fscanf_s(scn, "Pareto %lf;%lf", &x, &y)) {
		m_pareto[0][0] = x; m_pareto[0][1] = y;
		n += 1;
		while (2 == fscanf_s(scn, ";%lf;%lf", &x, &y)) {
			m_pareto[n][0] = x; m_pareto[n][1] = y;
			n += 1;
		}
		m_n_pareto = n;
	}

	Signal();
}

void Optimization::SaveStateTo(FILE * scn) {
	int size = orbiterkep__parameters__get_packed_size(m_param);
	uint8_t * buf[1024];
	orbiterkep__parameters__pack(m_param, (unsigned char *)buf);

	writeBase64ToFile(scn, "Parameters", (unsigned char *)buf, size);

	sprintf_s((char *)buf, 1023, "NSolutions %ld\n", n_solutions);
	fputs((char *)buf, scn);
	for (int i = 0; i < n_solutions; ++i) {
		char key[50];
		sprintf_s(key, "Solution%02d", i);
		size = orbiterkep__trans_xsolution__get_packed_size(m_solutions[i]);
		memset(buf, 0, size);
		size = orbiterkep__trans_xsolution__pack(m_solutions[i], (unsigned char *)buf);

		writeBase64ToFile(scn, key, (unsigned char *)buf, size);
	}
	if (m_n_pareto > 0) {
		fputs("Pareto ", scn);
		char buf[200];
		for (int i = 0; i < m_n_pareto - 1; ++i) {
			sprintf_s(buf, 1023, "%lf;%lf;", m_pareto[i][0], m_pareto[i][1]);
			fputs(buf, scn);
		}
		sprintf_s(buf, 1023, "%lf;%lf\n", m_pareto[m_n_pareto - 1][0], m_pareto[m_n_pareto - 1][1]);
		fputs(buf, scn);
	}
}

std::vector<std::string> Optimization::SavedPlans() {
	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;


	std::vector<std::string> result;
	char sPath[MAX_PATH];
	sprintf_s(sPath, "%s\\*.mga", "Config/MFD/GravityAssistMFD/MGAPlans");

	if ((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE) {
		return result;
	}
	do {
		if (strcmp(fdFile.cFileName, ".") == 0
			|| strcmp(fdFile.cFileName, "..") == 0) continue;

		std::string s(fdFile.cFileName, strlen(fdFile.cFileName) - 4);
		result.push_back(s);
	} while (FindNextFile(hFind, &fdFile));

	return result;
}

int Optimization::SaveCurrentPlan() {
	int hash = param_hash(*m_param);

	char cacheFile[40];
	sprintf_s(cacheFile, "cache/%ld", (unsigned int)hash);

	SavePlan(cacheFile);
	return hash;
}


void Optimization::LoadPlan(char * file) {
	char filename[MAX_PATH];
	sprintf_s(filename, "Config/MFD/GravityAssistMFD/MGAPlans/%s.mga", file);
	FILE * fileHandle; fopen_s(&fileHandle, filename, "r");
	if (fileHandle == 0) return;
	LoadStateFrom(fileHandle);

	fclose(fileHandle);
}


void Optimization::SavePlan(char * file) {
	char filename[MAX_PATH];
	sprintf_s(filename, "Config/MFD/GravityAssistMFD/MGAPlans/%s.mga", file);
	FILE * fileHandle; fopen_s(&fileHandle, filename, "w");

	SaveStateTo(fileHandle);

	fclose(fileHandle);
}

void Optimization::AddSolution(Orbiterkep__TransXSolution * newSolution) {
	if (n_solutions < 25) {
		m_solutions[n_solutions] = newSolution;
		n_solutions += 1;
	} else {

		int min_idx = -1;
		double min = DBL_MAX;

		int max_idx = -1;
		double max = 0;
		for (int i = 0; i < n_solutions; ++i) {
			double fuelCost = m_solutions[i]->fuel_cost;
			if (fuelCost < min) {
				min_idx = i;
				min = fuelCost;
			}
			if (fuelCost > max) {
				max_idx = i;
				max = fuelCost;
			}
		}

		if (newSolution->fuel_cost > max) return;

		if (m_best_solution == m_solutions[max_idx]) {
			m_best_solution = 0;
		}
		orbiterkep__trans_xsolution__free_unpacked(m_solutions[max_idx], NULL);
		m_solutions[max_idx] = newSolution;
	}
	
	if (m_best_solution == 0 || newSolution->fuel_cost < m_best_solution->fuel_cost) {
		m_best_solution = newSolution;
	}	
}


void Optimization::Update(HWND _hDlg) {
	char m_solution_buf[16000];
	if (m_best_solution != 0) {
		int len_sol = sprintf_transx_solution(m_solution_buf, m_best_solution);
		m_solution_str = std::string(m_solution_buf, len_sol);

		m_messenger.PutSolution(*m_best_solution);
	}
	if (_hDlg != NULL) {
		hDlg = _hDlg;
	}
	if (hDlg != 0) {
		PostMessage(hDlg, WM_OPTIMIZATION_READY, 0, 0);
	}
}
