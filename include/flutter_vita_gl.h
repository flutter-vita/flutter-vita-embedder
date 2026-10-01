// The GL platform under Ganesh, as the app provides it.
//
// The engine draws through a GLES2 it resolves by name. What owns the GXM
// context, the display chain and the swap is fvita_gles2, built from the
// tool's gles2/ into every app (ADR-0019; vitaGL was the other answer
// until 2026-09-14). The embedder calls these eight and nothing else of it;
// the tool's templates/vita-app/gl_gles2.c answers.
#ifndef FLUTTER_VITA_EMBEDDER_GL_H_
#define FLUTTER_VITA_EMBEDDER_GL_H_

#ifdef __cplusplus
extern "C" {
#endif

// Brings a context up on the panel. 0 on success. `log_line` takes one line
// without its newline, like the dev server's.
int flutter_vita_gl_init(int width, int height, void (*log_line)(const char*));

// Presents the frame. `dialog_open` says a common dialog (the IME) is up and
// has to be composited over it.
void flutter_vita_gl_swap(int dialog_open);

// An entry point by name, or NULL.
void* flutter_vita_gl_proc_address(const char* name);

// Finishes the GPU and releases the context. Once, on the way out, after the
// raster thread has been stopped.
void flutter_vita_gl_shutdown(void);

// "vitaGL" or "fvita_gles2", for the log and the HUD.
const char* flutter_vita_gl_backend(void);

// One line of the backend's own counters for the FRAMES line, or an empty
// string. fvita_gles2 reports what it holds of each memory, live textures and
// render targets, blocks waiting on the GPU, and how often a texture write or
// a copy had to wait for it. Under 250 characters.
void flutter_vita_gl_stats(char* out, int cap);

// GPU time so far, in microseconds: the sum over presented frames of the time
// from queueing the frame to the display queue taking it, which is when the
// GPU finished it. `frames` gets how many frames that covers. The vita/stats
// channel turns two readings into "GPU busy" for a HUD.
unsigned flutter_vita_gl_gpu_us(unsigned* frames);

// A function the backend runs with each finished frame, on its display
// thread, just before the frame is shown, with the frame's pixels (A8B8G8R8,
// `stride` pixels per row) open to the CPU. The debug HUD paints itself there:
// after the GPU, so nothing draws over it, and outside the GL context, so
// Skia's cached state stays true. NULL removes it.
void flutter_vita_gl_set_frame_hook(void (*hook)(void* ctx, void* pixels, int width, int height, int stride),
                                    void* ctx);

#ifdef __cplusplus
}
#endif

#endif  // FLUTTER_VITA_EMBEDDER_GL_H_
