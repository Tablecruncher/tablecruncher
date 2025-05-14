# Release Workflow

With version 1.8, Tablecruncher uses a simple MAJOR.MINOR versioning scheme for user-facing version numbers.

Internally, especially for CMake, we use this PATCH number scheme:
PATCH < 10      → beta
PATCH == 10     → final
PATCH > 10      → post-final (e.g. bugfixes) with .x suffix


## Workflow

1. Update version in `CMakeLists.txt`, e.g. `1.8.1` ("1.8-beta1") or `1.8.10` ("1.8")
2. Build the binaries  
2.1 macOS: call the sign script with the version `../../scripts/sign-and-notarize.sh 1.8-beta1`
2.2 Windows: set `AppVersion` in `win-setup.iss` and 
3. Test the binaries locally
4. Commit changes:
    ```
    git commit -am "Publish new beta version"
    git log -1	# optionally, to get the commit hash: e.g. abc1234
    git tag v1.8-beta1 abc1234
    git push && git push --tags
    ```
5. Release the binaries
