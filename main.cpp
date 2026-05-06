#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <string>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <iomanip>
#include <cmath>

#include "BTree.h"
#include "BPlusTree.h"
#include "BStarTree.h"

using namespace std;
using namespace chrono;

struct Student {
    string id, name, gender;
    float gpa, height, weight;
};

struct Metrics {
    double insertTime, searchTime, rangeTime, deleteTime;
    int splits, insertAccesses, searchAccesses, rangeAccesses, deleteAccesses;
    double utilization;
    int foundCount, totalRangeRetrieved, deletedCount;
    int redistributions, twoToThreeSplits, fallbackSplits;
    int height, totalNodes, leafNodes, mergeCount, borrowCount;
    bool correct, structureValid;
};

vector<Student> studentData;

// Load records from CSV into an in-memory array. 
bool loadCSV(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: cannot open file " << filename << endl;
        return false;
    }

    string line, item;
    getline(file, line);

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Student s;

        getline(ss, s.id, ',');
        getline(ss, s.name, ',');
        getline(ss, s.gender, ',');
        getline(ss, item, ','); s.gpa = stof(item);
        getline(ss, item, ','); s.height = stof(item);
        getline(ss, item, ','); s.weight = stof(item);

        studentData.push_back(s);
    }
    return true;
}

double mean(const vector<double>& v) {
    double sum = 0.0;
    for (double x : v) sum += x;
    return v.empty() ? 0.0 : sum / v.size();
}

double stddev(const vector<double>& v) {
    if (v.size() <= 1) return 0.0;
    double m = mean(v), acc = 0.0;
    for (double x : v) acc += (x - m) * (x - m);
    return sqrt(acc / (v.size() - 1));
}

int countKeysInRange(const vector<int>& sortedKeys, int low, int high) {
    return static_cast<int>(upper_bound(sortedKeys.begin(), sortedKeys.end(), high) - 
                            lower_bound(sortedKeys.begin(), sortedKeys.end(), low));
}

bool validateRID(int key, int rid) {
    if (rid < 0 || rid >= static_cast<int>(studentData.size())) return false;
    return stoi(studentData[rid].id) == key;
}

struct AnalyticalResult {
    int count;
    double avgGpa, avgHeight;
};

AnalyticalResult computeAnalyticalResult(const vector<int>& rids) {
    int count = 0;
    double gpaSum = 0.0, heightSum = 0.0;
    for (int rid : rids) {
        if (rid < 0 || rid >= static_cast<int>(studentData.size())) continue;
        const Student& s = studentData[rid];
        if (s.gender == "Male") {
            count++;
            gpaSum += s.gpa;
            heightSum += s.height;
        }
    }
    return {count, (count > 0) ? gpaSum / count : 0.0, (count > 0) ? heightSum / count : 0.0};
}

template <typename TreeType>
vector<int> rangeQueryByRepeatedSearch(TreeType& tree, const vector<int>& sortedKeys, int low, int high) {
    vector<int> result;
    auto left = lower_bound(sortedKeys.begin(), sortedKeys.end(), low);
    auto right = upper_bound(sortedKeys.begin(), sortedKeys.end(), high);
    for (auto it = left; it != right; ++it) {
        int rid = -1;
        if (tree.searchRID(*it, rid)) result.push_back(rid);
    }
    return result;
}

// Verifies the structural integrity and correctness after deletions
template <typename TreeType>
bool validateDeletionCorrectness(TreeType& tree, const vector<int>& allKeys, const vector<int>& deleteKeys) {
    unordered_set<int> deletedSet(deleteKeys.begin(), deleteKeys.end());
    for (int key : allKeys) {
        int rid = -1;
        bool found = tree.searchRID(key, rid);
        if (deletedSet.count(key) && found) return false;
        if (!deletedSet.count(key) && (!found || !validateRID(key, rid))) return false;
    }
    return true;
}

Metrics evaluateBTree(int d, const vector<int>& searchKeys, const vector<int>& deleteKeys, 
                      const vector<int>& allKeys, const vector<int>& sortedKeys, int rangeQueryCount, int rangeWindow) {
    Metrics m = {0}; m.correct = true;
    BTree tree(d);

    // 1. Insertion
    tree.resetMetrics();
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < studentData.size(); i++) tree.insert(stoi(studentData[i].id), i);
    m.insertTime = duration<double>(high_resolution_clock::now() - start).count();
    m.splits = tree.getTotalSplits();
    m.insertAccesses = tree.getDiskIOs();
    m.utilization = tree.getUtilization();
    m.height = tree.getHeight();
    m.totalNodes = tree.getTotalNodes();
    m.leafNodes = tree.getLeafNodes();

    // 2. Point Search
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int key : searchKeys) {
        int beforeAccess = tree.getLogicalNodeAccesses();
        int rid = -1;
        if (tree.searchRID(key, rid) && validateRID(key, rid)) m.foundCount++;
        else m.correct = false;
        m.searchAccesses += (tree.getLogicalNodeAccesses() - beforeAccess);
    }
    m.searchTime = duration<double>(high_resolution_clock::now() - start).count();

    // 3. Range Query
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int i = 0; i < rangeQueryCount; i++) {
        int low = searchKeys[i], high = low + rangeWindow;
        vector<int> result = rangeQueryByRepeatedSearch(tree, sortedKeys, low, high);
        m.totalRangeRetrieved += result.size();
        if (static_cast<int>(result.size()) != countKeysInRange(sortedKeys, low, high)) m.correct = false;
        (void)computeAnalyticalResult(result);
    }
    m.rangeTime = duration<double>(high_resolution_clock::now() - start).count();
    m.rangeAccesses = tree.getDiskIOs();

    // 4. Deletion
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int key : deleteKeys) { tree.remove(key); m.deletedCount++; }
    m.deleteTime = duration<double>(high_resolution_clock::now() - start).count();
    m.deleteAccesses = tree.getDiskIOs();
    m.mergeCount = tree.getMergeCount();
    m.borrowCount = tree.getBorrowCount();

    if (!validateDeletionCorrectness(tree, allKeys, deleteKeys)) m.correct = false;
    m.structureValid = tree.validateStructure();
    if (!m.structureValid) m.correct = false;
    
    return m;
}

Metrics evaluateBPlusTree(int d, const vector<int>& searchKeys, const vector<int>& deleteKeys, 
                          const vector<int>& allKeys, const vector<int>& sortedKeys, int rangeQueryCount, int rangeWindow) {
    Metrics m = {0}; m.correct = true;
    BPlusTree tree(d);

    // 1. Insertion
    tree.resetMetrics();
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < studentData.size(); i++) tree.insert(stoi(studentData[i].id), i);
    m.insertTime = duration<double>(high_resolution_clock::now() - start).count();
    m.splits = tree.getTotalSplits();
    m.insertAccesses = tree.getDiskIOs();
    m.utilization = tree.getUtilization();
    m.height = tree.getHeight();
    m.totalNodes = tree.getTotalNodes();
    m.leafNodes = tree.getLeafNodes();

    // 2. Point Search
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int key : searchKeys) {
        int beforeAccess = tree.getLogicalNodeAccesses();
        int rid = -1;
        if (tree.searchRID(key, rid) && validateRID(key, rid)) m.foundCount++;
        else m.correct = false;
        m.searchAccesses += (tree.getLogicalNodeAccesses() - beforeAccess);
    }
    m.searchTime = duration<double>(high_resolution_clock::now() - start).count();

    // 3. Range Query
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int i = 0; i < rangeQueryCount; i++) {
        int low = searchKeys[i], high = low + rangeWindow;
        vector<int> result = tree.rangeQuery(low, high);
        m.totalRangeRetrieved += result.size();
        if (static_cast<int>(result.size()) != countKeysInRange(sortedKeys, low, high)) m.correct = false;
        for (int rid : result) {
            if (rid < 0 || rid >= static_cast<int>(studentData.size()) || 
                stoi(studentData[rid].id) < low || stoi(studentData[rid].id) > high) {
                m.correct = false; break;
            }
        }
        (void)computeAnalyticalResult(result); 
    }
    m.rangeTime = duration<double>(high_resolution_clock::now() - start).count();
    m.rangeAccesses = tree.getDiskIOs();

    // 4. Deletion
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int key : deleteKeys) { tree.remove(key); m.deletedCount++; }
    m.deleteTime = duration<double>(high_resolution_clock::now() - start).count();
    m.deleteAccesses = tree.getDiskIOs();
    m.mergeCount = tree.getMergeCount();
    m.borrowCount = tree.getBorrowCount();

    if (!validateDeletionCorrectness(tree, allKeys, deleteKeys)) m.correct = false;
    m.structureValid = tree.validateStructure();
    if (!m.structureValid) m.correct = false;

    //Post-deletion range query integrity check
    for (int i = 0; i < min(rangeQueryCount, static_cast<int>(searchKeys.size())); i++) {
        int low = searchKeys[i], high = low + rangeWindow;
        for (int rid : tree.rangeQuery(low, high)) {
            if (rid < 0 || rid >= static_cast<int>(studentData.size()) || 
                stoi(studentData[rid].id) < low || stoi(studentData[rid].id) > high) m.correct = false;
        }
    }
    return m;
}

Metrics evaluateBStarTree(int d, const vector<int>& searchKeys, const vector<int>& deleteKeys, 
                          const vector<int>& allKeys, const vector<int>& sortedKeys, int rangeQueryCount, int rangeWindow) {
    Metrics m = {0}; m.correct = true;
    BStarTree tree(d);

    // 1. Insertion
    //B*Tree delays splits via redistribution (2-to-3 split policy) to maintain higher utilization.
    tree.resetMetrics();
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < studentData.size(); i++) tree.insert(stoi(studentData[i].id), i);
    m.insertTime = duration<double>(high_resolution_clock::now() - start).count();
    m.splits = tree.getTotalSplits();
    m.insertAccesses = tree.getDiskIOs();
    m.utilization = tree.getUtilization();
    m.height = tree.getHeight();
    m.totalNodes = tree.getTotalNodes();
    m.leafNodes = tree.getLeafNodes();
    m.redistributions = tree.getRedistributionCount();
    m.twoToThreeSplits = tree.getTwoToThreeSplitCount();
    m.fallbackSplits = tree.getFallbackSplitCount();

    // 2. Point Search
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int key : searchKeys) {
        int beforeAccess = tree.getLogicalNodeAccesses();
        int rid = -1;
        if (tree.searchRID(key, rid) && validateRID(key, rid)) m.foundCount++;
        else m.correct = false;
        m.searchAccesses += (tree.getLogicalNodeAccesses() - beforeAccess);
    }
    m.searchTime = duration<double>(high_resolution_clock::now() - start).count();

    // 3. Range Query
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int i = 0; i < rangeQueryCount; i++) {
        int low = searchKeys[i], high = low + rangeWindow;
        vector<int> result = rangeQueryByRepeatedSearch(tree, sortedKeys, low, high);
        m.totalRangeRetrieved += result.size();
        if (static_cast<int>(result.size()) != countKeysInRange(sortedKeys, low, high)) m.correct = false;
        (void)computeAnalyticalResult(result);
    }
    m.rangeTime = duration<double>(high_resolution_clock::now() - start).count();
    m.rangeAccesses = tree.getDiskIOs();

    // 4. Deletion
    tree.resetMetrics();
    start = high_resolution_clock::now();
    for (int key : deleteKeys) { tree.remove(key); m.deletedCount++; }
    m.deleteTime = duration<double>(high_resolution_clock::now() - start).count();
    m.deleteAccesses = tree.getDiskIOs();
    m.mergeCount = tree.getMergeCount();
    m.borrowCount = tree.getBorrowCount();

    if (!validateDeletionCorrectness(tree, allKeys, deleteKeys)) m.correct = false;
    m.structureValid = tree.validateStructure();
    if (!m.structureValid) m.correct = false;
    
    return m;
}

void writeCSVHeader(ofstream& out) {
    out << "tree,order,trial,insert_time,search_time,range_time,delete_time,"
        << "splits,insert_accesses,search_accesses,range_accesses,delete_accesses,"
        << "avg_search_access,avg_range_access,avg_delete_access,"
        << "utilization,height,total_nodes,leaf_nodes,"
        << "found_count,total_range_retrieved,deleted_count,"
        << "merge_count,borrow_count,redistributions,two_to_three_splits,fallback_splits,"
        << "structure_valid,correct\n";
}

void writeCSVRow(ofstream& out, const string& treeName, int d, int trial, const Metrics& m, 
                 int queryCount, int rangeQueryCount, int deleteCount) {
    out << treeName << "," << d << "," << trial << "," << m.insertTime << "," << m.searchTime << "," 
        << m.rangeTime << "," << m.deleteTime << "," << m.splits << "," << m.insertAccesses << "," 
        << m.searchAccesses << "," << m.rangeAccesses << "," << m.deleteAccesses << "," 
        << static_cast<double>(m.searchAccesses) / queryCount << "," 
        << (rangeQueryCount > 0 ? static_cast<double>(m.rangeAccesses) / rangeQueryCount : 0.0) << "," 
        << (deleteCount > 0 ? static_cast<double>(m.deleteAccesses) / deleteCount : 0.0) << "," 
        << m.utilization << "," << m.height << "," << m.totalNodes << "," << m.leafNodes << "," 
        << m.foundCount << "," << m.totalRangeRetrieved << "," << m.deletedCount << "," 
        << m.mergeCount << "," << m.borrowCount << "," << m.redistributions << "," 
        << m.twoToThreeSplits << "," << m.fallbackSplits << "," 
        << (m.structureValid ? "true" : "false") << "," << (m.correct ? "true" : "false") << "\n";
}

void printSummary(const string& treeName, int d, const vector<Metrics>& results, int queryCount, int rangeQueryCount, bool hasRange) {
    vector<double> insertTimes, searchTimes, rangeTimes, searchAccesses, rangeAccesses, deleteTimes, deleteAccesses;
    double avgSplits = 0, avgInsertAccesses = 0, avgUtil = 0, avgHeight = 0, avgTotalNodes = 0, avgLeafNodes = 0;
    double avgRedistributions = 0, avgTwoToThreeSplits = 0, avgFallbackSplits = 0, avgMergeCount = 0, avgBorrowCount = 0;
    bool allCorrect = true, allStructureValid = true;

    for (const Metrics& m : results) {
        insertTimes.push_back(m.insertTime); searchTimes.push_back(m.searchTime); deleteTimes.push_back(m.deleteTime);
        if (hasRange) {
            rangeTimes.push_back(m.rangeTime);
            rangeAccesses.push_back(static_cast<double>(m.rangeAccesses) / rangeQueryCount);
        }
        searchAccesses.push_back(static_cast<double>(m.searchAccesses) / queryCount);
        if (m.deletedCount > 0) deleteAccesses.push_back(static_cast<double>(m.deleteAccesses) / m.deletedCount);

        avgSplits += m.splits; avgInsertAccesses += m.insertAccesses; avgUtil += m.utilization;
        avgHeight += m.height; avgTotalNodes += m.totalNodes; avgLeafNodes += m.leafNodes;
        avgRedistributions += m.redistributions; avgTwoToThreeSplits += m.twoToThreeSplits; avgFallbackSplits += m.fallbackSplits;
        avgMergeCount += m.mergeCount; avgBorrowCount += m.borrowCount;
        
        if (!m.correct) allCorrect = false;
        if (!m.structureValid) allStructureValid = false;
    }

    int n = results.size();
    avgSplits /= n; avgInsertAccesses /= n; avgUtil /= n; avgHeight /= n; avgTotalNodes /= n; avgLeafNodes /= n;
    avgRedistributions /= n; avgTwoToThreeSplits /= n; avgFallbackSplits /= n; avgMergeCount /= n; avgBorrowCount /= n;

    cout << "\n--------------------------------------------------\n" << treeName << " (d = " << d << ")\n--------------------------------------------------\n";
    cout << fixed << setprecision(6) << "Insertion\n  Time (mean +- std)   : " << mean(insertTimes) << " +- " << stddev(insertTimes) << " s\n";
    cout << fixed << setprecision(2) << "  Node splits          : " << avgSplits << "\n  Logical accesses     : " << avgInsertAccesses << "\n  Node utilization     : " << avgUtil << "%\n";
    cout << "\nStructure\n  Height               : " << avgHeight << "\n  Total nodes          : " << avgTotalNodes << "\n  Leaf nodes           : " << avgLeafNodes << "\n";
    if (treeName == "B*-Tree") cout << "\nB*-tree insertion policy\n  Redistributions      : " << avgRedistributions << "\n  2-to-3 splits        : " << avgTwoToThreeSplits << "\n  Fallback splits      : " << avgFallbackSplits << "\n";
    cout << fixed << setprecision(6) << "\nPoint search\n  Time (mean +- std)   : " << mean(searchTimes) << " +- " << stddev(searchTimes) << " s\n";
    cout << fixed << setprecision(2) << "  Time per query       : " << (mean(searchTimes) / queryCount) * 1e9 << " ns\n  Logical access/query : " << mean(searchAccesses) << "\n";
    if (hasRange) cout << fixed << setprecision(6) << "\nRange query\n  Time (mean +- std)   : " << mean(rangeTimes) << " +- " << stddev(rangeTimes) << " s\n" << fixed << setprecision(2) << "  Logical access/query : " << mean(rangeAccesses) << "\n";
    if (!deleteTimes.empty() && mean(deleteTimes) > 0.0) cout << fixed << setprecision(6) << "\nDeletion\n  Time (mean +- std)   : " << mean(deleteTimes) << " +- " << stddev(deleteTimes) << " s\n" << fixed << setprecision(2) << "  Logical access/delete: " << mean(deleteAccesses) << "\n  Merges               : " << avgMergeCount << "\n  Borrows              : " << avgBorrowCount << "\n";
    cout << "\nValidation\n  Structure invariant  : " << (allStructureValid ? "PASS" : "FAIL") << "\n  Search correctness   : " << (allCorrect ? "PASS" : "FAIL") << "\n";
}

const int DELETE_COUNT = 2000;

int main() {
    const int TRIALS = 10, QUERY_COUNT_LIMIT = 10000, RANGE_QUERY_COUNT = 1000, RANGE_WINDOW = 500;
    int d;
    cout << "Enter the order (d) for the index trees: ";
    if (!(cin >> d)) {
        cerr << "Invalid input. Please enter an integer." << endl;
        return 1;
    }

    if (!loadCSV("student.csv") || studentData.empty()) {
        cerr << "Error: no records loaded or file open failed." << endl;
        return 1;
    }

    vector<int> allKeys; allKeys.reserve(studentData.size());
    for (const Student& s : studentData) allKeys.push_back(stoi(s.id));
    
    vector<int> sortedKeys = allKeys;
    sort(sortedKeys.begin(), sortedKeys.end());
    int queryCount = min(QUERY_COUNT_LIMIT, static_cast<int>(allKeys.size()));

    cout << "==================================================\nCSE321 Assignment #1 - B-tree Index Experiments\n==================================================\n";
    cout << "Records loaded          : " << studentData.size() << "\nTrials per configuration: " << TRIALS << "\nPoint queries / trial   : " << queryCount << "\nRange queries / trial   : " << RANGE_QUERY_COUNT << "\nDeletion workload       : " << DELETE_COUNT << " keys\nAccess metric           : logical node accesses\n==================================================\n";
    string fileName = "btree_results_d" + to_string(d) + ".csv";
    ofstream csv(fileName);
    writeCSVHeader(csv);

    for (int current_d : {d}) {
        int d = current_d;
        vector<Metrics> btreeResults, bptreeResults, bstResults;

        for (int trial = 1; trial <= TRIALS; trial++) {
            mt19937 gen(1000 + trial);
            vector<int> shuffledKeys = allKeys;
            shuffle(shuffledKeys.begin(), shuffledKeys.end(), gen);

            vector<int> searchKeys(shuffledKeys.begin(), shuffledKeys.begin() + queryCount);
            vector<int> deleteKeys(shuffledKeys.begin() + queryCount, shuffledKeys.begin() + queryCount + min(DELETE_COUNT, static_cast<int>(shuffledKeys.size()) - queryCount));

            Metrics bm = evaluateBTree(d, searchKeys, deleteKeys, allKeys, sortedKeys, RANGE_QUERY_COUNT, RANGE_WINDOW);
            Metrics bpm = evaluateBPlusTree(d, searchKeys, deleteKeys, allKeys, sortedKeys, RANGE_QUERY_COUNT, RANGE_WINDOW);
            Metrics bstm = evaluateBStarTree(d, searchKeys, deleteKeys, allKeys, sortedKeys, RANGE_QUERY_COUNT, RANGE_WINDOW);

            btreeResults.push_back(bm);
            bptreeResults.push_back(bpm);
            bstResults.push_back(bstm);

            writeCSVRow(csv, "BTree", d, trial, bm, queryCount, RANGE_QUERY_COUNT, deleteKeys.size());
            writeCSVRow(csv, "BPlusTree", d, trial, bpm, queryCount, RANGE_QUERY_COUNT, deleteKeys.size());
            writeCSVRow(csv, "BStarTree", d, trial, bstm, queryCount, RANGE_QUERY_COUNT, deleteKeys.size());
        }
        cout << "\n\n==================================================\nOrder d = " << d << "\n==================================================\n";
        printSummary("B-Tree", d, btreeResults, queryCount, RANGE_QUERY_COUNT, true);
        printSummary("B+-Tree", d, bptreeResults, queryCount, RANGE_QUERY_COUNT, true);
        printSummary("B*-Tree", d, bstResults, queryCount, RANGE_QUERY_COUNT, true);
    }    
    csv.close();
    cout << "\nResults written to " << fileName << endl; 
    
    return 0;
}
