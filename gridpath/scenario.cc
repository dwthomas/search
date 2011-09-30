#include "scenario.hpp"
#include "../utils/utils.hpp"
#include "../search/main.hpp"
#include <iostream>
#include <cmath>

static const float Epsilon = 0.001;

Scenario::Scenario(int ac, char *av[]) :
		argc(ac), argv(av), maproot("./"), lastmap(NULL),
		entry(-1), nentries(0) {
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-maproot") == 0 && i < argc - 1) {
			maproot = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-entry") == 0 && i < argc - 1) {
			entry = strtol(argv[i+1], NULL, 10);
			i++;
		}
	}
	if (maproot[maproot.size()-1] != '/')
		maproot += '/';
}

Scenario::~Scenario(void) {
	if (lastmap)
		delete lastmap;
}

void Scenario::run(std::istream &in) {
	checkver(in);
	outputhdr(stdout);

	ScenarioEntry ent(*this);
	while (in >> ent) {
		nentries++;
		if (entry >= 0 && nentries - 1 != entry)
			continue;

		Search<GridPath> *srch = getsearch<GridPath>(argc, argv);
		Result<GridPath> r = ent.run(srch);
		ent.outputrow(stdout, nentries-1, r);
		res.add(r);
		delete srch;
	}

	res.output(stdout);
	dfpair(stdout, "number of entries", "%u", nentries);
}

void Scenario::checkver(std::istream &in) {
	float ver;
	std::string verstr;
	std::cin >> verstr >> ver;
	if (verstr != "version")
		fatal("Expected a version header");
	if (ver != 1.0)
		fatal("Version %g is unsupported.  Latest supported version is 1.0", ver);
}

void Scenario::outputhdr(FILE *out) {
	dfrowhdr(out, "run", 15,
		"num", "bucket", "width", "height", "start x", "start y",
		"finish x", "finish y", "optimal sol", "nodes expanded",
		"nodes generated", "sol cost", "sol length",
		"wall time", "cpu time");
}

GridMap *Scenario::getmap(std::string mapfile) {
	std::string path = maproot + mapfile; 
	if (!lastmap || lastmap->filename() != path) {
		if (lastmap)
			delete lastmap;
		lastmap = new GridMap(path);
	}
	return lastmap;
}

ScenarioEntry::ScenarioEntry(Scenario &s) : scen(s) { }

Result<GridPath> ScenarioEntry::run(Search<GridPath> *srch) {
	GridPath d(scen.getmap(mapfile), x0, y0, x1, y1);
	GridPath::State s0 = d.initialstate();

	Result<GridPath> &r = srch->search(d, s0);
	if (fabsf(r.cost - opt) > Epsilon)
		fatal("Expected optimal cost of %g, got %g\n", opt, r.cost);

	return r;
}

void ScenarioEntry::outputrow(FILE *out, unsigned int n, Result<GridPath> &r) {
	dfrow(out, "run", "uuuuuuuuguugugg",
		(unsigned long) n, (unsigned long) bucket, (unsigned long) w,
		(unsigned long) h, (unsigned long) x0, (unsigned long) y0,
		(unsigned long)  x1, (unsigned long) y1, opt, r.expd, r.gend,
		r.cost, (unsigned long) r.path.size(), r.wallend - r.wallstrt,
		r.cpuend - r.cpustrt);
} 

std::istream &operator>>(std::istream &in, ScenarioEntry &s) {
	in >> s.bucket;
	in >> s.mapfile;
	in >> s.w >> s.h;
	in >> s.x0 >> s.y0;
	in >> s.x1 >> s.y1;
	in >> s.opt;
	return in;
}