#include "Optimisation/Optimizer.hpp"

#include <Orbitersdk.h>
#include <orbiterkep/opt/optimise-c.h>
#include <orbiterkep/proto/ext-c.h>

#include "ModuleMessaging\MGAModuleMessenger.h"
#include "Dialog/MGAFinder.hpp"

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
	if (m_param != 0) {
		orbiterkep__parameters__free_unpacked(m_param, NULL); m_param = 0;
	}

	for (int i = 0; i < n_solutions; ++i) {
		orbiterkep__trans_xsolution__free_unpacked(m_solutions[i], NULL);
	}

	free(m_solutions);
}

Optimization::Optimization(const MGAModuleMessenger &messenger) : m_messenger(messenger)
{
	m_computing = false;
	n_solutions = 0;
	m_best_solution = 0;
	m_solutions = (Orbiterkep__TransXSolution **)malloc(25 * sizeof(Orbiterkep__TransXSolution *));

	m_param = (Orbiterkep__Parameters *)malloc(sizeof(Orbiterkep__Parameters));
	orbiterkep__parameters__init(m_param);
	char ** planets = allocate_string_array(10, 50);
	m_param->n_planets = 3;	
	strcpy_s(planets[0], 50, "earth");
	strcpy_s(planets[1], 50, "venus");
	strcpy_s(planets[2], 50, "mercury");
	m_param->planets = planets;
	char ** single_obj_algos = allocate_string_array(10, 50);
	strcpy_s(single_obj_algos[0], 50, "jde");
	m_param->n_single_objective_algos = 1;
	m_param->single_objective_algos = single_obj_algos;
	char ** multi_obj_algos = allocate_string_array(10, 50);
	strcpy_s(multi_obj_algos[0], 50, "nsga2");
	m_param->n_multi_objective_algos = 1;
	m_param->multi_objective_algos = multi_obj_algos;

	m_param->problem = (char *)malloc(50 * sizeof(char));
	strcpy_s(m_param->problem, 50, "MGA");

	m_param->has_n_trials = 1;
	m_param->n_trials = 1;

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

	m_param->has_max_deltav = 1;
	m_param->max_deltav = 24.0;

	m_param->has_circularize = 1;
	m_param->circularize = 1;

	m_param->has_add_arr_vinf = 1;
	m_param->add_arr_vinf = 1;

	m_param->has_add_dep_vinf = 1;
	m_param->add_dep_vinf = 1;

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
}

std::string Optimization::get_solution_str_current_stage() const
{
	if (!has_solution()) return "";

	char result_buf[10000];
	int len = 0;

	double currentTime = oapiGetSimMJD();
	if (currentTime <= m_best_solution->escape->mjd + 1) { // if we are less than 1 day after planned launch, current stage is ESCAPE
		len = sprintf_transx_escape(result_buf, m_best_solution->escape);
	} else {
		int i = 0; int j = 0;
		int n_dsm = m_best_solution->n_dsms; int n_flyby = m_best_solution->n_flybyes;
		bool found = false;
		while (i < n_dsm || j < n_flyby) {
			if (i < n_dsm) {
				if (currentTime < m_best_solution->dsms[i]->mjd + 1) {
					len = sprintf_transx_dsm(result_buf, m_best_solution->dsms[i]);
					found = true;
					break;
				}
				++i;
			}
			if (j < n_flyby) {
				if (currentTime < m_best_solution->flybyes[j]->mjd + 1) {
					len = sprintf_transx_flyby(result_buf, m_best_solution->flybyes[j]);
					found = true;
					break;
				}
				++j;
			}
		}

		if (!found) {
			len = sprintf_transx_arrival(result_buf, m_best_solution->arrival);
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

void Optimization::RunOptimization(HWND hDlg)
{
	m_computing = true;

	ResetSolutions();

	m_cancel = false;
	while (n_solutions < 25 && !m_cancel) {
		int len = orbiterkep__parameters__get_packed_size(m_param);
		void * buf = malloc(len);
		orbiterkep__parameters__pack(m_param, (uint8_t *)buf);

		void * sol_buf = malloc(2000);
		int sol_len = orbiterkep_optimize((const uint8_t *)buf, len, (uint8_t *)sol_buf);

		Orbiterkep__TransXSolution * solution = orbiterkep__trans_xsolution__unpack(NULL, sol_len, (uint8_t *)sol_buf);
		AddSolution(solution);

		Update(hDlg);
	}

	Update(hDlg);
	m_computing = false;
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

int readBase64FromScenario(FILEHANDLE scn, char * key, unsigned char * outputBuf) {
	char * buf;
	if (!oapiReadScenario_nextline(scn, buf)) {
		return 0;
	}
	if (strnicmp(buf, key, strlen(key))) {
		return 0;
	}
	char * str = buf + strlen(key) + 1;

	int len = strlen((char *)str);
	unsigned int out_l;
	base64_decode((unsigned char *)str, len, outputBuf, out_l);

	return out_l;
}

int readIntFromScenario(FILEHANDLE scn, char * key, int & result) {
	char * buf;
	if (!oapiReadScenario_nextline(scn, buf)) {
		return -1;
	}
	if (!strnicmp(buf, key, strlen(key))) {
		return -1;
	}
	sscanf_s(buf + strlen(key), "%d", &result);

	return 0;
}

int writeBase64ToScenario(FILEHANDLE scn, char * key, const unsigned char * buf, int size)
{
	unsigned char base64[1024];
	unsigned int out_l;
	base64_encode(buf, size, base64, out_l);
	base64[out_l] = 0;
	
	oapiWriteScenario_string(scn, key, (char *)base64);
}

void Optimization::LoadStateFrom(FILEHANDLE scn)
{
	/** Load parameters */	
	unsigned char out[2048];
	int out_l = readBase64FromScenario(scn, "Parameters", out);

	if (m_param) {
		orbiterkep__parameters__free_unpacked(m_param, NULL);
		m_param = 0;
	}
	m_param = orbiterkep__parameters__unpack(NULL, out_l, out);

	/** Load solutions */
	int nSol;
	if (readIntFromScenario(scn, "NSolutions", nSol)) {
		return;
	}

	ResetSolutions();

	for (int i = 0; i < nSol; ++i) {
		char key[50];
		sprintf(key, "Solution%02d", i);
		unsigned char buf[1024];
		int l = readBase64FromScenario(scn, key, buf);

		m_solutions[i] = orbiterkep__trans_xsolution__unpack(NULL, l, buf);
	}
	n_solutions = nSol;
}

void Optimization::SaveStateTo(FILEHANDLE scn) {
	int size = orbiterkep__parameters__get_packed_size(m_param);
	uint8_t * buf[1024];
	orbiterkep__parameters__pack(m_param, (unsigned char *)buf);

	writeBase64ToScenario(scn, "Parameters", (unsigned char *)buf, size);

	oapiWriteScenario_int(scn, "NSolutions", n_solutions);
	for (int i = 0; i < n_solutions; ++i) {
		char key[50];
		sprintf(key, "Solution%02d", i);
		size = orbiterkep__trans_xsolution__get_packed_size(m_solutions[i]);
		orbiterkep__trans_xsolution__pack(m_solutions[i], (unsigned char *)buf);

		writeBase64ToScenario(scn, key, (unsigned char *)buf, size);
	}
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

		orbiterkep__trans_xsolution__free_unpacked(m_solutions[max_idx], NULL);
		m_solutions[max_idx] = newSolution;
	}
	
	if (m_best_solution == 0 || newSolution->fuel_cost < m_best_solution->fuel_cost) {
		m_best_solution = newSolution;
	}	
}


void Optimization::Update(HWND hDlg) {
	char m_solution_buf[16000];
	int len_sol = sprintf_transx_solution(m_solution_buf, m_best_solution);
	m_solution_str = std::string(m_solution_buf, len_sol);

	m_messenger.PutSolution(*m_best_solution);

	if (hDlg != 0) {
		PostMessage(hDlg, WM_OPTIMIZATION_READY, 0, 0);
	}
}
