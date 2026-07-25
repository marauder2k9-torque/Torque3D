//-----------------------------------------------------------------------------
// macFontInit.mm — createFontInit()/createFontShutdown() for macOS.
//
// Genuinely new. macFont.mm's CoreText-based OSXFont never needed any
// shared setup/teardown state the way Windows' GDI-based WinFont did
// (which needed a shared HDC/HBITMAP pair created in createFontInit() and
// torn down in createFontShutdown() — see win32Font.cpp) — CoreText fonts
// are self-contained per-instance (CTFontRef, created and released by
// each OSXFont individually). But createFontInit()/createFontShutdown()
// are called unconditionally by the engine's own startup/shutdown
// sequence regardless of platform, so macOS needs to define them even
// though there's nothing for them to actually do.
//-----------------------------------------------------------------------------
void createFontInit()
{
    // Nothing to do — see file header comment.
}

void createFontShutdown()
{
    // Nothing to do — see file header comment.
}
