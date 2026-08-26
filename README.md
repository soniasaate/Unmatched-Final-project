# Unmatched-Final-project

40412358022 sonia saate
40412358006 zahra akbari

Repository Link: https://github.com/soniasaate/Unmatched-Final-project.git


## 🛠️ Build & Run Instructions

### Prerequisites
- Internet connection (for downloading dependencies on first build)
- CMake 3.20+ installed

> **✅ No manual library installation is required.**  
> All dependencies (SFML, FTXUI, json) are automatically downloaded and built by CMake during the first compilation.

---

### Windows

1. Clone the repository:
    ```
   git clone https://github.com/soniasaate/Unmatched-Final-project.git
   cd Unmatched-Final-project
    ```

2. Run the build script:
   ```
   .\build_all.bat
   ```

3. Run the game:
   ```
   cd build
   unmatched_graphical.exe   # Graphical version
   unmatched_tui.exe         # Terminal UI version
   ```

> **Note:** The script will automatically install SFML via MSYS2 if needed.  
> If you get an "MSYS2 not found" error, install it from [msys2.org](https://www.msys2.org/).

---

### 🐧 Linux

1. Clone the repository:
   ```
   git clone https://github.com/soniasaate/Unmatched-Final-project.git
   cd Unmatched-Final-project
   ```

2. Make the script executable:
   ```
   chmod +x build_all.sh
   ```

3. Run the build script:
   ```
   ./build_all.sh
   ```

4. Run the game:
   ```
   cd build
   ./unmatched_graphical   # Graphical version
   ./unmatched_tui         # Terminal UI version
   ```

> **Note:** If `cmake` or `g++` is missing, install them with:
> ```bash
> sudo apt install cmake g++ make
> ```

---

### ⏱️ First Build
The first build will take **5–10 minutes** as it downloads and compiles SFML and other dependencies.

---

### 📁 Output
Executables are placed in the `build/` directory after a successful build.
```

---

Let me know if you want any tweaks! 🚀