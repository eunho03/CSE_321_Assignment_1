# CSE321 Assignment #1: B-Tree Index Structures - 20221016 Eunho Koh

## Overview
This repository contains the implementation and performance analysis of fundamental database index structures from scratch: **B-tree, B+-tree, and B*-tree**. 
The project evaluates their structural integrity, insertion/deletion overhead, and search performance (point and range queries) using 100,000 student records. 
Crucially, the fan-out order `d` is strictly configurable at runtime as per the project requirements.

## Features
- **Configurable Order (d):** The tree order is taken as standard input at runtime, allowing dynamic parameter tuning (e.g., d=3, 5, 10) without recompilation.
- **B-Tree:** Standard preemptive split and deletion with key-RID pairs in internal nodes.
- **B+-Tree:** Linked-list leaf nodes for efficient horizontal range scans and copy-up split policy.
- **B*-Tree:** 2-to-3 split policy and sibling redistribution to maintain high node utilization (~80%).
- **Performance Analytics:** Tracks logical node accesses, split/merge counts, redistribution events, and tree heights.
- **Automated Visualization:** Generates analytical plots and stacked bar charts for deletion events using Python.

## Tech Stack
- **Core Implementation:** C++11 
- **Visualization:** Python 3 (pandas, matplotlib)
- **Environment:** Windows (Tested on VS Code with g++ 6.3.0 MinGW)

## Repository Structure 
- `main.cpp`             # Experiment entry point, user input handling, and metric evaluations
- `Node.h`               # BTreeNode struct definition
- `BTree.h / .cpp`       # Base B-Tree implementation
- `BPlusTree.h / .cpp`   # B+-Tree implementation (Overrides BTree)
- `BStarTree.h / .cpp`   # B*-Tree implementation (Overrides BTree)
- `plot_results.py`      # Python script for Matplotlib visualizations
- `student.csv`          # Input dataset (100,000 records)
- `README.md`            # Project documentation

---

# Getting Started: Step-by-Step Execution Guide

Follow these step-by-step instructions to compile the C++ code, execute the parameter tuning experiments, and generate the visualizations.

# 1. Prerequisites
Ensure you have the following installed in your environment:
- GCC compiler (`g++`) supporting **C++11** or higher.
- **Python 3.x**
- The provided dataset file (`student.csv`) must be placed in the root directory of the project alongside the source files.

# 2. Compilation (C++)
Open your terminal in the repository root directory and compile the C++ source files using the following command:

g++ -std=c++11 main.cpp BTree.cpp BPlusTree.cpp BStarTree.cpp -o result

# 3. Execution (C++)
Run the compiled executable. The program will first prompt you to enter the tree order d. You must enter an integer (e.g., 3, 5, or 10) and press Enter.

## On Windows
.\result.exe

## On Linux/macOS
./result

## Execution Flow:

Input Prompt: Enter the order (d):

Type your desired order (e.g., 5) and press Enter.

The program will load the 100,000 records, execute the point/range queries, and perform deletion workloads.

Output: The metrics will be displayed on the console, and detailed trial data will be saved to a dynamically named CSV file (e.g., btree_results_d5.csv).

Note: To evaluate multiple orders as requested in the manual, simply run the executable again and input a different d value. Each run generates its own specific CSV file.

# 4. Data Visualization (Python)
To generate the performance charts, install the required Python dependencies:
## On Windows
pip install pandas matplotlib

## On Linux/macOS (Use pip3 if pip is not recognized)
pip3 install pandas matplotlib

Once the packages are installed and the C++ program has successfully created the CSV files (e.g., btree_results_d3.csv, btree_results_d5.csv, btree_results_d10.csv), run the Python script to process the results:
## On Windows
python plot_results.py

## On Linux/macOS
python3 plot_results.py

(The script will automatically read the generated CSV files and produce the corresponding analytical plots).
