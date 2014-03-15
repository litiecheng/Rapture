#include "g_local.h"

// TILE PARSING
static unordered_map<const char*, jsonParseFunc> tileParseFields;

void ParseSubtile(cJSON* json, void* ptTile, int tileNum) {
	Tile* t = (Tile*)ptTile;
	if(!t) {
		return;
	}
	cJSON* jsonNode = cJSON_GetObjectItem(json, "low");
	if(jsonNode) {
		t->lowmask |= (1 << tileNum);
	}

	jsonNode = cJSON_GetObjectItem(json, "high");
	if(jsonNode) {
		t->highmask |= (1 << tileNum);
	}

	jsonNode = cJSON_GetObjectItem(json, "light");
	if(jsonNode) {
		t->lightmask |= (1 << tileNum);
	}

	jsonNode = cJSON_GetObjectItem(json, "vis");
	if(jsonNode) {
		t->vismask |= (1 << tileNum);
	}
}

void InitTileParseFields() {
#define SUBTILE_PARSER(x) [](cJSON* j, void* p) -> void { ParseSubtile(j, p, x); }
#define INT_PARSER(x) \
[](cJSON* j, void* p) -> void { Tile* t = (Tile*)p; t->##x## = cJSON_ToInteger(j); }
	tileParseFields.insert(make_pair("subtile0", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile1", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile2", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile3", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile4", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile5", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile6", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile7", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile8", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile9", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile10", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile11", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile12", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile13", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile14", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("subtile15", SUBTILE_PARSER(0)));
	tileParseFields.insert(make_pair("lowmask", INT_PARSER(lowmask)));
	tileParseFields.insert(make_pair("highmask", INT_PARSER(highmask)));
	tileParseFields.insert(make_pair("lightmask", INT_PARSER(lightmask)));
	tileParseFields.insert(make_pair("vismask", INT_PARSER(vismask)));
}


MapLoader::MapLoader(const string& presetsPath, const string& tilePath) {
	// Load the tiles.
	InitTileParseFields();
	vector<string> paths;
	int numFiles = trap->ListFilesInDir(tilePath.c_str(), paths, ".json");
	if(numFiles <= 0) {
		R_Printf("FATAL: MapLoader could not load tiles (missing?)\n");
		assert(0);
	}
	for(auto it = paths.begin(); it != paths.end(); ++it) {
		Tile t;
		JSON_ParseFile((char*)it->c_str(), tileParseFields, &t);
		vTiles.push_back(t);
		mTileResolver.insert(make_pair(t.name, vTiles.end()-1));
	}
}