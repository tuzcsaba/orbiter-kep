#include <Optimisation\SolutionDiskStore.hpp>


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
	}
	else {
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

MGAPlan * SolutionDiskStore::LoadPlan(char * file) {
	char filename[MAX_PATH];
	sprintf_s(filename, "Config/MFD/GravityAssistMFD/MGAPlans/%s.mga", file);
	FILE * fileHandle; fopen_s(&fileHandle, filename, "r");
	if (fileHandle == 0) return 0;
	MGAPlan * plan = LoadStateFrom(fileHandle);
	fclose(fileHandle);

	return plan;
}


void SolutionDiskStore::SavePlan(char * file, MGAPlan * plan) {
	char filename[MAX_PATH];
	sprintf_s(filename, "Config/MFD/GravityAssistMFD/MGAPlans/%s.mga", file);
	FILE * fileHandle; fopen_s(&fileHandle, filename, "w");

	SaveStateTo(fileHandle, plan);

	fclose(fileHandle);
}

int SolutionDiskStore::SavePlan(MGAPlan * plan) {
	int hash = param_hash(plan->param());

	char cacheFile[40];
	sprintf_s(cacheFile, "cache/%ld", (unsigned int)hash);

	SavePlan(cacheFile, plan);

	return hash;
}

MGAPlan * SolutionDiskStore::LoadScenario(FILEHANDLE scn) {
	int hash;
	if (readIntFromFile(scn, "Hash", hash, true)) {
		return 0;
	}

	char cacheFile[40];
	sprintf_s(cacheFile, "cache/%ld", (unsigned int)hash);

	return LoadPlan(cacheFile);
}

void SolutionDiskStore::SaveScenario(FILEHANDLE scn, MGAPlan * plan) {

	int hash = SavePlan(plan);

	oapiWriteScenario_int(scn, "Hash", hash);
}

MGAPlan * SolutionDiskStore::LoadStateFrom(FILE * scn)
{
	/** Load parameters */
	unsigned char out[2048];
	unsigned int out_l;
	if (readBase64FromFile(scn, "Parameters", out, out_l)) {
		return 0;
	}
	Orbiterkep__Parameters * param = orbiterkep__parameters__unpack(NULL, out_l, out);
	MGAPlan * plan = new MGAPlan(param);

	/** Load solutions */
	int nSol;
	if (readIntFromFile(scn, "NSolutions", nSol, false)) {
		return 0;
	}

	char key[50];
	unsigned char buf[16000];
	for (int i = 0; i < nSol; ++i) {
		sprintf_s(key, "Solution%02d", i);
		unsigned int l;
		if (readBase64FromFile(scn, key, buf, l)) {
			return 0;
		}

		plan->AddSolution(orbiterkep__trans_xsolution__unpack(NULL, l, buf));
	}

	double x;
	double y;
	int n = 0;
	if (2 == fscanf_s(scn, "Pareto %lf;%lf", &x, &y)) {
		plan->AddPareto(x, y);
		while (2 == fscanf_s(scn, ";%lf;%lf", &x, &y)) {
			plan->AddPareto(x, y);
		}
	}

	return plan;
}

void SolutionDiskStore::SaveStateTo(FILE * scn, MGAPlan * plan) {
	auto param = plan->param();
	int size = orbiterkep__parameters__get_packed_size(&param);
	uint8_t * buf[16000];
	orbiterkep__parameters__pack(&param, (unsigned char *)buf);

	writeBase64ToFile(scn, "Parameters", (unsigned char *)buf, size);

	int n_solutions = plan->get_n_solutions();
	sprintf_s((char *)buf, 15999, "NSolutions %ld\n", n_solutions);
	fputs((char *)buf, scn);
	for (int i = 0; i < n_solutions; ++i) {
		char key[50];
		sprintf_s(key, "Solution%02d", i);
		auto sol = plan->get_solution(i);
		size = orbiterkep__trans_xsolution__get_packed_size(&sol);
		memset(buf, 0, size);
		size = orbiterkep__trans_xsolution__pack(&sol, (unsigned char *)buf);

		writeBase64ToFile(scn, key, (unsigned char *)buf, size);
	}
	int n = *plan->get_n_pareto();
	if (n > 0) {
		fputs("Pareto ", scn);
		double x, y;
		int i = 0;
		for (i = 0; i < n - 1; ++i) {
			plan->get_pareto(i, x, y);
			sprintf_s((char *)buf, 15999, "%lf;%lf;", x, y);
			fputs((char *)buf, scn);
		}
		plan->get_pareto(i, x, y);
		sprintf_s((char *)buf, 15999, "%lf;%lf\n", x, y);
		fputs((char *)buf, scn);
	}
}