#ifndef DELETEGCCMAINCALL_HPP
#define DELETEGCCMAINCALL_HPP

#include <fstream>

static std::string Utils_DeleteGCCMainCall(std::string assemblyFile) {

	std::ifstream file(assemblyFile.c_str());
    std::string str;

    std::string res;
    while (std::getline(file, str))
    {
    	if(str.find("__main") == std::string::npos) {

    		res += str;
    		res += "\n";
    	}

    	str.clear();
    }

    file.close();

    return res;
}

#endif