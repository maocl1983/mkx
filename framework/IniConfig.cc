#include <stdio.h>
#include "IniConfig.h"

using namespace std;

IniConfig::IniConfig()
{

}

IniConfig::~IniConfig()
{
	fields_.clear();
}

int IniConfig::Parse(const string& filename)
{
	FILE* file = fopen(filename.c_str(), "r");
	if (file == nullptr) {
		return -1;
	}

	fields_.clear();
	static const int BUFF_LEN = 2048;
	char buff[BUFF_LEN] = {0};
	while (fgets(buff, BUFF_LEN, file) != NULL) {
		string line(buff);
		string::size_type pos = line.find(';');
		if (pos != string::npos) {
			line.erase(pos);
		}
		strTrim(line);

		if (line.empty()) {
			continue;
		}

		pos = line.find('=');
		if (pos == string::npos) {
			continue;
		}

		string key = line.substr(0, pos);
		string value = line.substr(pos + 1);
		strTrim(key);
		strTrim(value);
		fields_[key] = value;
	}

	fclose(file);
	return 0;
}

string IniConfig::GetStr(const string& name) const
{
	const string* pstr = getVal(name);
	if (pstr) {
		return *pstr;
	}
	return string("");
}

int IniConfig::GetInt(const string& name, int defaultVal) const
{
	const string* pstr = getVal(name);
	if (pstr) {
		return atoi(pstr->c_str());
	}
	return defaultVal;
}

void IniConfig::strTrim(string& str)
{
	str.erase(0, str.find_first_not_of(' '));
	str.erase(str.find_last_not_of('\n') + 1);
	str.erase(str.find_last_not_of(' ') + 1);
}
	
const string* IniConfig::getVal(const string& key) const
{
	map<string, string>::const_iterator it = fields_.find(key);
	if (it == fields_.end()) {
		return nullptr;
	}

	return &it->second;
}

