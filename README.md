# CSE321 Assignment #1: B-Tree Index Structures - 20221016 Eunho Koh
Code for sharing including source, header, main, graphical, README files.

## Overview
This repository contains the implementation and performance analysis of fundamental database index structures from scratch: **B-tree, B+-tree, and B*-tree**. 
The project evaluates their structural integrity, insertion/deletion overhead, and search performance (point and range queries) using 100,000 student records.

## Features
- B-Tree: Standard preemptive split and deletion with key-RID pairs in internal nodes.
- B+-Tree: Linked-list leaf nodes for efficient horizontal range scans and copy-up split policy.
- B*-Tree: 2-to-3 split policy and sibling redistribution to maintain high node utilization (~80%).
- Performance Analytics: Tracks logical node accesses, split/merge counts, redistribution events, and tree heights.
- Automated Visualization: Generates analytical plots and stacked bar charts for deletion events using Python.

## Tech Stack
- C++11: Core data structures and benchmarking.
- Python 3: Data processing and visualization.
- Environment: VS Code, Windows.

## Repository Structure 
CSE321-Project-1
 ㅏ  main.cpp               # Experiment entry point and metric evaluations
 ㅏ  Node.h                 # BTreeNode struct definition
 ㅏ  BTree.h / .cpp         # Base B-Tree implementation
 ㅏ  BPlusTree.h / .cpp     # B+-Tree implementation (Overrides BTree)
 ㅏ  BStarTree.h / .cpp     # B*-Tree implementation (Overrides BTree)
 ㅏ  plot_results.py        # Python script for Matplotlib visualizations
 ㅏ  student.csv            # Input dataset (100,000 records)
 ㄴ  README.md              # Project documentation

### 0. Getting Started

Follow these step-by-step instructions to compile the C++ code, run the experiments, and generate the visualizations.

### 1. Prerequisites
Ensure you have the following installed:
- GCC compiler (`g++`) supporting C++11.
- Python 3.x
- The dataset file (`student.csv`) must be placed in the root directory of the project.

### 2. Compilation (C++)
Open your terminal and compile the C++ source files using the following 
command: g++ -std=c++11 main.cpp BTree.cpp BPlusTree.cpp BStarTree.cpp -o result

### 3. Execution (C++)
Run the compiled executable. This process will read the dataset, execute point/range queries, perform deletion workloads, and output the metrics into btree_experiment_results.csv.
command: .\result.exe

### 4. Data Visualization (Python)
To generate the performance charts, install the required Python dependencies:

command: pip install pandas matplotlib

Once the packages are installed and the btree_experiment_results.csv file is successfully created by the C++ program, run the Python script:

command: python plot_results.py
