#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include "BTree.h"
#include <vector>

class BPlusTree : public BTree {
private:
    BTreeNode* firstLeaf; 

    void splitChild(BTreeNode* parent, int i, BTreeNode* y) override; 
    void insertNonFull(BTreeNode* node, int key, int rid) override;  

    bool searchRID(BTreeNode* node, int key, int& outRid);

    bool removeRecursive(BTreeNode* node, int key);
    void rebalanceAfterDelete(BTreeNode* parent, int childIndex);

    void borrowFromPrevLeaf(BTreeNode* parent, int childIndex);
    void borrowFromNextLeaf(BTreeNode* parent, int childIndex);
    void mergeLeaf(BTreeNode* parent, int childIndex);

    void borrowFromPrevInternal(BTreeNode* parent, int childIndex);
    void borrowFromNextInternal(BTreeNode* parent, int childIndex);
    void mergeInternal(BTreeNode* parent, int childIndex);

    int getSubtreeMinKey(BTreeNode* node);
    void refreshInternalKeys(BTreeNode* node);

public:
    BPlusTree(int order);
    ~BPlusTree();

    void insert(int key, int rid) override;
    void remove(int key) override;
    bool searchRID(int key, int& outRid) override;

    std::vector<int> rangeQuery(int startKey, int endKey);
    
    bool validateStructure() const override;
};

#endif