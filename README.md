# flutter-vita-embedder

The program that starts the Flutter engine on a PlayStation Vita. It is the
`main()` of each app that [flutter-vita](https://github.com/flutter-vita/flutter-vita)
builds. The tool compiles this source with the app's AOT snapshot, the GL
layer, the plugins and the dev server, and links all of it against the
engine archive from [flutter-vita/flutter](https://github.com/flutter-vita/flutter).

This is source, not a binary. Each app build compiles it. That is what lets
a release build remove the dev server and the debug strip: the build links
two stubs in their place.

## What is here

| Path | What it is |
| --- | --- |
| `src/vita_embedder.cc` | The embedder: engine start-up against the linked-in snapshot, the GL and software present paths, vsync, touch and pad input, the on-screen keyboard, lifecycle, platform channels, the `FRAMES`/`JANK` log lines, the watchdog, the debug strip, the cursor and the `vita/config` channel. |
| `src/embedder_version.h` | `FLUTTER_VITA_EMBEDDER_VERSION`. The first log line and `FlutterVita.info()` report it. |
| `src/vita_hud_font.h` | The strip's font: Roboto Mono at 10 px and 12 px, as bitmaps. Generated. Do not edit. |
| `src/devserver.c`, `include/devserver.h` | The on-device dev server: the TCP port an app opens in a debug build, for install, launch, logs, screenshots, taps and the handoff to the receiver. |
| `include/flutter_vita_gl.h` | The eight functions the embedder needs from the GL the app links. The tool's `gl_gles2.c` supplies them over `fvita_gles2`. |
| `CMakeLists.txt` | An OBJECT library. The app build adds it with `add_subdirectory()`. |
| `tools/gen_hud_font.py` | Regenerates `src/vita_hud_font.h` from `tools/fonts/RobotoMono-Regular.ttf`. |

## How it is built

The embedder does not build alone. It needs an engine to link against. An
app build sets `FLUTTER_VITA_ENGINE_INCLUDE` to the artifact's `include/`
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
is the reference. The tool carries this repository as a git submodule at
`engine/embedder`.

From the artifact's `include/` the embedder uses `embedder.h` (the Flutter
embedder API), `embedder_software_frame_damage.h` (the port's
partial-repaint channel) and `engine_version.h` (the revision the archive
was built from; the debug strip shows it).

## What the app supplies

Each thing the embedder needs from the app is a C symbol. A product can
replace one piece with a definition of its own and leave this source as it
is.

| Symbol | Where the tool defines it | Purpose |
| --- | --- | --- |
| `_kDart*Snapshot*` (four) | the app's `snapshot.S` from `gen_snapshot` | The AOT program. |
| `flutter_vita_gl_*` (eight) | `templates/vita-app/gl_gles2.c` | The GL under Skia. The software path is a run-time choice (`flutter_vita_force_software`, or the file `ux0:data/flutter-vita/software.on`), so all eight are always linked. |
| `dev_server_*` (five) | `src/devserver.c`, or the `release.c` stubs | The dev loop. |
| `flutter_vita_channel_call` | `templates/vita-app/plugin.c` | Platform channels to the app's Vita plugins. Weak. |
| `flutter_vita_hardware_call[_async]` | `templates/vita-app/hardware.c` | The gallery's hardware checks. Weak. |
| `_newlib_heap_size_user` | `templates/vita-app/heap.c` | The size of the C heap, for the strip's `heap` column. Weak. |

Five weak integers. A build or a package can override each with a strong
definition:

| Integer | Default | Meaning |
| --- | --- | --- |
| `flutter_vita_release` | 0 | 1 in a release build: the strip is off and `SystemNavigator.pop()` leaves the app. The tool's `release.c` sets it. |
| `flutter_vita_debug_overlay` | 1 | 1 shows the strip from the first frame, 2 keeps it hidden until L+R+SELECT, 0 never draws it. The `flutter_vita` package sets 2. |
| `flutter_vita_watchdog` | 1 | The stall watchdog. |
| `flutter_vita_force_software` | 0 | The software present path instead of GL. |
| `flutter_vita_keep_awake` | 1 | No screen dimmer and no sleep while the app runs. |

## Channels the embedder answers

| Channel | What it does |
| --- | --- |
| `vita/config` | `configure {…}`, `info`, `log`. The `flutter_vita` package's channel: the strip mode, keep-awake, the watchdog, the pixel ratio, the pad as a cursor, rear-panel scroll, the clocks, what exit does, the PS button lock, and `bounce`, a list of channels to send up to Dart. |
| `vita/bounce` | A channel in the bounce list goes to Dart as `[name length][name][message]`. The Dart reply goes back to the original response handle. |
| `flutter/platform` | `Clipboard.*` in process. `SystemNavigator.pop` and `System.exitApplication`: handoff to the receiver in a debug build, process exit in a release build. `SystemChrome.*`, `HapticFeedback.*`, `SystemSound.*`: a success that does nothing. |
| `vita/stats` | The strip's numbers, for `vita_debug_strip`. |
| `flutter/textinput` | The console's on-screen keyboard. |
| Other channels | `flutter_vita_channel_call` (the app's plugins), else an empty reply. |

## The debug strip

Each debug build draws a strip of live numbers over the top of the frame:
fps, raster and swap time, GPU busy, the four CPU cores, RAM, CDRAM, the C
heap and the battery. The CPU composes it into a 960x34 box every 20
frames. The embedder blends it over the finished frame before the flip: on
the GL path through `flutter_vita_gl_set_frame_hook`, on the software path
after the blit. [Debug an app](https://github.com/flutter-vita/flutter-vita/blob/master/docs/debug-app.md)
explains each column.

To change its font:

```sh
pip install pillow
python3 tools/gen_hud_font.py --preview strip.png
```

## Versions

Tags are `<flutter version>_<embedder version>`, for example `3.44.8_0.1.0`.
The embedder is built against one engine, and the engine against one
Flutter. A new Flutter gets a new prefix. The suffix does not restart.

## Licence

BSD 3-Clause, see [LICENSE](LICENSE). Roboto Mono is under the SIL Open Font
License 1.1 ([tools/fonts/OFL.txt](tools/fonts/OFL.txt)). Only its bitmaps
reach an app.
