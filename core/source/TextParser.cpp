
#include <iostream>
#include <sstream>
#include <algorithm>

#include "TextParser.hpp"
#include "Support.hpp"

#define debugStr std::cout << " TextParser: [debug] "
#define errorStr std::cout << " TextParser: [error] "
#define fatalErrorStr std::cout << " TextParser: [fatal error!] "

NumericalString::NumericalString(const unsigned int& val) :
	str(uintToStr(val)),
	result(val),
	op(0x0),
	type(NUMTYPE::INTEGER),
	computed(false)
{
}

NumericalString::NumericalString(const std::string& s, OperatorType* op_/*=0x0*/) :
	str(s),
	result(getUserInputUInt(s)),
	op(op_),
	type(NUMTYPE::INTEGER),
	computed(false)
{
}

std::string NumericalString::getStr() const { 
	return (computed ? uintToStr(result) : str); 
}

void NumericalString::setResult(const NumericalString& res, const NUMTYPE& newType/* = NUMTYPE::NONE*/) {
	result = res.getUInt();
	computed = true;
	if (newType != NUMTYPE::NONE)
		type = newType;
}

NumericalString ExternalVariable::get() {
	NumericalString retval;
	switch (dtype) {
	case CPPTYPE::BOOL:
		retval.result = *(static_cast<bool*>(ptr));
		retval.type = NUMTYPE::BOOLEAN;
		break;
	case CPPTYPE::UINT8:
		retval.result = *(static_cast<unsigned char*>(ptr));
		break;
	case CPPTYPE::UINT16:
		retval.result = *(static_cast<unsigned short*>(ptr));
		break;
	case CPPTYPE::UINT32:
		retval.result = *(static_cast<unsigned int*>(ptr));
		break;
	case CPPTYPE::INT8:
		retval.result = *(static_cast<char*>(ptr));
		break;
	case CPPTYPE::INT16:
		retval.result = *(static_cast<short*>(ptr));
		break;
	case CPPTYPE::INT32:
		retval.result = *(static_cast<int*>(ptr));
		break;
	case CPPTYPE::FLOAT:
		retval.result = *(static_cast<float*>(ptr));
		//retval.type = NUMTYPE::FLOAT;
		break;
	default:
		break;
	}
	return retval;
}

NumericalString ExternalVariable::set(const NumericalString& rhs) {
	switch (dtype) {
	case CPPTYPE::BOOL:
		*(static_cast<bool*>(ptr)) = static_cast<bool>(rhs.result);
		break;
	case CPPTYPE::UINT8:
		*(static_cast<unsigned char*>(ptr)) = static_cast<unsigned char>(rhs.result);
		break;
	case CPPTYPE::UINT16:
		*(static_cast<unsigned short*>(ptr)) = static_cast<unsigned short>(rhs.result);
		break;
	case CPPTYPE::UINT32:
		*(static_cast<unsigned int*>(ptr)) = static_cast<unsigned int>(rhs.result);
		break;
	case CPPTYPE::INT8:
		*(static_cast<char*>(ptr)) = static_cast<char>(rhs.result);
		break;
	case CPPTYPE::INT16:
		*(static_cast<short*>(ptr)) = static_cast<short>(rhs.result);
		break;
	case CPPTYPE::INT32:
		*(static_cast<int*>(ptr)) = static_cast<int>(rhs.result);
		break;
	case CPPTYPE::FLOAT:
		*(static_cast<float*>(ptr)) = static_cast<float>(rhs.result);
		break;
	default:
		break;
	}
	return (*this);
}

TextParser::TextParser() :
	debugMode(false),
	fatalError(false),
	maxOperatorLength(0),
	lastResult()
{
	// Add operators to the list, following order of precedence
	// Arithmetic operators
	addArithmeticOperator("*", OPERATOR::MULTIPLY);
	addArithmeticOperator("/", OPERATOR::DIVIDE);
	addArithmeticOperator("%", OPERATOR::MODULUS);
	addArithmeticOperator("+", OPERATOR::ADD);
	addArithmeticOperator("-", OPERATOR::SUBTRACT);
	addArithmeticOperator("<<", OPERATOR::BITSHIFT_LEFT);
	addArithmeticOperator(">>", OPERATOR::BITSHIFT_RIGHT);
	addArithmeticOperator("&", OPERATOR::BITWISE_AND);
	addArithmeticOperator("^", OPERATOR::BITWISE_XOR);
	addArithmeticOperator("|", OPERATOR::BITWISE_OR);

	// Logical operators
	addLogicalOperator("<", OPERATOR::LESS_THAN);
	addLogicalOperator(">", OPERATOR::GREATER_THAN);
	addLogicalOperator("<=", OPERATOR::LESS_THAN_EQUAL);
	addLogicalOperator(">=", OPERATOR::GREATER_THAN_EQUAL);
	addLogicalOperator("==", OPERATOR::EQUALITY);
	addLogicalOperator("!=", OPERATOR::NOT_EQUAL);
	addLogicalOperator("&&", OPERATOR::LOGICAL_AND, NUMTYPE::BOOLEAN);
	addLogicalOperator("||", OPERATOR::LOGICAL_OR, NUMTYPE::BOOLEAN);

	// Assignment operators
	addAssignmentOperator("=", OPERATOR::ASSIGNMENT);
	addAssignmentOperator("*=", OPERATOR::MULTIPLY);
	addAssignmentOperator("/=", OPERATOR::DIVIDE);
	addAssignmentOperator("%=", OPERATOR::MODULUS);
	addAssignmentOperator("+=", OPERATOR::ADD);
	addAssignmentOperator("-=", OPERATOR::SUBTRACT);
	addAssignmentOperator("<<=", OPERATOR::BITSHIFT_LEFT);
	addAssignmentOperator(">>=", OPERATOR::BITSHIFT_RIGHT);
	addAssignmentOperator("&=", OPERATOR::BITWISE_AND);
	addAssignmentOperator("^=", OPERATOR::BITWISE_XOR);
	addAssignmentOperator("|=", OPERATOR::BITWISE_OR);

	// Number type name map for nicer error message output
	numTypes[NUMTYPE::NONE] = "none";
	numTypes[NUMTYPE::BOOLEAN] = "bool";
	numTypes[NUMTYPE::INTEGER] = "int";
}

TextParser::TextParser(const std::string& str) :
	TextParser()
{
	parse(str, lastResult);
}

unsigned char compareOperatorStrings(const std::string& str1, const std::string& str2) {
	unsigned char retval = 0;
	bool mismatch = false;
	for (size_t i = 0; i < str2.length(); i++) {
		if (str1[i] != str2[i]) {
			mismatch = true;
			break;
		}
	}
	if (!mismatch) {
		bitSet(retval, (unsigned char)(str2.length()-1));
	}
	return retval;
}

bool TextParser::findOperatorHere(const std::string& input, const size_t& loc, OperatorType*& op) {
	std::string dummy = input.substr(loc, maxOperatorLength);
	unsigned char matchStrength;
	std::vector<std::pair<unsigned char, OperatorType*> > matches;
	for (auto iter = operators.begin(); iter != operators.end(); iter++) {
		matchStrength = compareOperatorStrings(dummy, iter->sName);
		if (matchStrength) {
			matches.push_back(std::pair<unsigned char, OperatorType*>(matchStrength, iter->getPtr()));
		}
	}
	unsigned char maxStrength = 0;
	for (auto match = matches.begin(); match != matches.end(); match++) {
		if (match->first > maxStrength) {
			op = match->second;
			maxStrength = match->first;
		}
	}
	if(debugMode && maxStrength > 1)
		debugStr << " found long operator at index=" << loc << " in input string, strength=" << (int)maxStrength << std::endl;
	return (maxStrength > 0);
}

void TextParser::splitByOperators(const std::string& str, std::vector<NumericalString>& parts) {
	std::string current = "";
	OperatorType* op = 0x0;
	for (size_t i = 0; i < str.length(); i++) {
		if (findOperatorHere(str, i, op)) {
			parts.push_back(NumericalString(current));
			parts.push_back(NumericalString(op->sName, op)); // operator
			current = "";
			if (op->sName.length() > 1) {
				i += op->sName.length() - 1;
			}
		}
		else
			current += str[i];
	}
	parts.push_back(NumericalString(current));
}

bool TextParser::getInput(const NumericalString& name, NumericalString& val){
	if (name.computed) { // Value was computed in a previous operation
		val = name;
	}
	else if (!isNotNumeric(name.str)) {
		val = name.result;
	}
	else {
		auto def = defines.find(name.str);
		if (def != defines.end()) {
			val = def->second;
		}
		else if ((def = tempDefines.find(name.str)) != tempDefines.end()) {
			val = def->second;
		}
		else {
			auto ext = externalDefines.find(name.str);
			if (ext != externalDefines.end()) {
				val = ext->second.get();
			}
			else {
				errorStr << " variable (" << name.str << ") is not defined!\n";
				fatalError = true;
				return false;
			}
		}
	}
	return true;
}

bool TextParser::compute(NumericalString& lhs, const NumericalString& rhs, const NumericalString& op) {
	if (!lhs.checkTypes(op, rhs)) {
		errorStr << " operand type mismatch for operator " << op.str << ", \'" << numTypes[lhs.type] << "\' and \'" << numTypes[rhs.type] << "\'" << std::endl;
		fatalError = true;
		return false;
	}
	NumericalString* ptr = 0x0;
	NumericalString valL = 0;
	NumericalString valR = 0;
	if (op.isAssignmentOperator()) {
		if (lhs.computed || !isNotNumeric(lhs.str)) {
			errorStr << " cannot assign to numerical value (" << lhs.str << ")!\n";
			fatalError = true;
			return false;
		}
		else {
			auto def = defines.find(lhs.str);
			if (def != defines.end()) {
				valL = def->second;
				ptr = &def->second;
			}
			else {
				auto ext = externalDefines.find(lhs.str);
				if (ext != externalDefines.end()) {
					valL = ext->second.get();
					ptr = &ext->second;
				}
				else if (op.getOperator() == OPERATOR::ASSIGNMENT) { // New definition
					addDefinition(lhs.str, 0);
					def = defines.find(lhs.str);
					valL = def->second;
					ptr = &def->second;
				}
				else {
					errorStr << " variable (" << lhs.str << ") is not defined!\n";
					fatalError = true;
					return false;
				}
			}
		}
	}
	else if (!getInput(lhs, valL)) { // Get left hand value
		errorStr << " variable (" << lhs.str << ") is not defined!\n";
		fatalError = true;
		return false;
	}
	if (!getInput(rhs, valR)) { // Get right hand value
		errorStr << " variable (" << rhs.str << ") is not defined!\n";
		fatalError = true;
		return false;
	}
	NumericalString result = 0;
	switch (op.getOperator()) {
	case OPERATOR::NONE: // No operation
		break;
	case OPERATOR::MULTIPLY:
		result = valL * valR;
		break;
	case OPERATOR::DIVIDE:
		result = valL / valR;
		break;
	case OPERATOR::MODULUS:
		result = valL % valR;
		break;
	case OPERATOR::ADD:
		result = valL + valR;
		break;
	case OPERATOR::SUBTRACT:
		result = valL - valR;
		break;
	case OPERATOR::BITSHIFT_LEFT:
		result = valL << valR;
		break;
	case OPERATOR::BITSHIFT_RIGHT:
		result = valL >> valR;
		break;
	case OPERATOR::LESS_THAN:
		result = (valL < valR);
		break;
	case OPERATOR::GREATER_THAN:
		result = (valL > valR);
		break;
	case OPERATOR::LESS_THAN_EQUAL:
		result = (valL <= valR);
		break;
	case OPERATOR::GREATER_THAN_EQUAL:
		result = (valL >= valR);
		break;
	case OPERATOR::EQUALITY:
		result = (valL == valR);
		break;
	case OPERATOR::NOT_EQUAL:
		result = (valL != valR);
		break;
	case OPERATOR::BITWISE_AND:
		result = valL & valR;
		break;
	case OPERATOR::BITWISE_XOR:
		result = valL ^ valR;
		break;
	case OPERATOR::BITWISE_OR:
		result = valL | valR;
		break;
	case OPERATOR::LOGICAL_AND:
		result = ((valL != 0) && (valR != 0));
		break;
	case OPERATOR::LOGICAL_OR:
		result = ((valL != 0) || (valR != 0));
		break;
	case OPERATOR::ASSIGNMENT:
		result = valR;
		break;
	default:
		break;
	}
	if (op.isAssignmentOperator()) { // Assignment operators
		ptr->set(result);
	}
	if (debugMode)
		debugStr << "  >compute(): l=" << valL.getUInt() << ", r=" << valR.getUInt() << ", op=" << op.str << ", result=" << result.getUInt() << std::endl;
	lhs.setResult(result, op.getResultType()); // store the result into the left operand
	return true;
}

bool TextParser::eval(const unsigned int& index, const std::string& str) {
	if (debugMode)
		debugStr << " [in] @[" << index << "]=\"" << str << "\"" << std::endl;
	std::vector<NumericalString> parts;
	std::vector<NumericalString>::iterator iter;
	splitByOperators(str, parts);
	if(parts.size() % 2 == 0){
		fatalErrorStr << " TextParser: invalid number of arguments passed to eval()" << std::endl;
		fatalError = true;
		return false;
	}
	for (auto op = operators.begin(); op != operators.end(); op++) { // Order of operations
		if (fatalError) {
			fatalErrorStr << " TextParser: encountered fatal error while parsing string" << std::endl;
			fatalError = true;
			return false;
		}
		while ((iter = std::find(parts.begin(), parts.end(), op->sName)) != parts.end()) {
			if (debugMode) {
				std::string strToEvaluate = (iter - 1)->getStr() + iter->getStr() + (iter + 1)->getStr();
				debugStr << "  eval(): compute(\"" << strToEvaluate << "\")" << std::endl;
			}
			if (!compute(*(iter - 1), *(iter + 1), *iter)) // compute the result and store it into (iter-1)
				break;
			parts.erase(iter, iter + 2); // erase iter and iter+1
		}
	}
	lastResult = parts.front();
	if (debugMode)
		debugStr << " [out] @[" << index << "]=" << lastResult.result << "" << std::endl;
	addTempDefinition(index, lastResult);
	return true;
}

void TextParser::addTempDefinition(const unsigned int& index, const NumericalString& value) {
	std::stringstream repstr;
	repstr << "@[" << index << "]";
	tempDefines[repstr.str()] = value;
}

void TextParser::addArithmeticOperator(const std::string& name, const OPERATOR& op, const NUMTYPE& type) {
	operators.push_back(OperatorType(name, op, OPERATION::ARITHMETIC));
	operators.back().operands = type;
	if (name.length() > maxOperatorLength)
		maxOperatorLength = (unsigned int)name.length();
}

void TextParser::addLogicalOperator(const std::string& name, const OPERATOR& op, const NUMTYPE& type) {
	operators.push_back(OperatorType(name, op, OPERATION::LOGICAL));
	operators.back().operands = type;
	operators.back().result = NUMTYPE::BOOLEAN;
	if (name.length() > maxOperatorLength)
		maxOperatorLength = (unsigned int)name.length();
}

void TextParser::addAssignmentOperator(const std::string& name, const OPERATOR& op, const NUMTYPE& type) {
	operators.push_back(OperatorType(name, op, OPERATION::ASSIGNMENT));
	operators.back().operands = type;
	if (name.length() > maxOperatorLength)
		maxOperatorLength = (unsigned int)name.length();
}

bool TextParser::isExpression(const std::string& str) {
	OperatorType* op = 0x0;
	for (size_t i = 0; i < str.length(); i++) {
		if (findOperatorHere(str, i, op)) {
			return true;
		}
	}
	return false;
}

bool TextParser::parse(const std::string& str, NumericalString& result) {
	tempDefines.clear();
	fatalError = false;
	unsigned int c1 = countOccurances(str, '(');
	unsigned int c2 = countOccurances(str, ')');
	if (c1 < c2) {
		errorStr << " extraneous ')' encountered" << std::endl;
		fatalError = true;
		return false;
	}
	if (c1 > c2) {
		errorStr << " expected ')' at end of input" << std::endl;
		fatalError = true;
		return false;
	}
	std::string retval = stripAllWhitespace(str);
	int count = 0;
	while (!fatalError) {
		std::stringstream repstr;
		repstr << "@[" << count << "]";
		std::string enc = extractString(retval, '(', ')', repstr.str());
		if (enc.empty())
			break;
		if(debugMode)
			debugStr << " eval(\"" << enc << "\")" << std::endl;
		eval(count++, enc);
	}
	if (!fatalError) {
		if (debugMode)
			debugStr << " eval(\"" << retval << "\")" << std::endl;
		eval(count, retval);
		result = lastResult;
		return true;
	}
	return false;
}
