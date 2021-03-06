#include "tr_local.h"

renderImports_s *trap = nullptr;

SDL_Window* pWindow = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_RendererInfo renderInfo;

SDL_Surface* renderSurf = nullptr;
SDL_Texture* renderTex = nullptr;

FadeEffectData fade;

// Cvars
Cvar* r_textureFiltering;

void InitCvars() {
	r_textureFiltering = trap->RegisterIntCvar("r_textureFiltering", 
		"Texture filter quality. 0 = nearest pixel, 1 = linear filtering, 2 = anisotropic filtering", 1, (1 << CVAR_ARCHIVE));
}

// Render Exports
void RenderExport::Initialize() {
	int nWidth = 0, nHeight = 0;
	bool bFullscreen = false;

	trap->Print(PRIORITY_NOTE, "Render Init : SDL\n");
	trap->GetRenderProperties(&nWidth, &nHeight, &bFullscreen);

	pWindow = trap->GetWindow();
	if (pWindow == nullptr) {
		trap->Print(PRIORITY_ERRFATAL, "Bad window sent to renderer\n");
		return;
	}

	renderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if (renderer == nullptr) {
		trap->Print(PRIORITY_ERRFATAL, "Failed to create SDL renderer\n");
		return;
	}

	SDL_GetRendererInfo(renderer, &renderInfo);
	if (renderInfo.flags & SDL_RENDERER_SOFTWARE) {
		trap->Print(PRIORITY_WARNING, "Using software renderer. Performance will suffer.\n");
	}

	if (!(renderInfo.flags & SDL_RENDERER_TARGETTEXTURE)) {
		trap->Print(PRIORITY_ERRFATAL, "Render-to-texture not supported");
		SDL_DestroyRenderer(renderer);
		return;
	}

	SDL_RenderSetLogicalSize(renderer, nWidth, nHeight);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_SetRenderTarget(renderer, nullptr);

	InitCvars();

	string filter = "" + trap->CvarIntegerValue(r_textureFiltering);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, filter.c_str());

	// Create render texture (this will need to be destroyed and recreated entirely if we change video dimensions)
	renderSurf = SDL_CreateRGBSurface(0, nWidth, nHeight, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	renderTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, nWidth, nHeight);

	fade.bShouldWeFade = false;

	ptText = new TextManager();

	int flags = IMG_INIT_JPG | IMG_INIT_PNG;
	trap->Print(PRIORITY_NOTE, "IMG_Init()\n");
	if ((IMG_Init(flags) & flags) != flags) {
		trap->Print(PRIORITY_ERROR, "FAILED! %s\n", IMG_GetError());
	}
}

void RenderExport::Shutdown() {
	delete ptText;

	SDL_DestroyTexture(renderTex);
	SDL_FreeSurface(renderSurf);

	SDL_DestroyRenderer(renderer);
}

void RenderExport::Restart() {
	// TODO
}

void RenderExport::WindowWidthChanged(int newWidth) {
	int nWidth = 0, nHeight = 0;
	bool bFullscreen = false;
	
	trap->GetRenderProperties(&nWidth, &nHeight, &bFullscreen);
	SDL_RenderSetLogicalSize(renderer, newWidth, nHeight);
}

void RenderExport::WindowHeightChanged(int newHeight) {
	int nWidth = 0, nHeight = 0;
	bool bFullscreen = false;

	trap->GetRenderProperties(&nWidth, &nHeight, &bFullscreen);
	SDL_RenderSetLogicalSize(renderer, nWidth, newHeight);
}

void RenderExport::ClearFrame() {
	ptText->ResetFrame();
	SDL_RenderClear(renderer);

	fade.currentTime = SDL_GetTicks();
	if (fade.fadeTime > fade.currentTime) {
		fade.bShouldWeFade = true;
	}
	else if (fade.bShouldWeFade) {
		fade.bShouldWeFade = false;
	}
}

void RenderExport::DrawActiveFrame() {
	for (auto it = qScreenshots.begin(); it != qScreenshots.end(); it++) {
		it = qScreenshots.erase(it);
	}
	SDL_RenderPresent(renderer);
}

void RenderExport::QueueScreenshot(const char *szFileName, const char *szExtension) {
	string szFullName = szFileName;
	szFullName += szExtension;

	Screenshot* newScreen = new Screenshot(szFullName.c_str());
}

void RenderExport::FadeFromBlack(int ms) {
	fade.currentTime = SDL_GetTicks();
	fade.initialFadeTime = fade.currentTime;
	fade.fadeTime = fade.initialFadeTime + ms;
}

// Main entrypoint of the program
static renderExports_s renderExport;
extern "C" {
	__declspec(dllexport) renderExports_s* GetRefAPI(renderImports_s* import) {
		trap = import;

		renderExport.Initialize = RenderExport::Initialize;
		renderExport.Shutdown = RenderExport::Shutdown;
		renderExport.Restart = RenderExport::Restart;

		renderExport.WindowWidthChanged = RenderExport::WindowWidthChanged;
		renderExport.WindowHeightChanged = RenderExport::WindowHeightChanged;

		renderExport.ClearFrame = RenderExport::ClearFrame;
		renderExport.DrawActiveFrame = RenderExport::DrawActiveFrame;

		renderExport.RegisterMaterial = Material::Register;
		renderExport.DrawMaterial = Material::DrawMaterial;
		renderExport.DrawMaterialAspectCorrection = Material::DrawMaterialAspectCorrection;
		renderExport.DrawMaterialClipped = Material::DrawMaterialClipped;
		renderExport.DrawMaterialAbs = Material::DrawMaterialAbs;
		renderExport.DrawMaterialAbsClipped = Material::DrawMaterialAbsClipped;

		renderExport.RegisterStreamingTexture = Texture::RegisterStreamingTexture;
		renderExport.LockStreamingTexture = Texture::LockStreamingTexture;
		renderExport.UnlockStreamingTexture = Texture::UnlockStreamingTexture;
		renderExport.DeleteStreamingTexture = Texture::DeleteStreamingTexture;
		renderExport.BlendTexture = Texture::BlendTexture;

		renderExport.RegisterFontAsync = TextManager::_RegisterFontAsync;
		renderExport.RenderSolidText = TextManager::_RenderTextSolid;
		renderExport.RenderShadedText = TextManager::_RenderTextShaded;
		renderExport.RenderBlendedText = TextManager::_RenderTextBlended;

		renderExport.QueueScreenshot = RenderExport::QueueScreenshot;

		renderExport.FadeFromBlack = RenderExport::FadeFromBlack;

		return &renderExport;
	}
}