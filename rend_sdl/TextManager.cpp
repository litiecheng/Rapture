#include "tr_local.h"

TextManager* ptText = nullptr;
static fontRegisteredCallback currentCallback = nullptr;

// Occurs when a font resource has finished loading
void TextManager::FontRequestCallback(AssetComponent* pComponent) {
	if (ptText != nullptr && pComponent != nullptr) {
		ComponentFont* pFont = pComponent->data.fontComponent;
		SDL_RWops* rw = SDL_RWFromMem(pFont->fontData, pComponent->meta.decompressedSize - sizeof(pFont->head));
		const char* compName = pComponent->meta.componentName;

		if (rw == nullptr) {
			trap->Print(PRIORITY_WARNING, "Couldn't read memory for font component %s\n", compName);
			return;
		}
		TTF_Font* ttfFont = TTF_OpenFontIndexRW(rw, 0, pFont->head.pointSize, pFont->head.fontFace);
		if (ttfFont == nullptr) {
			trap->Print(PRIORITY_WARNING, "Could not load font component %s (reason: %s)\n", compName, TTF_GetError());
			SDL_RWclose(rw);
			return;
		}

		TTF_SetFontStyle(ttfFont, pFont->head.style);

		ptText->umFontsRegistered[compName] = (Font*)ttfFont;
		if (currentCallback != nullptr) {
			currentCallback(compName, (Font*)ttfFont);
		}
	}
}

// Registers a font asynchronously
void TextManager::RegisterFontAsync(const char* szFontComponent, fontRegisteredCallback callback) {
	auto it = umFontsRegistered.find(szFontComponent);
	if (it == umFontsRegistered.end()) {
		// It's not there
		umFontsRegistered[szFontComponent] = nullptr;
		currentCallback = callback;
		trap->ResourceAsyncURI(szFontComponent, TextManager::FontRequestCallback);
	}
	else {
		callback(szFontComponent, it->second);
	}
}

// Renders a text surface
void TextManager::RenderTextSurface(SDL_Surface* pSurf, int x, int y) {
	SDL_Surface* surf2 = SDL_ConvertSurfaceFormat(pSurf, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf2);
	SDL_Rect dstRct{ x, y, surf2->w, surf2->h };
	SDL_RenderCopy(renderer, tex, nullptr, &dstRct);
	SDL_FreeSurface(pSurf);
	SDL_FreeSurface(surf2);

	// Cache the texture so we can kill it after this frame
	vTextFields.push_back(tex);
}

// Renders some text onto the display
void TextManager::RenderTextSolid(Font* font, const char* text, int x, int y, int r, int g, int b) {
	SDL_Color color = { r, g, b, 255 };
	SDL_Surface* surf = TTF_RenderText_Solid((TTF_Font*)font, text, color);
	if (surf == nullptr) {
		return;
	}

	RenderTextSurface(surf, x, y);
}

// Renders text with a background color onto the display
void TextManager::RenderTextShaded(Font* font, const char* text, int x, int y, int br, int bg, int bb, int fr, int fg, int fb) {
	SDL_Color cB = { br, bg, bb, 255 };
	SDL_Color cF = { fr, fg, fb, 255 };
	SDL_Surface* surf = TTF_RenderText_Shaded((TTF_Font*)font, text, cF, cB);
	if (surf == nullptr) {
		return;
	}

	RenderTextSurface(surf, x, y);
}

// Renders text which is alpha-blended onto the display.
void TextManager::RenderTextBlended(Font* font, const char* text, int x, int y, int r, int g, int b) {
	SDL_Color color = { r, g, b, 255 };
	SDL_Surface* surf = TTF_RenderText_Blended((TTF_Font*)font, text, color);
	if (surf == nullptr) {
		return;
	}

	RenderTextSurface(surf, x, y);
}

// Clears any text surfaces
void TextManager::ResetFrame() {
	for (auto it = vTextFields.begin(); it != vTextFields.end(); it++) {
		SDL_DestroyTexture(*it);
	}
	vTextFields.clear();
}

// Initializes the text manager
TextManager::TextManager() {
	TTF_Init();
}

// Shuts down the text manager.
// Since this closes the fonts for us, we don't have to worry about freeing them at any point during execution.
TextManager::~TextManager() {
	for (auto it = vTextFields.begin(); it != vTextFields.end(); it++) {
		SDL_DestroyTexture(*it);
	}
	for (auto it = umFontsRegistered.begin(); it != umFontsRegistered.begin(); ++it) {
		TTF_CloseFont((TTF_Font*)it->second);
	}
	umFontsRegistered.clear();
	vTextFields.clear();
	currentCallback = nullptr;

	TTF_Quit();
}