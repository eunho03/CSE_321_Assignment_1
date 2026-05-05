#ifndef BSTARTREE_H
#define BSTARTREE_H

#include "BTree.h"

class BStarTree : public BTree {
private:
    int redistributionCount;
    int twoToThreeSplitCount;
    int fallbackSplitCount;

    void insertNonFull(BTreeNode* node, int key, int rid) override;

    void splitChild(BTreeNode* parent, int index, BTreeNode* child) override;

    bool tryRedistribute(BTreeNode* parent, int index);
    void redistributeWithLeft(BTreeNode* parent, int index);
    void redistributeWithRight(BTreeNode* parent, int index);

public:
    BStarTree(int order);
    ~BStarTree();

    void insert(int key, int rid) override;

    void resetMetrics(); 

    int getRedistributionCount() const;
    int getTwoToThreeSplitCount() const;
    int getFallbackSplitCount() const;
};

#endif