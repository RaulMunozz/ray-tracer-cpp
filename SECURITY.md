# Security Policy

**Last reviewed:** 2026-11-24  
**Scope:** `RaulMunozz/ray-tracer-cpp` — an offline C++20 ray tracer that writes PPM frames.

## Architecture

- Standalone command-line executable; no web server, sockets, HTTP, DNS or APIs.
- No `argv`, stdin configuration, environment-variable input or config parser.
- Files: writes `frame_0.ppm` … `frame_35.ppm` to the current working directory. No file reads except the compiled program itself.
- No database, authentication, sessions, cookies, user accounts or secrets.
- Third-party build/runtime dependencies: none (C++ standard library only). `ffmpeg` is mentioned only in the README as a manual step to assemble the video.
- Build: `CMakeLists.txt`, C++20.

## Applicability of OWASP controls

SQL injection, XSS, CSRF, SSRF, broken access control, IDOR, open redirects, insecure deserialization, server-side template injection, HTTP parameter pollution, mass assignment, authentication and session controls do not apply. The program has no network surface, no user-controlled input and no stored data.

## Implemented protections

| Protection | Implementation |
|---|---|
| Write-error handling | `main.cpp` now checks the `std::ofstream` after opening each frame and after rendering, printing to `stderr` and exiting with code 1 if the file cannot be written. Previously failures were silent. |
| Build integrity | `CMakeLists.txt` now references `main.cpp`, which is the actual file in the repository root. A clean clone can now configure and build. |
| No dynamic input | No `system`, `popen`, `eval`, deserialization, plugin loading or user-controlled paths. Frame names are generated from a fixed loop index. |
| Secrets | No credentials or API keys in the repository. |

## Known limitations

- **Low — resource consumption:** rendering 36 frames at 600×600 with 100 samples/pixel is CPU- and disk-intensive. This is expected behaviour and is not attacker-controlled. Run in an empty directory and ensure free disk space.
- **Low — file overwrite:** existing `frame_*.ppm` files in the working directory are overwritten. This is unchanged from the original design.
- **Informational — numerical edge cases:** deterministic constants are used; no untrusted geometry is parsed. No raw pointer arrays or unbounded recursion.
- **Informational — toolchain:** the build was not executed in this environment (`cmake`/C++ compiler unavailable). Verification was static and via the GitHub API.
- **Informational — local copy layout:** a local reference copy keeps `src/main.cpp` and `frames/animation.mp4`; the published repository uses the root-level layout documented above.

## Secure build and use

1. Clone the repository and run `cmake -B build` / `cmake --build build`.
2. Execute the binary from an empty working directory.
3. Do not place untrusted files named `frame_*.ppm` where you do not want them overwritten.
4. Review this file if file inputs, scene files, command-line options or networking are added.

## Maintenance

- Re-run the build on a supported toolchain after changes.
- Keep `README.md`, `CMakeLists.txt` and the repository tree in sync.
- If the program ever accepts scene files or user input, perform a new audit covering parsing, path validation and resource limits.
