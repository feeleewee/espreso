
#include "parser.h"

using namespace espreso::input;

WorkbenchParser::WorkbenchParser(): bodyCounter(0), nSelection(-1), eSelection(-1)
{
	_commands["/wb"] = WorkbenchCommands::WB;
	_commands["nblock"] = WorkbenchCommands::NBLOCK;
	_commands["eblock"] = WorkbenchCommands::EBLOCK;
	_commands["cmblock"] = WorkbenchCommands::CMBLOCK;
	_commands["MP"] = WorkbenchCommands::MP;

	_commands["et"] = WorkbenchCommands::ET;
	_commands["cmsel"] = WorkbenchCommands::CMSEL;
	_commands["d"] = WorkbenchCommands::DISPLACEMENT;
	_commands["nsel"] = WorkbenchCommands::NSEL;
	_commands["_loadvari"] = WorkbenchCommands::LOADVAR;
}

std::vector<std::string> WorkbenchParser::divide(std::string &line, std::string delim)
{
	std::vector<std::string> parts;
	size_t start, end = -1;
	do {
		start = end + 1;
		end = line.find_first_of(delim, start);
		parts.push_back(line.substr(start, end - start));
	} while(end != std::string::npos);

	return parts;
}

std::vector<int> WorkbenchParser::parseBlockHeader(std::string &line)
{
	std::vector<int> sizes;
	size_t start, end = 0;
	std::string substr;
	do {
		start = end + 1;
		end = line.find_first_of(",)", start);
		substr = line.substr(start, end - start);
		std::vector<std::string> parts = divide(substr, "ie");
		sizes.insert(sizes.end(), std::stoi(parts[0]), std::stoi(parts[1]));
	} while(end < line.size() && line[end] != ')');

	return sizes;
}

bool WorkbenchParser::trash(const std::string &line)
{
	if (line.size() && line[0] == '!') { // comment
		return false;
	}
	return false;
}

WorkbenchCommands WorkbenchParser::process()
{
	while (_file.good()) {
		do {
			getline(_file, _line);
		} while (trash(_line));

		std::string command = _line.substr(0, _line.find_first_of(','));
		auto it = _commands.find(command);
		if (it != _commands.end()) {
			switch (it->second) {
			case WorkbenchCommands::ET:       et(); break;
			case WorkbenchCommands::CMSEL:    cmsel(); break;
			case WorkbenchCommands::NSEL:     nsel(); break;

			default: {
				return it->second;
			}
			}
		}

	}

	return WorkbenchCommands::END;
}

bool WorkbenchParser::workbench(const std::string type, const std::string status)
{
	std::vector<std::string> params = divide(_line);
	return params[1].compare(0, type.size(), type) == 0 && params[2].compare(0, status.size(), status) == 0;
}

void WorkbenchParser::nblock(Coordinates &coordinates)
{
	std::vector<std::string> params = divide(_line);
	eslocal NUMFILED, Solkey, NDMAX, NDSEL; // TODO: use all parameters
	switch (params.size()) {
	case 5:
		NDSEL = std::stol(params[3]);
	case 4:
		NDMAX = std::stol(params[3]);
	case 3:
		Solkey = std::stol(params[2]);
		ESINFO(GLOBAL_ERROR) << "The input point format is not implemented in ESPRESO";
	case 2:
		NUMFILED = std::stol(params[1]);
	}

	getline(_file, _line);
	std::vector<int> sizes = parseBlockHeader(_line);
	size_t offset;

	Point point;
	size_t id;

	while (true) {
		getline(_file, _line);
		id = std::stol(_line.substr(0, sizes[0]));
		if (id == -1) {
			return; // TODO: loading more NBLOCKs
		}
		offset = sizes[0];

		point.x = std::stod(_line.substr(offset, sizes[1]));
		offset += sizes[1];
		point.y = std::stod(_line.substr(offset, sizes[2]));
		offset += sizes[2];
		point.z = std::stod(_line.substr(offset, sizes[3]));
		offset += sizes[3];

		coordinates.add(point, id - 1, id - 1);
	}
}

void WorkbenchParser::eblock(std::vector<Element*> &elements)
{
	int MATERIAL, ETYPE, CONSTANT, COORDINATES, NODES, PARAM_SIZE, NODE_SIZE;
	bool SOLID;

	std::vector<std::string> params = divide(_line);
	eslocal NUMNODES, NDMAX, NDSEL;
	std::string solid = "SOLID";
	switch (params.size()) {
	case 5:
		NDSEL = params[4].size() ? std::stol(params[4]) : 0;
	case 4:
		NDMAX = params[3].size() ? std::stol(params[3]) : 0;
	case 3:
		//if (params[2].size() != solid.size() || !std::equal(params[2].begin(), params[2].end(), solid.begin(), CaseInsensitiveCompare::equals)) {
		if (StringCompare::caseInsensitiveEq(params[2], "SOLID")) {
			SOLID = false;
			ETYPE = 1;
			CONSTANT = 2;
			MATERIAL = 3;
			COORDINATES = 4;
			PARAM_SIZE = 5;
		} else { // SOLID element
			SOLID = true;
			MATERIAL = 0;
			ETYPE = 1;
			CONSTANT = 2;
			COORDINATES = 4;
			NODES = 8;
			PARAM_SIZE = 11;
		}
		NUMNODES = std::stoi(params[1]);
		break;
	default:
		ESINFO(GLOBAL_ERROR) << "Unknown eblock format";
	}

	getline(_file, _line);
	std::vector<int> sizes = parseBlockHeader(_line);

	if (NDSEL) {
		elements.reserve(elements.size() + NDSEL);
	}
	std::vector<eslocal> values(38), eParams(Element::PARAMS_SIZE);
	for (eslocal i = 0; i < (NDSEL ? NDSEL : i + 1); i++) {
		getline(_file, _line);
		int start = 0;
		NODE_SIZE = 0;
		for (size_t i = 0; i < sizes.size() && _line.size() > start + sizes[i]; i++) {
			values[i] = std::stol(_line.substr(start, sizes[i]));
			if (i >= PARAM_SIZE) {
				values[i]--;
				NODE_SIZE++;
			}
			if (NDSEL == 0 && values[i] == -2) { // -2 == -1 - 1
				break;
			}
			start += sizes[i];
		}
		if (SOLID && values[NODES] > NUMNODES - PARAM_SIZE) {
			start = 0;
			getline(_file, _line);
			for (size_t i = 0; i < values[NODES] - sizes.size() + PARAM_SIZE; i++) {
				values[sizes.size() + i] = std::stol(_line.substr(start, sizes[i])) - 1;
				start += sizes[i];
			}
		}

		if (SOLID) {
			NODE_SIZE = values[NODES];
		}

		eParams[Element::MATERIAL] = values[MATERIAL] - 1;
		eParams[Element::CONSTANT] = values[CONSTANT];
		eParams[Element::COORDINATES] = values[COORDINATES] - 1;
		eParams[Element::BODY] = bodyCounter;
		elements.push_back(AnsysUtils::createElement(values.data() + PARAM_SIZE, NODE_SIZE, eParams.data(), eType[values[ETYPE] - 1]));
	}
	bodyCounter++;
}

void WorkbenchParser::mp(std::vector<Material> &materials)
{
	std::vector<std::string> params = divide(_line);

	int mNumber = std::stoi(params[2]);
	materials.resize(mNumber--);

	if (!params[1].compare(0, 4, "DENS")) {
		materials[mNumber].density = std::stod(params[3]);
	}
	if (!params[1].compare(0, 4, "ALPX")) {
		materials[mNumber].termalExpansion = std::stod(params[3]);
	}
	if (!params[1].compare(0, 1, "C")) {
		materials[mNumber].termalCapacity = std::stod(params[3]);
	}
	if (!params[1].compare(0, 3, "KXX")) {
		materials[mNumber].termalConduction = std::stod(params[3]);
	}
	if (!params[1].compare(0, 2, "EX")) {
		materials[mNumber].youngModulus = std::stod(params[3]);
	}
	if (!params[1].compare(0, 4, "NUXY")) {
		materials[mNumber].poissonRatio = std::stod(params[3]);
	}
}

void WorkbenchParser::displacement(std::vector<BoundaryCondition*> &conditions)
{
	std::vector<std::string> params = divide(_line);

	if (!params[1].compare(0, 3, "all")) {
		if (!params[2].compare(0, 3, "all")) {
			if (nSelection == -1) {
				ESINFO(GLOBAL_ERROR) << "Displacement on all nodes is not supported";
			} else {
				conditions[nSelection]->set(0, ConditionType::DIRICHLET, { true, true, true });
			}
		} else {
			ESINFO(GLOBAL_ERROR) << "Not implemented d option '" << params[2] << "'";
		}
	} else {
		ESINFO(GLOBAL_ERROR) << "Not implemented d option '" << params[1] << "'";
	}
}

void WorkbenchParser::loadvar()
{

}

void WorkbenchParser::cmblock(std::vector<BoundaryCondition*> &conditions)
{
	std::vector<std::string> params = divide(_line);

	if (params[2].compare(0, 4, "NODE") == 0) {
		eslocal size = std::stol(params[3]);
		NodeCondition *nc = new NodeCondition();

		getline(_file, _line);
		std::vector<int> sizes = parseBlockHeader(_line);

		getline(_file, _line);
		int start = 0, n = 0;
		nc->nodes().reserve(size);
		while (nc->nodes().size() < size) {
			nc->nodes().push_back(std::stol(_line.substr(start, sizes[n])) - 1);
			start += sizes[n++];
			if (n % sizes.size() == 0) {
				getline(_file, _line);
				start = 0;
				n = 0;
			}
		}
		conditions.push_back(nc);
		selections.push_back({params[1], ConditionElements::NODES});
	} else {
		ESINFO(GLOBAL_ERROR) << "Not implemented loading of cmblock of elements";
	}
}

void WorkbenchParser::eblock(std::vector<BoundaryCondition*> &conditions)
{
	SurfaceCondition *ec = new SurfaceCondition();
	eblock(ec->faces());

	conditions.push_back(ec);
	std::stringstream ss;
	ss << eType.back();
	selections.push_back({ss.str(), ConditionElements::ELEMENTS});
}

void WorkbenchParser::et()
{
	std::vector<std::string> params = divide(_line);
	int n = std::stoi(params[1]);
	eType.resize(n);
	eType[n - 1] = std::stoi(params[2]);
}

void WorkbenchParser::cmsel()
{
	std::vector<std::string> params = divide(_line);

	if (params[1].compare(0, 1, "s") == 0) {
		for (size_t i = 0; i < selections.size(); i++) {
			if (!selections[i].first.compare(0, params[2].size() - 1, params[2], 0, params[2].size() - 1)) {
				switch (selections[i].second) {
				case ConditionElements::NODES:
					nSelection = i;
					return;
				case ConditionElements::ELEMENTS:
					eSelection = i;
					return;
				}
			}
		}
	} else {
		ESINFO(GLOBAL_ERROR) << "Not implemented cmsel option '" << params[1] << "'";
	}
}

void WorkbenchParser::nsel()
{
	std::vector<std::string> params = divide(_line);

	if (params[1].compare(0, 3, "all") == 0) {
		nSelection = -1;
	} else {
		ESINFO(GLOBAL_ERROR) << "Not implemented nsel option '" << params[1] << "'";
	}
}

void WorkbenchParser::esel()
{
	std::vector<std::string> params = divide(_line);

	if (params[1].compare(0, 3, "all") == 0) {
		eSelection = - 1;
	} else {
		ESINFO(GLOBAL_ERROR) << "Not implemented esel option '" << params[1] << "'";
	}
}



