Rigs of Rods test harness
=========================

This directory (/test) contains projects which perform automated checking of RoR functionality.
This is important because RoR is feature-rich and involves a great number of community-created mods, so it must remain backwards-compatible.

Terminology
-----------

'Test' means single `TEST()` macro, 'test suite' is a project containing multiple tests.
See also https://github.com/google/googletest/blob/master/googlemock/docs/ForDummies.md

We try avoiding term 'unit test' because we mostly test more complex scenarios involving content loading/processing and gameplay simulation.
See also https://www.leadingagile.com/2018/11/whats-the-scope-of-a-unit-test/

'Fakes' are manually-created dummy implementations of RoR functionality and use actual includes from RoR source;
'mocks' are made using googletest and they are stand-alone.

Guidelines
----------

The test harness (including these guidelines) is experimental and subject to breaking changes.

Tests are written using https://github.com/google/googletest framework and built as regular projects using CMake.
However, our primary method of isolating code for testing are hand-created fakes (see Terminology)

Dependencies are managed the same way as in user-facing projects, see [/source/main/CMakeLists.txt](/source/main/CMakeLists.txt) (look for `target_link_libraries()`)

### Test source tree description:

* '/test/fakes'   - Fakes as header files (named "*Fake.h", for example "InputEngineFake.h") shared between test suites. These always include actual RoR header and optionally (`#ifdef ROR_FAKES_IMPL`) also generate implementations.
* '/test/mocks'   - Mocks as header files (named "*Mock.h", for example "TerrainManagerMock.h") shared between test suites. These define the whole class (using googlemock) and are stand-alone.
* '/test/proxies' - Replacements of actual RoR headers (with same names) which link to fakes/mocks or carry custom code. Shared between test suites.
* '/test/suites'  - Individual projects for testing specific area
* '/test/suites/fakes_impl.inl' - Helper; `#include` it to your test suite to generate implementations for fakes.

### Include path management:

1. Test suite directory comes first, so any include can be overriden there.
2. Proxies directory comes second, these are general-purpose proxies.
3. Actual RoR source directories - usually '/source/main' for ForwardDeclarations.h and Application.h

### Organizing tests

Tests are grouped to testcases depending on class/file you use as entry point, no matter what other classes/files come to play.
Example: When you're testing actor spawning and your entry point is `SimController::QueueActorSpawn`, then the test belongs to 'SimControllerTest'.

Following syntax is recommended for test name: {SCOPE}\_{SYMBOL}\_{ACTION}
* SCOPE: 'Func/Method' (unit test of single function/method) or 'Feat' (feature test, involves multiple subsystems)
* SYMBOL: Name of the function/method you're using as entry point
* ACTION: Intended operation to be performed, possibly also argument values used to archieve it
