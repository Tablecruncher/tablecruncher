<p align="center">
  <img src="assets/artwork/logo2019.png" alt="Tablecruncher logo" />
</p>

**Tablecruncher** is a blazing-fast CSV editor built to handle massive files with ease, available for macOS, Windows and Linux. 
Need to open a 2 GB file with 16 million rows? On a Mac Mini M2, Tablecruncher does it in just **32 seconds**.

Originally released in 2017 as a commercial app, Tablecruncher is now fully open source under the **GPL v3 license** (or later).


---

## Screenshot

![Tablecruncher Screenshot](assets/artwork/screenshot.png)

---

## Key Features

- **Opens huge CSV files effortlessly**
- **Built-in JavaScript macro language**
- **Supports multiple encodings**: UTF-8, UTF-16LE, UTF-16BE, Latin-1 (ISO-8859-1), and Windows 1252
- **Four color themes** to match your style

---

## Download

At the moment, only the **previous commercial version for macOS** is available as a prebuilt binary:  
**[tablecruncher.com/download](https://tablecruncher.com/download/)**

The newly open-sourced version is currently available as **source code only**.  
Prebuilt installers for macOS, Windows, and Linux will be published soon.

_Building and testing binaries across all supported platforms takes a bit of time — thanks for your patience!_

---

## Building the App

Tablecruncher is written in C++17, build files for CMake are provided. The UI toolkit **FLTK** is the only external dependency.

To build Tablecruncher from source:

1. Install **Xcode Command Line Tools** (on macOS), **Visual Studio C++** (on Windows) or your favorite build toolchain on Linux.
2. Install **CMake**
3. Download and build **FLTK**
4. Run CMake to build the application

Detailed build instructions for macOS, Windows and Linux are available in [BUILD.md](BUILD.md).

---

## Want to Support Tablecruncher?

Here’s how you can help:

- **Star this repository** — it really helps!
- **Share it** on LinkedIn, X/Twitter, Mastodon, or your favorite platform
- **Test the app and report bugs** via GitHub Issues
- **Join my newsletter**: [The Missing Header](https://missingheader.com) — a deep dive into data handling challenges and tools

---

## License & Branding

The Tablecruncher application is licensed under [GPL v3 or later](LICENSE).

All UI icons are © 2025 Stefan Fischerländer and licensed under GPL-3.0-or-later or CC BY 4.0, at your choice.
The Tablecruncher app logo is © 2025 Stefan Fischerländer and licensed only under GPL-3.0-or-later.

You are welcome to fork and build upon Tablecruncher. If you release a modified version, _please use a different name and logo_ to avoid user confusion.

---

## Third-Party Libraries Used in Tablecruncher


Tablecruncher includes the following third-party libraries:

- **[duktape](https://duktape.org/)**  
  License: MIT  
  Copyright: 2013-2023 by Duktape authors

- **[cpp-httplib](https://github.com/yhirose/cpp-httplib)**  
  License: MIT  
  Copyright: 2019-2025 by Yuji Hirose

- **[nlohmann/json](https://github.com/nlohmann/json)**  
  License: MIT  
  Copyright: 2013-2025 by Niels Lohmann

- **[utfcpp](https://github.com/nemtrif/utfcpp)**  
  License: Boost Software License 1.0  
  Copyright: 2006-2023 by Nemanja Trifunovic

Each library is included in source form under the terms of its respective license.
License texts are found in the corresponding subdirectories within [/external/](/external/), either as dedicated license files or embedded directly in the source code.

