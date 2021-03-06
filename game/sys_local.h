#pragma once
#include "sys_shared.h"
#include "tr_shared.h"
#include "ui_shared.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <RaptureAsset.h>

#define R_Error Sys_Error

typedef void* ptModule;
typedef void* ptModuleFunction;

//
// sys_main.cpp
//
class GameModule {
	// modcode that gets loaded via shared objects
private:
	ptModule ptModuleHandle;
public:
	GameModule(string modulename);
	~GameModule();
	gameExports_s* GetRefAPI(gameImports_s* import);
};

class RaptureGame {
private:
	static RaptureGame* singleton;

	uint32_t uGameFlags;
public:
	enum GameFlags {
		Rapture_Initialized,
		Rapture_FatalError,
		Rapture_EditorLoaded,
		Rapture_GameLoaded,
		Rapture_Quitting,
	};

	static RaptureGame* GetSingleton(int argc = 0, char** argv = nullptr);
	static void DestroySingleton();

	bool HasFlags(const GameFlags flags);
	void AddFlag(const GameFlags flags);

	GameModule*		game;
	GameModule*		editor;
	gameExports_s*	trap;

	RaptureGame(int argc, char **argv);
	~RaptureGame();

	RaptureGame(const RaptureGame& other) {}
	RaptureGame& operator= (RaptureGame other) { return *this; }

	void HandleCommandline(int argc, char **argv);
	void RunLoop();


	GameModule* CreateGameModule(const char* bundle);
	bool AllowedToStartNewModules();
	void StartGameFromFile(const char* szFileName, bool bMultiplayer);
	void JoinRemoteGame(const char* szFileName, const char* szHostName);
	void StartEditor();
	void SaveAndExit();

	static GameModule* GetGameModule();
	static GameModule* GetEditorModule();
	static gameExports_s* GetImport();

	static int RaptureInputCallback(void *notUsed, SDL_Event* e);
};

//
// CvarSystem.cpp
//

template<typename T>
struct CvarValueSet {
	T defaultVal;
	T currentVal;

	void AssignBoth(T value) { defaultVal = currentVal = value; }
};

template<>
struct CvarValueSet<char*> {
	char defaultVal[64];
	char currentVal[64];

	void AssignBoth(char* value) { 
		strncpy(defaultVal, value, sizeof(defaultVal));
		strncpy(currentVal, value, sizeof(currentVal));
	}
};

class Cvar {
public:
	enum cvarType_e {
		CV_STRING,
		CV_INTEGER,
		CV_FLOAT,
		CV_BOOLEAN
	};
private:
	string name;
	string description;
	union {
		CvarValueSet<char*> s;
		CvarValueSet<int> i;
		CvarValueSet<float> v;
		CvarValueSet<bool> b;
	};
	union {
		void (*ptsChangeCallback)(char *to);
		void (*ptiChangeCallback)(int to);
		void (*ptfChangeCallback)(float to);
		void (*ptbChangeCallback)(bool to);
	};

	cvarType_e type;
	int flags;
	void AssignHardValue(char* value);
	void AssignHardValue(int value);
	void AssignHardValue(float value);
	void AssignHardValue(bool value);
	bool registered;
public:
	Cvar(Cvar&& other);
	Cvar(const string& sName, const string& sDesc, int iFlags, char* startValue);
	Cvar(const string& sName, const string& sDesc, int iFlags, int startValue);
	Cvar(const string& sName, const string& sDesc, int iFlags, float startValue);
	Cvar(const string& sName, const string& sDesc, int iFlags, bool startValue);
	Cvar();
	~Cvar();
	Cvar& operator= (char* value);
	Cvar& operator= (int value);
	Cvar& operator= (float value);
	Cvar& operator= (bool value);

	inline cvarType_e GetType() { return type; }
	inline int GetFlags() { return flags; }
	inline string GetName() { return name; }
	inline string GetDescription() { return description; }

	void SetValue(char* value);
	void SetValue(int value);
	void SetValue(float value);
	void SetValue(bool value);
	inline char* String() { return s.currentVal; }
	inline int Integer() { return i.currentVal; }
	inline float Value() { return v.currentVal; }
	inline bool Bool() { return b.currentVal; }
	inline char* DefaultString() { return s.defaultVal; }
	inline int DefaultInteger() { return i.defaultVal; }
	inline float DefaultValue() { return v.defaultVal; }
	inline bool DefaultBool() { return b.defaultVal; }

	string TypeToString();

	void AddCallback(void* function);
	void RunCallback();

	template<typename T>
	static Cvar* Get(const char* sName, const char* sDesc, int iFlags, T startValue) {
		try {
			auto it = CvarSystem::cvars.find(sName);
			if(it == CvarSystem::cvars.end()) {
				throw string("not registered");
			}
			Cvar* cv = it->second;
			if(!cv->registered) {
				throw string("not registered");
			}
			return cv;
		}
		catch( string e ) {
			// register a new cvar
			R_Message(PRIORITY_MESSAGE, "%s %s\n", sName, e.c_str());
			return CvarSystem::RegisterCvar(sName, sDesc, iFlags, startValue);
		}
	}

	static bool Exists(const string& sName);
friend class CvarSystem;
friend class Cvar;
};

// Whenever we use /set or /seta on nonexisting cvars, those cvars are chucked into the cache
struct CvarCacheObject {
	string initvalue;
	bool archive;
};

class CvarSystem {
	static unordered_map<string, Cvar*> cvars;
	static unordered_map<string, CvarCacheObject*> cache;
	static bool init;
	static void Cache_Free(const string& sName);
	static void ArchiveCvars();
public:
	static string CvarSystem::GetNextCvar(const string& previous, bool& bFoundCommand);
	static string CvarSystem::GetFirstCvar(bool& bFoundCommand);

	static Cvar* RegisterCvar(Cvar *cvar);
	static Cvar* RegisterCvar(const char* sName, const char* sDesc, int iFlags, char* startValue);
	static Cvar* RegisterCvar(const char* sName, const char* sDesc, int iFlags, int startValue);
	static Cvar* RegisterCvar(const char* sName, const char* sDesc, int iFlags, float startValue);
	static Cvar* RegisterCvar(const char* sName, const char* sDesc, int iFlags, bool startValue);
	static Cvar* RegisterStringVar(const char* sName, const char* sDesc, char* defaultValue, int iFlags) { 
		return RegisterCvar(sName, sDesc, iFlags, defaultValue);
	}
	static Cvar* RegisterIntegerVar(const char* sName, const char* sDesc, int defaultValue, int iFlags) {
		return RegisterCvar(sName, sDesc, iFlags, defaultValue);
	}
	static Cvar* RegisterFloatVar(const char* sName, const char* sDesc, float defaultValue, int iFlags) {
		return RegisterCvar(sName, sDesc, iFlags, defaultValue);
	}
	static Cvar* RegisterBooleanVar(const char* sName, const char* sDesc, bool defaultValue, int iFlags) {
		return RegisterCvar(sName, sDesc, iFlags, defaultValue);
	}
	static void CacheCvar(const string& sName, const string& sValue, bool bArchive = false);
	static void Initialize();
	static void Destroy();

	static int GetCvarFlags(const string& sName) { return cvars[sName]->flags; }
	static void SetCvarFlags(const string& sName, int flags) { cvars[sName]->flags = flags; }
	static Cvar::cvarType_e GetCvarType(const string& sName) { return cvars[sName]->type; }

	static void SetStringValue(const string &sName, char* newValue) { strcpy(cvars[sName]->s.currentVal, newValue); cvars[sName]->RunCallback(); }
	static void SetIntegerValue(const string &sName, int newValue) { cvars[sName]->i.currentVal = newValue; cvars[sName]->RunCallback(); }
	static void SetFloatValue(const string &sName, float newValue) { cvars[sName]->v.currentVal = newValue; cvars[sName]->RunCallback(); }
	static void SetBooleanValue(const string &sName, bool newValue) { cvars[sName]->b.currentVal = newValue; cvars[sName]->RunCallback(); }

	static string GetStringValue(const char* sName);
	static int GetIntegerValue(const char* sName);
	static float GetFloatValue(const char* sName);
	static bool GetBooleanValue(const char* sName);

	static char* GetStringValue(Cvar* pCvar) { return pCvar->String(); }
	static int GetIntegerValue(Cvar* pCvar) { return pCvar->Integer(); }
	static float GetFloatValue(Cvar* pCvar) { return pCvar->Value(); }
	static bool GetBooleanValue(Cvar* pCvar) { return pCvar->Bool(); }

	static bool CompareCvars(pair<string, Cvar*> pFirst, pair<string, Cvar*> pSecond);
	static void ListCvars();

	static bool ProcessCvarCommand(const string& sName, const vector<string>& VArguments);

	static int EXPORT_IntValue(Cvar* cvar, int* value);
	static char* EXPORT_StrValue(Cvar* cvar, char* value);
	static bool EXPORT_BoolValue(Cvar* cvar, bool* value);
	static float EXPORT_Value(Cvar* cvar, float* value);
friend class Cvar;
};

/* Memory */

//
// Zone.cpp
//

namespace Zone {
	enum zoneTags_e {
		TAG_NONE,
		TAG_CVAR,
		TAG_FILES,
		TAG_RENDERER,
		TAG_IMAGES,
		TAG_MATERIALS,
		TAG_FONT,
		TAG_NETWORK,
		TAG_CUSTOM, // should only be used by mods
		MAX_ENGINE_TAGS // last of the engine tags, but we can add more for vms
	};

	extern string tagNames[];

	struct ZoneChunk {
		size_t memInUse;
		bool isClassObject;

		ZoneChunk(size_t mem, bool bClass) : 
		memInUse(mem), isClassObject(bClass) {}
		ZoneChunk() { memInUse = 0; isClassObject = false; }
	};

	struct ZoneTag {
		size_t zoneInUse;
		size_t peakUsage;
		map<void*, ZoneChunk> zone;

		ZoneTag() : zoneInUse(0), peakUsage(0) { }
	};

	class MemoryManager {
	private:
		unordered_map<string, ZoneTag> zone;
	public:
		void* Allocate(int iSize, zoneTags_e tag);
		void* Allocate(int iSize, const string& tag);
		void Free(void* memory);
		void FastFree(void* memory, const string& tag);
		void FreeAll(const string& tag);
		void* Reallocate(void *memory, size_t iNewSize);
		void CreateZoneTag(const string& tag) { zone[tag] = ZoneTag(); }
		MemoryManager();
		~MemoryManager(); // deliberately ignoring rule of three
		void PrintMemUsage();

		template<typename T>
		T* AllocClass(zoneTags_e tag) {
			T* retVal = new T();
			ZoneChunk zc(sizeof(T), true);
			zone[tagNames[tag]].zoneInUse += sizeof(T);
			if(zone[tagNames[tag]].zoneInUse > zone[tagNames[tag]].peakUsage) {
				zone[tagNames[tag]].peakUsage = zone[tagNames[tag]].zoneInUse;
			}
			zone[tagNames[tag]].zone[retVal] = zc;
			return retVal;
		}
	};

	extern MemoryManager *mem;

	void Init();
	void Shutdown();

	void* Alloc(int iSize, zoneTags_e tag);
	void* Alloc(int iSize, const string& tag);
	void* VMAlloc(int iSize, const char* tag)/* { return Alloc(iSize, tag); }*/ ;
	void  Free(void* memory);
	void  FastFree(void *memory, const string& tag);
	void  VMFastFree(void* memory, const char* tag)/* { FastFree(memory, tag); }*/ ;
	void  FreeAll(const string& tag);
	void  VMFreeAll(const char* tag)/* { FreeAll(tag); }*/ ;
	void* Realloc(void *memory, size_t iNewSize);
	void  NewTag(const char* tag)/* { mem->CreateZoneTag(tag); }*/ ;
	template<typename T>
	T* New(zoneTags_e tag) { return mem->AllocClass<T>(tag); }
	void MemoryUsage();
};

//
//
//

template<typename T>
class MutexVariable {
	T var;
	mutex mut;
public:
	bool GetVar(T& x) { if (!mut.try_lock()) return false; x = var; return true; }
	T& GetVar() { mut.lock(); return var; }
	void Descope() { mut.unlock(); }
	MutexVariable<T>() {}
	MutexVariable<T>(T& other) { var = other; }
};

//
// FileSystem.cpp
//

namespace Filesystem {
	extern Cvar* fs_core;
	extern Cvar* fs_homepath;
	extern Cvar* fs_basepath;
	extern Cvar* fs_game;

	extern Cvar* fs_multithreaded;
	extern Cvar* fs_threads;

	void Init();
	void Exit();

	string& ResolveFilePath(string& filePath, const string& file, const string& mode);
	string ResolveAssetPath(const string& assetName);
	char* ResolveFilePath(char* buffer, size_t bufferSize, const char* file, const char* mode);
	const char* ResolveAssetPath(const char* assetName);

	void LoadRaptureAsset(RaptureAsset** pAsset, const string& assetName);
	AssetComponent* FindComponentByName(RaptureAsset* pAsset, const char* compName);

	void QueueFileOpen(File* pFile, fileOpenedCallback callback);
	void QueueFileRead(File* pFile, void* data, size_t dataSize, fileReadCallback callback);
	void QueueFileWrite(File* pFile, void* data, size_t dataSize, fileWrittenCallback callback);
	void QueueFileClose(File* pFile, fileClosedCallback callback);

	void QueueResource(Resource* pRes, assetRequestCallback callback);

	void ListAllFilesInPath(vector<string>& vFiles, const char* extension, const char* folder);
};

/* A File is something which we pull from the hard drive */
class File {
private:
	string path;
	string mode;
	FILE* fp;

	enum Flags {
		File_Opened		= 1,
		File_Read		= 2,
		File_Written	= 4,
		File_Closed		= 8,
		File_Bad		= 16
	};
	uint8_t flags;

	File();
public:
	File(const File& other);

	static File*	OpenAsync(const char* file, const char* mode = "rb+", fileOpenedCallback callback = nullptr);
	static void		ReadAsync(File* pFile, void* data, size_t dataSize, fileReadCallback callback = nullptr);
	static void		WriteAsync(File* pFile, void* data, size_t dataSize, fileWrittenCallback callback = nullptr);
	static void		CloseAsync(File* pFile, fileClosedCallback callback = nullptr);

	static bool		AsyncOpened(File* pFile);
	static bool		AsyncRead(File* pFile);
	static bool		AsyncWritten(File* pFile);
	static bool		AsyncClosed(File* pFile);
	static bool		AsyncBad(File* pFile);

	static File*	OpenSync(const char* file, const char* mode = "rb+");
	static bool		ReadSync(File* pFile, void* data, size_t dataSize);
	static bool		WriteSync(File* pFile, void* data, size_t dataSize);
	static bool		CloseSync(File* pFile);

	void DequeOpen(fileOpenedCallback callback);
	void DequeRead(void* data, size_t dataSize, fileReadCallback callback);
	void DequeWrite(void* data, size_t dataSize, fileWrittenCallback callback);
	void DequeClose(fileClosedCallback callback);

	const char* GetFileMode() { return mode.c_str(); }
	const char* GetFilePath() { return path.c_str(); }
	const FILE* GetFilePointer() { return fp; }
};

/* A resource is something that is streamed from an asset file */
class Resource {
	AssetComponent* component;
	bool bRetrieved;
	bool bBad;

	string szAssetFile;
	string szComponent;

	Resource();
public:
	static Resource* ResourceAsync(const char* asset, const char* component, assetRequestCallback callback = nullptr);
	static Resource* ResourceAsyncURI(const char* uri, assetRequestCallback callback = nullptr);
	static Resource* ResourceSync(const char* asset, const char* component);
	static Resource* ResourceSyncURI(const char* uri);
	static void FreeResource(Resource* pResource);

	void DequeRetrieve(assetRequestCallback callback);

	bool Retrieved();
	bool Bad();
	static bool ResourceRetrieved(Resource* pResource) { return pResource->Retrieved(); }
	static bool ResourceBad(Resource* pResource) { return pResource->Bad(); }

	AssetComponent* GetAssetComponent() { return component; }
	static AssetComponent* GetAssetComponent(Resource* pRes) { return pRes->GetAssetComponent(); }
};

struct AsyncFileTask {
	enum TaskType {
		Task_Open,
		Task_Read,
		Task_Write,
		Task_Close
	};

	TaskType type;
	File* pFile;
	void* callback;
	void* data;
	size_t	dataSize;

	AsyncFileTask& operator=(const AsyncFileTask& rhs);
};

struct AsyncResourceTask {
	enum TaskType {
		Task_Request
	};

	TaskType type;
	Resource* pResource;
	void* callback;

	AsyncResourceTask& operator=(const AsyncResourceTask& rhs);
};

//
// CmdSystem.cpp
//

namespace Cmd {
	void ProcessCommand(const char *cmd);
	void AddTabCompletion(const string& cmdName);
	void AddCommand(const string& cmdName, conCmd_t cmd);
	void AddCommand(const char* cmdName, conCmd_t cmd);
	void RemoveCommand(const string& cmdName);
	void RemoveCommand(const char* cmdName);
	void ClearCommandList();
	void ListCommands();
	vector<string> Tokenize(const string &str);
	string TabComplete(const string& input);
};


//
// Input.cpp
//

class InputManager {
private:
	map<SDL_Scancode, string> binds;
	list<SDL_Scancode> thisFrameKeysDown;
	void BindCommandMouse(const string& keycodeArg, const string& commandArg);
	void (*Keycatcher)(SDL_Keycode key);
public:
	void SendKeyUpEvent(SDL_Keysym key, char* text);
	void SendKeyDownEvent(SDL_Keysym key, char* text);
	void InputFrame();
	void ExecuteBind(const string& bindName);
	void BindCommand(const string& keycodeArg, string commandArg);
	void SendMouseButtonEvent(unsigned int buttonId, unsigned char state, int x, int y);
	void SendMouseMoveEvent(int x, int y);
	void SendTextInputEvent(char* text);
	InputManager();
};

extern InputManager *Input;
void InitInput();
void DeleteInput();
extern const string keycodeNames[];

//
// Timer.cpp
//
class Timer {
protected:
	unsigned long ulStartTicks;
	unsigned long ulPausedTicks;

	bool bIsStarted;
	bool bIsPaused;

	string sTimerName;
public:
	Timer();
	Timer(const string& sName);

	void Start();
	void Stop();
	void Pause();
	void Unpause();

	unsigned long GetTicks();

	bool IsStarted() { return bIsStarted; };
	bool IsPaused() { return bIsPaused; };
};


//
// Dispatch.cpp
//
class Dispatch {
private:
	File* ptLogFile;

	int iHiddenMask;
	int iShutdownMask;
	int iMessageMask;

	bool bSetup;
	vector<pair<int, string>> vPreSetupMessages;
public:
	Dispatch(const int _iHiddenMask, const int _iShutdownMask, const int _iMessageMask);
	void CatchError();
	void PrintMessage(const int iPriority, const char* message);
	void Setup();
	~Dispatch();

	static void ChangeHiddenMask(int newValue);
	static void ChangeShutdownMask(int newValue);
	static void ChangeMessageMask(int newValue);
};
extern Dispatch* ptDispatch;

/* Netcode. Note that only TCP is supported, not UDP. */

#ifdef _WIN32
typedef SOCKET socket_t;
#else
typedef int socket_t;
#endif

#define RAPTURE_DEFAULT_PORT		1750
#define RAPTURE_DEFAULT_BACKLOG		32
#define RAPTURE_DEFAULT_MAXCLIENTS	8

#define RAPTURE_NETPROTOCOL			0

// The Network namespace contains all of the basic, low-level functions 
namespace Network {
	enum NetworkInterfaceCallbacks {
		NIC_ALL = -1,			// Not actually a callback, just a sentinel value for all callbacks
		NIC_EXIT,				// trap->saveAndExit
		NIC_SERVERFRAME,		// trap->serverFrame
		NIC_CLIENTFRAME,		// trap->clientFrame
		NIC_SERVERSERIALIZE,	// trap->serializepackettoclient
		NIC_SERVERDESERIALIZE,	// trap->deserializepacketfromclient
		NIC_CLIENTSERIALIZE,	// trap->serializepackettoserver
		NIC_CLIENTDESERIALIZE,	// trap->deserializepacketfromserver
		NIC_MAX
	};

	enum Netmode_e {
		Netmode_Red,		// No connections allowed on the server, and any current connections are terminated
		Netmode_Yellow,		// No new connections allowed, current connections aren't terminated
		Netmode_Green,		// Anything goes
	};

	enum Netstate_e {
		Netstate_NoConnect,		// Not connected to any network, not listening
		Netstate_Listen,		// Listening
		Netstate_NeedAuth,		// Connected to a server, need authorization packet
		Netstate_Authorized,	// Connected to a server and authorized
	};

	typedef bool(*networkCallbackFunction)(...);
	typedef pair<Packet, int> packetMsg;

	extern Cvar* net_port;
	extern Cvar* net_serverbacklog;
	extern Cvar* net_maxclients;
	extern Cvar* net_netmode;
	extern Cvar* net_timeout;
	extern Cvar* net_ipv6;

	extern Socket*				localSocket;
	extern map<int, Socket*>	mOtherConnectedClients;
	extern Socket*				remoteSocket;
	extern vector<packetMsg>	vPacketsAwaitingSend;
	extern Netstate_e			currentNetState;
	extern vector<Socket*>		vTemporaryConnections;

	extern int			numConnectedClients;
	extern int			myClientNum;				// Client 0 is always the host
	extern int			lastFreeClientNum;
	extern networkCallbackFunction	callbacks[NIC_MAX];

	void Init();
	void Shutdown();
	void AddCallback(NetworkInterfaceCallbacks callback, networkCallbackFunction func);
	void RemoveCallback(NetworkInterfaceCallbacks callback);

	namespace Server {
		void QueuePacket(packetType_e packetType, int clientNum, void* extraData);
		void Frame();
		bool StartLocalServer();
		void DispatchSinglePacket(Packet& packet, int clientNum);
		void DropClient(int clientNum);
	}

	namespace Client {
		void QueuePacket(packetType_e packetType, void* extraData);
		void DisconnectFromRemote();
		void Frame();
		bool JoinServer(const char* hostname);
		void Connect(const char* hostname);
		void DispatchSinglePacket(Packet& packet);
	}

	namespace Packets {
		// Serverside handling
		namespace Server {
			void Packet_PingDeserialize(Packet& packet, int clientNum);
			void Packet_DropSerialize(Packet& packet, int clientNum, void* extraData);
			void Packet_DropDeserialize(Packet& packet, int clientNum);
			void Packet_ClientAttemptDeserialize(Packet& packet, int clientNum);
			void Packet_ClientAcceptSerialize(Packet& packet, int clientNum, void* extraData);
			void Packet_ClientDeniedSerialize(Packet& packet, int clientNum, void* extraData);
		}

		// Clientside handling
		namespace Client {
			void Packet_PingDeserialize(Packet& packet, int clientNum);
			void Packet_DropSerialize(Packet& packet, int clientNum, void* extraData);
			void Packet_DropDeserialize(Packet& packet, int clientNum);
			void Packet_ClientAttemptSerialize(Packet& packet, int clientNum, void* extraData);
			void Packet_ClientAcceptDeserialize(Packet& packet, int clientNum);
			void Packet_ClientDeniedDeserialize(Packet& packet, int clientNum);
		}
	}
};

//
// Socket.cpp
// Socket is just a wrapper for internal socket format, with some functions operating on it
// TODO: Make this into a class with inheritance (TCPSocket.cpp/UDPSocket.cpp)
//
struct Socket {
private:
	socket_t internalSocket;
	int af, type;

	bool SetNonBlocking();
	bool SendPacketHeader(PacketHeader& head);
	bool RecvPacketHeader(PacketHeader& head);
	bool SendEntireData(void* data, size_t dataSize);
	bool ReadEntireData(void* data, size_t dataSize);
public:
	uint64_t lastHeardFrom;
	uint64_t lastSpoken;

	Socket(int af_, int type_);
	Socket(addrinfo& pConnectingClient, socket_t socket);
	Socket(Socket& other);
	~Socket();

	bool Connect(const char* hostname, unsigned short port);
	bool StartListening(unsigned short port, uint32_t backlog);
	void Disconnect();

	template<typename T>
	T Read();

	template<typename T>
	void Write(T in);

	bool SendPacket(Packet& outgoing);
	bool ReadPacket(Packet& incoming);
	Socket* CheckPendingConnections();
	bool Select();
};

//
// TimeDate.cpp
//
namespace TimeDate {
	Rapture_TimeDate GetCurrent();
	void Subtract(Rapture_TimeDate a, Rapture_TimeDate b, Rapture_TimeDate* result);
	void Add(Rapture_TimeDate a, Rapture_TimeDate b, Rapture_TimeDate* result);
}

//
// FrameCapper.cpp
//
class FrameCapper {
protected:
	Timer capTimer;
	Cvar* capCvar;
	Cvar* hitchWarningCvar;
public:
	void StartFrame();
	void EndFrame();
	FrameCapper();
};

void setGameQuitting(const bool b);

//
// CmdSystem.cpp
//

void Sys_InitCommands();

//
// SaveGame.cpp
//

namespace SaveGame {
	const char* RequestSavegameInfo(bool bMultiplayer);
	void DeleteSavegame(const char* szSaveFilePath);
	void CreateSavegame(const char* charCreateJSON);
}

//
// <Platform>.cpp
//
char* Sys_FS_GetHomepath();
char* Sys_FS_GetBasepath();
string Sys_GetClipboardContents();
void Sys_SendToClipboard(string text);
void Sys_FS_MakeDirectory(const char* path);
ptModule Sys_LoadLibrary(string name);
void Sys_FreeLibrary(ptModule module);
ptModuleFunction Sys_GetFunctionAddress(ptModule module, string name);
bool Sys_Assertion(const char* msg, const char* file, const unsigned int line);
void Sys_Error(const char* error, ...);
void Sys_InitSockets();
void Sys_ExitSockets();
const char* Sys_SocketError(int& number);