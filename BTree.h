#ifndef BTREE_H
#define BTREE_H

#include "Node.h"
#include <cstddef>

class BTree {
protected:
    int d;
    BTreeNode* root;

    int totalSplits, mergeCount, borrowCount;
    int diskIOs, logicalNodeAccesses;

    void clear(BTreeNode* node);
    void countNodeAccess();

    bool searchRID(BTreeNode* node, int key, int& outRid);
    bool updateRID(BTreeNode* node, int key, int newRid);

    virtual void insertNonFull(BTreeNode* node, int key, int rid);
    virtual void splitChild(BTreeNode* parent, int index, BTreeNode* child);

    void remove(BTreeNode* node, int key);
    int findKey(BTreeNode* node, int key);
    void removeFromLeaf(BTreeNode* node, int index);
    void removeFromInternal(BTreeNode* node, int index);
    void fill(BTreeNode* node, int index);
    void borrowFromPrev(BTreeNode* node, int index);
    void borrowFromNext(BTreeNode* node, int index);
    void merge(BTreeNode* node, int index);

    int getPredecessorKey(BTreeNode* node, int index);
    int getPredecessorRID(BTreeNode* node, int index);
    int getSuccessorKey(BTreeNode* node, int index);
    int getSuccessorRID(BTreeNode* node, int index);

    void calculateNodeStats(BTreeNode* node, int& totalKeys, int& totalNodes);
    int calculateHeight(BTreeNode* node) const;
    void calculateNodeCounts(BTreeNode* node, int& totalNodes, int& leafNodes) const;
    size_t calculateMemoryUsage(BTreeNode* node) const;

public:
    BTree(int order);
    virtual ~BTree();

    virtual bool searchRID(int key, int& outRid);
    virtual void insert(int key, int rid);
    virtual void remove(int key);

    void resetMetrics();
    int getTotalSplits() const;
    int getDiskIOs() const;
    int getLogicalNodeAccesses() const;
    int getMergeCount() const;
    int getBorrowCount() const;
    size_t getTotalMemoryUsage() const;
    
    double getUtilization();
    int getHeight() const;
    int getTotalNodes() const;
    int getLeafNodes() const;
    virtual bool validateStructure() const;
};

#endif