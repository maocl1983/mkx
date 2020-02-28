#pragma once
#include <map>
#include <string>

class IniConfig {
public:
	IniConfig();
	~IniConfig();

	int Parse(const std::string& filename);
	std::string GetStr(const std::string& name) const;
	int GetInt(const std::string& name, int defaultVal = 0);

private:
	void strTrim(std::string& str);
	const std::string* getVal(const std::string& key) const;

private:
	std::map<std::string, std::string>	fields_;
};
