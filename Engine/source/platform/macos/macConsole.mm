//-----------------------------------------------------------------------------
// macConsole.mm — macOS console logging.
//
// Genuinely new. The original macOS build compiled POSIXConsole.cpp
// directly (StdConsole) via macMain.mm's Platform::init(), which is
// exactly the "mac shares code with the POSIX/Linux layer" coupling this
// rewrite has been removing — and, per the original diagnosis of the
// double-logging bug, is the actual root cause: Torque's core console
// (Con::_printf) already writes every line to stdout by default on every
// platform; StdConsole ALSO echoed every line it received via write() on
// top of that, on the assumption it was the only thing doing terminal
// output. Enabling it unconditionally meant every line appeared twice —
// once from the core console's own stdout write, once from StdConsole's
// echo.
//
// Fix (same principle as the Linux and Windows consoles): this file adds
// no consumer of its own, and Platform::init() (macProcessControl.mm)
// does not create or enable StdConsole at all. macOS gets nothing extra
// layered on top of the core console's own stdout output — which is
// already sufficient for a normal run. NSLog is deliberately NOT used as
// a substitute logging path either, since it writes to both stderr AND
// the unified system log (Console.app), and combined with the core
// console's own stdout write would reintroduce a form of the exact same
// double-output problem.
//
// If an interactive stdin console (arrow-key history, tab completion —
// what StdConsole actually provided beyond plain logging) is ever wanted
// for a headless/dedicated macOS build, it should be implemented natively
// here rather than pulling in the POSIX StdConsole class, mirroring how
// linuxConsole.cpp's interactive front-end is opt-in and Linux-only
// rather than auto-enabled.
//-----------------------------------------------------------------------------
