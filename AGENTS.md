# obs-cd-rec-status – notes for AI agents

An OBS Studio frontend plugin (C++17, Qt6, built from the official OBS plugin template). It adds a dock called "CD Rec Status" that shows the state of the ongoing CD recording. The status comes from a WebSocket at `ws://localhost:8765` (hardcoded), which is served by `slidegen`. In production it runs in the **OBS Flatpak** (`com.obsproject.Studio`, stable branch) on the livestreaming PC of the Mennonitengemeinde Basel-Holee.

## Layout

- `cd-rec-status.cpp` → the whole plugin: the `CDStatusDock` QWidget, a websocketpp client (standalone asio) on its own `std::thread`, reconnecting every 3 s, and JSON parsing via `QJsonDocument`. `obs_module_load()` registers the dock with `obs_frontend_add_custom_qdock`.
- `CMakeLists.txt` → fetches `asio` (asio-1-30-2) and `websocketpp` (0.8.2) via `FetchContent`, links `OBS::libobs`, `OBS::obs-frontend-api`, Qt6. The project version is set here.
- `cmake/`, `CMakePresets.json`, `buildspec.json` → mostly unchanged plugin template boilerplate. Its metadata (`plugintemplate-for-obs`, `Your Name Here`) was never filled in.
- `data/locale/en-US.ini` → locale strings (currently empty).
- `.github/workflows/release.yml` → **the actual build**. A pushed tag `v*` builds the plugin as a Flatpak extension against the `com.obsproject.Studio` runtime (SDK 25.08), copies `cd-rec-status.so` and `data/` into `cd-rec-status/{bin/64bit,data}`, sets the rpath to `$ORIGIN` and publishes `cd-rec-status-linux-x86_64.tar.gz` as a GitHub release.
- `.gitignore` uses a **whitelist** (`/*`, then `!…`). Any new top-level file or directory must be added there, otherwise git silently ignores it.

## Conventions

- **Threading:** websocketpp handlers run on the client thread. Every UI change must be marshalled to the Qt thread with `QMetaObject::invokeMethod(this, …, Qt::QueuedConnection)`, as the existing handlers do. Never touch widgets from the WebSocket thread.
- Log with `blog(LOG_…, "[cd-rec-status] …")`.
- Format C++ with `.clang-format` (clang-format ≥ 16) and CMake with `.gersemirc` (gersemi).
- The README's "Building" section (in-tree build inside the OBS sources) is outdated. The release workflow is the reference build.
- There are no tests. A local build needs the OBS Flatpak SDK, like in the workflow. A real check means loading the plugin into OBS while the `slidegen` WebSocket (or a small fake server that sends the JSON below) is running.

## Interface: status JSON

The plugin reads the keys `recording` (bool), `cd` (int), `track` (int), `cd_time` and `track_time` (`"MM:SS"` strings), sent periodically by `slidegen`'s `cd_recording_status_websocket.py`. If `recording` is false, only an "inactive" text is shown. Changing keys, types, the port or the host means changing `slidegen` too.

## Deployment and safety

- The plugin is **not deployed by Ansible**. On the PC it was unpacked by hand from the release tarball into `~/.var/app/com.obsproject.Studio/config/obs-studio/plugins/cd-rec-status/`. A new version means a new tag/release and then replacing that directory by hand.
- If the OBS Flatpak runtime moves to a new major version, the plugin may need a rebuild (new release).
- Don't create tags, trigger releases or replace the plugin on the PC (`ssh livestream@192.168.71.139`) without asking. OBS is live during the service.

## Sibling repos

On the dev machine, the related repos are checked out next to this one in `~/dox/src/` (`../<name>/`).

| Sibling | Interface with this repo |
| :--- | :--- |
| `slidegen` | Serves the WebSocket: `cd_recording_status_websocket.py`, host/port/interval in `config/default_config.py` (`CD_RECORD_WEBSOCKET_*`), and the status values come from the CD recording cachefile (`recording/verify.py`). |
| `livestreaming-pc-infra` | Runs the WebSocket server as the systemd unit `cd_recording_status_websocket.service` and installs the OBS Flatpak, but not this plugin. |
| `technik-dokumentation` | Full technical documentation (German). Read its `AGENTS.md` (section map) first, then only the relevant sections of `home.md`, never the whole file. Most relevant: `### Die automatische CD-Aufnahme und Marker-Setzung`. |

Rules:

* When changing the JSON contract, port or host, check `slidegen` and make the matching change, or explicitly say what needs to change there.
* Read siblings selectively, never whole repos.
* If a sibling is not checked out, say so instead of guessing its interface.

## Workflow

* **NEVER commit or push.** Leave all git history operations to the user.
* When a change is done, suggest a commit message following Conventional Commits,
  **unscoped** (`feat!: ...`, `feat: ...`, `fix: ...`, `chore: ...`, `doc: ...`, `refactor: ...`),
  lowercase, imperative, matching `git log`. If the change spans several repos, suggest
  one message per repo.
