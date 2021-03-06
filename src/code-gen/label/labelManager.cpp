#include "labelManager.h"
#include <sstream>
#include <vector>
#include <stdarg.h>
#include <string.h>

std::string LabelManager::labelizer = "$";
std::string LabelManager::virt = "VIRT" + LabelManager::labelizer;
std::string LabelManager::inheritance = "INH" + LabelManager::labelizer;
std::string LabelManager::implInterface = "INTER" + LabelManager::labelizer;
std::string LabelManager::initializer = "INIT" + LabelManager::labelizer;
std::string LabelManager::array = ".array";
std::string LabelManager::constructor = "CTOR" + LabelManager::labelizer;
std::string LabelManager::alloc = "ALLOC" + LabelManager::labelizer;
std::string LabelManager::native = "NATIVE";

std::string LabelManager::getLabelizer() {
	return LabelManager::labelizer;
}

std::string LabelManager::getConstructor() {
	return LabelManager::constructor;
}

std::string LabelManager::labelizeForAlloc(const std::string& toLabelize) {
	return alloc + toLabelize;
}

std::string LabelManager::labelizeForConstructor(const std::string& toLabelize) {
	return constructor + toLabelize;
}

std::string LabelManager::labelizeForConstructor(const std::string& toLabelize, int n_args, ...) {
	std::stringstream ss;
	std::vector<char*> tokens;
	char* temp;

	ss << LabelManager::labelizeForConstructor(toLabelize);

	va_list ap;
    va_start(ap, n_args);
    for (int i = 0; i < n_args; i++) {
    	ss << "$";

    	temp = va_arg(ap, char*);
    	char *token = strtok(temp, ".");
	    while (token != NULL) {
	        tokens.push_back(token);
	        token = strtok(NULL, " ");
	    }
    	if (tokens.size() > 1 && std::string(tokens[1]) == "array") {
    		ss << LabelManager::labelizeForArrays(tokens[0]);
    	} else {
    		ss << tokens[0];
    	}
    }
    va_end(ap);

    ss << "$";
	return ss.str();
}

std::string LabelManager::labelizeForArrays(const std::string& toLabelize) {
    return toLabelize + array;
}

std::string LabelManager::labelizeToInheritanceTable(const std::string& toLabelize) {
    return LabelManager::inheritance + toLabelize;
}

std::string LabelManager::labelizeToVirtualTable(const std::string& toLabelize) {
    return LabelManager::virt + toLabelize;
}

std::string LabelManager::labelizeToInterfaceMethodTable(const std::string& toLabelize) {
    return LabelManager::implInterface + toLabelize;
}

std::string LabelManager::labelizeToFieldInitializer(const std::string& toLabelize) {
    return LabelManager::initializer + toLabelize;
}

std::string LabelManager::getLabelForArrayImplInterfaceMethodTable() {
    return LabelManager::implInterface + array;
}

std::string LabelManager::getLabelForArrayVirtualTable() {
    return LabelManager::virt + LabelManager::array;
}

std::string LabelManager::getLabelForArrayInheritanceTable(const std::string& arrayComponentType) {
    return LabelManager::labelizeForArrays(LabelManager::labelizeToInheritanceTable(arrayComponentType));
}

std::string LabelManager::labelizeToNativeMethod(const std::string& toLabelize) {
    return LabelManager::native + toLabelize;
}
