# WindTunnel - 2D Lattice Boltzmann Simulation

This project is a high-performance 2D fluid flow simulation based on the **Lattice Boltzmann Method (LBM)**. It utilizes the **D2Q9 model** and is optimized for modern CPUs to provide real-time visualization of fluid dynamics.

## Key Features

* **LBM Solver:** Robust implementation of the D2Q9 lattice model using the BGK collision operator.
* **High Performance:** **OpenMP:** Multi-threaded execution for lattice collision and streaming steps.
* **Real-Time Visualization:** Integrated with the **Visualization Toolkit (VTK)**, featuring interactive UI sliders to adjust simulation parameters (like viscosity or velocity) on the fly.
* **Modern C++:** Built using the C++20 standard for clean and efficient code.

## Prerequisites & Setup

### Requirements
* **Visual Studio 2022** (or 2019) with the "Desktop development with C++" workload.
* **CMake:** Included with Visual Studio.
* **VTK (Self-Built):** This project requires a pre-built version of VTK on your system.

### Configuration in Visual Studio
Since VTK was built manually, you need to point Visual Studio to your VTK build directory:

1.  Open the `Windtunnel` folder in Visual Studio via **File -> Open -> Folder**.
2.  If CMake doesn't configure automatically, right-click `CMakeLists.txt` -> **CMake Settings for WindTunnel**.
3.  In the **CMake variables** section, add the following entry:
    * **Name:** `VTK_DIR`
    * **Type:** `PATH`
    * **Value:** The path to your VTK build folder (the directory containing `VTKConfig.cmake`).
4.  Save the settings (Ctrl + S) to regenerate the CMake cache.

## Build & Run

1.  Select the **x64-Release** configuration in the top toolbar.
2.  Select **WindTunnelRun.exe** as the startup item.
3.  Press **F5** to compile and run the simulation.


## 📂 Project Structure

* `src/`: Core solver logic (`lattice.cpp`) and VTK visualization setup (`main.cpp`).
* `include/`: Header files for the LBM algorithm and data structures (`lattice.h`, `defines.h`, etc.).
* `CMakeLists.txt`: Build configuration with optimized compiler flags (`/arch:AVX2`, `/openmp:llvm`).

