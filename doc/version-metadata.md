# Build version metadata

Building `generate_version` (also a dependency of the `version_info` library) produces
`source/version_info/version_info.cpp`, `version_def.h` and `package-version.json` in the
build directory. One invocation evaluates the version, UTC clock and Git state for all
three files. `configure_file` preserves each output's timestamp when its contents are
unchanged. All three files are declared build byproducts.

Selection follows the existing precedence:

1. An installer build (`ROR_BUILD_INSTALLER` other than `Off`) uses `PROJECT_VERSION`,
   overriding custom and development versions.
2. Otherwise, a defined `ROR_CUSTOM_VERSION` supplies the exact display version.
3. Otherwise, `ROR_BUILD_DEV_VERSION=ON` selects UTC `year.month-dev-<short revision>`,
   appending `-dirty` for tracked changes, or `-dev-without-git` when Git is unavailable.
4. An ordinary release uses `PROJECT_VERSION`. Previously this case had no selection
   branch and generated `.` for both version strings and empty resource integers.

Custom versions must start with two decimal components separated by a dot, such as
`2040.08-preview`. Invalid prefixes fail with a diagnostic. The complete suffix is
preserved, including spaces, quotes, backslashes, semicolons and control characters.
CMake cannot represent NUL. The target writes custom text as raw data at configure
time and reads it during generation; it is never inserted into executable CMake code
or interpreted as a generator expression. The generated C++, Windows RC and JSON
escape strings independently. C++ question marks use fixed-width octal escapes to prevent C++11 trigraph conversion. The resource
header selects RC literals when `RC_INVOKED` is defined, otherwise C++ literals.
RC embedded quotes are doubled, with escaped backslashes and fixed-width octal
controls for the narrow resource string. These rules follow
[Microsoft's resource string syntax](https://learn.microsoft.com/en-us/windows/win32/menurc/stringtable-resource)
and [LLVM's resource string processing](https://github.com/llvm/llvm-project/blob/main/llvm/tools/llvm-rc/ResourceFileWriter.cpp).
The Windows resource integers are decimal without leading zeros; display strings
retain their original formatting.

## JSON schema 1

| Field | Meaning |
| --- | --- |
| `schema` | Integer `1` |
| `project_version` | Raw CMake project version, e.g. `2026.01` |
| `display_version` | Exact full version exposed by C++ and the resource header |
| `short_version` | The selected `year.month` prefix exposed by C++ |
| `full_revision` | Full 40-character Git revision, or JSON `null` when unavailable |
| `dirty` | Boolean tracked-change status, or `null` when unknown |
| `custom_version` | Selected custom string, including the project version for installer builds, otherwise `null` |
| `build_dev_version` | Boolean: the development branch was selected; false when custom or installer selection takes precedence |
| `build_date`, `build_time` | One UTC snapshot formatted as `YYYY-MM-DD` and `HH:MM` |
| `apple_short_version`, `apple_bundle_version` | Normalized numeric project major/minor/patch, with absent components zero-filled |

Git metadata is collected in the explicitly supplied source directory for every mode,
including custom and release builds. The short revision and tracked-change query use
the captured full revision. As before, dirty status ignores untracked files. A failed
tracked-change query gives `null`, without claiming a dirty or clean tree.

Both Apple fields derive only from the project version: `2026.01` becomes `2026.1.0`.
For example, custom display `2040.08-preview` changes the C++/header/JSON display while
both numeric fields remain `2026.1.0`. A fourth project version component remains in
`project_version` and release display, but is not part of the three Apple components.
There is no new numeric override option. Consumers must validate their platform's
component limits and any distribution build-number policy; portable builds impose no
new Apple-specific numeric limits.

CMake's `SOURCE_DATE_EPOCH` environment support controls the single timestamp when
provided. Without it, each generation uses the current UTC clock, including the date
portion of development versions. Pinned sources and dependencies alone do not promise
byte-identical builds. Packaging should consume this completed build metadata without
scraping C++ or running Git/date again, and should reject unknown schema versions.

The script also retains the original `VERSION_FILE_INPUT`/`VERSION_FILE_OUTPUT`
interface with `BUILD_DEV_VERSION`, `BUILD_CUSTOM_VERSION`, `CUSTOM_VERSION` and
`GIT_EXECUTABLE`. Direct multi-output callers supply `PROJECT_VERSION`,
`VERSION_SOURCE_DIR`, `VERSION_INFO_INPUT_DIR` and `VERSION_INFO_OUTPUT_DIR`.
`CUSTOM_VERSION_FILE`, when supplied for a custom build, takes precedence over
`CUSTOM_VERSION`. Production generation needs CMake 3.16 APIs and no new runtime tool.

## Focused verification

Run from the repository root:

```sh
python3 tests/version_info/test_version_info.py -v
```

Python 3's standard library, Git, CMake and a C++ compiler are test prerequisites.
Ninja is selected when available; otherwise CMake's default generator is used.
`--work-dir /absolute/fixture-directory` retains the fixture sources and build outputs.
The tests run the production script and unmodified production target/templates with a
tiny fixture root project. They compile only the version library and check generated
string syntax under C++11, covering release/dev/custom/installer precedence, fixed
epoch, Git and no-Git cases, normalization, escaping, all trigraph spellings, snapshot
command counts and unchanged output modification times. Resource assertions use the
RC grammar documented above, independently of the C++ literal decoder; they check
doubled quotes, backslashes and controls against exact expected text. They do not
establish compilation by a native RC tool. They do not configure or launch the game.
