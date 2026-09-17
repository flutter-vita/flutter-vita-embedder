# flutter-vita-embedder

The program that starts the Flutter engine on a PlayStation Vita. It is the
`main()` of every app built with [flutter-vita](https://github.com/flutter-vita/flutter-vita):
the tool compiles this source beside the app's AOT snapshot, the GL layer, the
plugins and the dev server, and links the lot against the engine archive from
[flutter-vita/flutter](https://github.com/flutter-vita/flutter).

Source, not a binary. Every app build compiles it, which is what lets a
release build drop the dev server and the debug strip by linking two stubs
in their place.

## What is here

| Path | What it is |
| --- | --- |
| `src/vita_embedder.cc` | The embedder: engine start-up against the linked-in snapshot, the GL and software present paths, vsync, touch and pad input, the on-screen keyboard, lifecycle, platform channels, the `FRAMES`/`JANK` log lines, the watchdog and the debug strip. |
| `src/vita_hud_font.h` | The strip's font, Roboto Mono rasterised at 10 px and 12 px. Generated; do not edit. |
| `src/devserver.c`, `include/devserver.h` | The on-device dev server: the TCP port an app listens on in a debug build, for install, launch, logs, screenshots, taps and the handoff to the receiver. |
| `include/flutter_vita_gl.h` | The eight functions the embedder wants from the GL the app links. The tool's `gl_gles2.c` answers them over `fvita_gles2`. |
| `CMakeLists.txt` | An OBJECT library for the app build to add with `add_subdirectory()`. |
| `tools/gen_hud_font.py` | Regenerates `src/vita_hud_font.h` from `tools/fonts/RobotoMono-Regular.ttf`. |

## How it is built

The embedder does not build on its own; it needs an engine to link against.
An app build sets `FLUTTER_VITA_ENGINE_INCLUDE` to the artifact's `include/`
and adds this directory:

```cmake
set(FLUTTER_VITA_ENGINE_INCLUDE "${ENGINE_DIR}/include")
add_subdirectory("${EMBEDDER_DIR}" flutter_vita_embedder)
add_executable(app ${SNAPSHOT_S} ${FLUTTER_VITA_DEVSERVER_SOURCE} ...)
target_link_libraries(app flutter_vita_embedder
  -Wl,--whole-archive "${ENGINE_DIR}/lib/libflutter_engine_vita.a" -Wl,--no-whole-archive
  ...)
```

The tool's [`templates/vita-app/CMakeLists.txt`](https://github.com/flutter-vita/flutter-vita/blob/master/templates/vita-app/CMakeLists.txt)
is the reference. It carries this repository as a git submodule at
`engine/embedder`.

From the artifact's `include/` the embedder takes `embedder.h` (the Flutter
embedder API), `embedder_software_frame_damage.h` (the port's partial-repaint
channel) and `engine_version.h` (the revision the archive was built from,
shown on the debug strip).

## What the app supplies

Everything the embedder needs from the app is a C symbol, so a product can
swap a piece by defining it and leave this source alone.

| Symbol | Where the tool defines it | Purpose |
| --- | --- | --- |
| `_kDart*Snapshot*` (four) | the app's `snapshot.S` from `gen_snapshot` | The AOT program. |
| `flutter_vita_gl_*` (eight) | `templates/vita-app/gl_gles2.c` | The GL under Skia. The software path is a runtime choice (`flutter_vita_force_software`, or the file `ux0:data/flutter-vita/software.on`), so all eight are always linked. |
| `dev_server_*` (five) | `src/devserver.c`, or `release.c` stubs | The dev loop. |
| `flutter_vita_channel_call` | `templates/vita-app/plugin.c` | Platform channels to the app's Vita plugins. Weak. |
| `flutter_vita_hardware_call[_async]` | `templates/vita-app/hardware.c` | The gallery's hardware checks. Weak. |
| `_newlib_heap_size_user` | `templates/vita-app/heap.c` | The C heap's size, for the strip's `heap` column. Weak. |

And four weak integers a build may override with a strong definition:
`flutter_vita_debug_overlay` (1 shown from the first frame, 2 built in and
hidden until L+R+SELECT, 0 never), `flutter_vita_watchdog`,
`flutter_vita_force_software`, `flutter_vita_keep_awake`. The tool's
`release.c` sets the first to 0 beside its dev-server stubs.

## The debug strip

Every debug build draws a strip of live numbers over the top of the frame:
fps, raster and swap time, GPU busy, the four CPU cores, RAM, CDRAM, the C
heap and the battery. It is composed on the CPU into a 960x34 box every 20
frames and blended over the finished frame before the flip, on the GL path
through `flutter_vita_gl_set_frame_hook` and on the software path after the
blit. [Debug an app](https://github.com/flutter-vita/flutter-vita/blob/master/docs/debug-app.md)
explains each column.

To change its font:

```sh
pip install pillow
python3 tools/gen_hud_font.py --preview strip.png
```

## Versions

Tags are `<flutter version>_<embedder version>`, for instance `3.44.8_0.1.0`:
the embedder is built against one engine, and the engine against one Flutter.
A new Flutter gets a new prefix; the suffix never restarts.

## Licence

BSD 3-Clause, see [LICENSE](LICENSE). Roboto Mono is under the SIL Open Font
License 1.1 ([tools/fonts/OFL.txt](tools/fonts/OFL.txt)); only its rasterised
glyphs reach an app.
