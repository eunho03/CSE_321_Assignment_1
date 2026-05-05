#include "BStarTree.h"
#include <algorithm>
#include <vector>

using namespace std;

BStarTree::BStarTree(int order) : BTree(order) {
    redistributionCount = twoToThreeSplitCount = fallbackSplitCount = 0;
}

BStarTree::~BStarTree() {}

void BStarTree::resetMetrics() {
    BTree::resetMetrics();
    redistributionCount = twoToThreeSplitCount = fallbackSplitCount = 0;
}

int BStarTree::getRedistributionCount() const { return redistributionCount; }
int BStarTree::getTwoToThreeSplitCount() const { return twoToThreeSplitCount; }
int BStarTree::getFallbackSplitCount() const { return fallbackSplitCount; }

//Insertion: Handles root specifically
void BStarTree::insert(int key, int rid) {
    if (root == nullptr) {
        root = new BTreeNode(d, true);
        root->keys[0] = key; root->rids[0] = rid; root->n = 1;
        countNodeAccess();
        return;
    }

    countNodeAccess();

    if (root->n == 2 * d - 1) {
        BTreeNode* newRoot = new BTreeNode(d, false);
        newRoot->children[0] = root;
        // Root has no sibling
        splitChild(newRoot, 0, root);
        root = newRoot;
    }

    insertNonFull(root, key, rid);
}

// Internal Node Strategy
void BStarTree::insertNonFull(BTreeNode* node, int key, int rid) {
    if (node == nullptr) return;
    countNodeAccess();

    int pos = std::distance(node->keys, std::lower_bound(node->keys, node->keys + node->n, key));

    if (pos < node->n && node->keys[pos] == key) {
        node->rids[pos] = rid; 
        return;
    }

    if (node->isLeaf) {
        for (int i = node->n - 1; i >= pos; i--) {
            node->keys[i + 1] = node->keys[i];
            node->rids[i + 1] = node->rids[i];
        }
        node->keys[pos] = key;
        node->rids[pos] = rid;
        node->n++;
        return;
    }

    int childIndex = pos;

    if (node->children[childIndex]->n == 2 * d - 1) {
        if (!tryRedistribute(node, childIndex)) {
            splitChild(node, childIndex, node->children[childIndex]);
        }
        childIndex = std::distance(node->keys, std::upper_bound(node->keys, node->keys + node->n, key));
    }

    insertNonFull(node->children[childIndex], key, rid);
}

//Sibling Redistribution 
bool BStarTree::tryRedistribute(BTreeNode* parent, int index) {
    int maxKeys = 2 * d - 1;

    if (index > 0 && parent->children[index - 1] != nullptr && parent->children[index - 1]->n <= maxKeys - 2) {
        redistributeWithLeft(parent, index);
        redistributionCount++;
        return true;
    }

    if (index < parent->n && parent->children[index + 1] != nullptr && parent->children[index + 1]->n <= maxKeys - 2) {
        redistributeWithRight(parent, index);
        redistributionCount++;
        return true;
    }

    return false;
}

// Redistribution Helper
void BStarTree::redistributeWithLeft(BTreeNode* parent, int index) {
    BTreeNode* child = parent->children[index];
    BTreeNode* left = parent->children[index - 1];

    left->keys[left->n] = parent->keys[index - 1];
    left->rids[left->n] = parent->rids[index - 1];
    if (!child->isLeaf) left->children[left->n + 1] = child->children[0];
    left->n++;

    parent->keys[index - 1] = child->keys[0];
    parent->rids[index - 1] = child->rids[0];

    for (int i = 1; i < child->n; i++) {
        child->keys[i - 1] = child->keys[i];
        child->rids[i - 1] = child->rids[i];
    }

    if (!child->isLeaf) {
        for (int i = 1; i <= child->n; i++) child->children[i - 1] = child->children[i];
        child->children[child->n] = nullptr;
    }
    child->n--;
}

// Redistribution Helper
void BStarTree::redistributeWithRight(BTreeNode* parent, int index) {
    BTreeNode* child = parent->children[index];
    BTreeNode* right = parent->children[index + 1];

    for (int i = right->n - 1; i >= 0; i--) {
        right->keys[i + 1] = right->keys[i];
        right->rids[i + 1] = right->rids[i];
    }

    if (!right->isLeaf) {
        for (int i = right->n; i >= 0; i--) right->children[i + 1] = right->children[i];
        right->children[0] = child->children[child->n];
        child->children[child->n] = nullptr;
    }

    right->keys[0] = parent->keys[index];
    right->rids[0] = parent->rids[index];
    right->n++;

    parent->keys[index] = child->keys[child->n - 1];
    parent->rids[index] = child->rids[child->n - 1];
    child->n--;
}

// 2-to-3 Split
void BStarTree::splitChild(BTreeNode* parent, int index, BTreeNode* child) {
    int maxKeys = 2 * d - 1;

    if (parent->n == 0) { 
        fallbackSplitCount++;
        BTree::splitChild(parent, index, child);
        return;
    }

    bool useRight = false;
    int leftIndex, sepIndex;

    if (index < parent->n && parent->children[index + 1] != nullptr) {
        useRight = true; leftIndex = index; sepIndex = index;
    } else if (index > 0 && parent->children[index - 1] != nullptr) {
        useRight = false; leftIndex = index - 1; sepIndex = index - 1;
    } else {
        fallbackSplitCount++;
        BTree::splitChild(parent, index, child);
        return;
    }

    BTreeNode* left = parent->children[leftIndex];
    BTreeNode* middle = parent->children[leftIndex + 1];

    // Redistribution must have failed, meaning both siblings are full
    if (left->n != maxKeys || middle->n != maxKeys) {
        fallbackSplitCount++;
        BTree::splitChild(parent, index, child);
        return;
    }

    totalSplits++;
    twoToThreeSplitCount++;

    BTreeNode* right = new BTreeNode(d, left->isLeaf);

    vector<int> keys; vector<int> rids; vector<BTreeNode*> children;
    keys.reserve(4 * d); rids.reserve(4 * d); children.reserve(4 * d + 1);

    // Combine left node, parent separator, and middle node into temporary vectors
    for (int i = 0; i < left->n; i++) { keys.push_back(left->keys[i]); rids.push_back(left->rids[i]); }
    keys.push_back(parent->keys[sepIndex]); rids.push_back(parent->rids[sepIndex]);
    for (int i = 0; i < middle->n; i++) { keys.push_back(middle->keys[i]); rids.push_back(middle->rids[i]); }

    if (!left->isLeaf) {
        for (int i = 0; i <= left->n; i++) children.push_back(left->children[i]);
        for (int i = 0; i <= middle->n; i++) children.push_back(middle->children[i]);
    }

    // Determine the optimal distribution layout
    int totalKeys = static_cast<int>(keys.size());
    int leftKeys = totalKeys / 3;
    int remainingAfterLeftSep = totalKeys - leftKeys - 1;
    int middleKeys = remainingAfterLeftSep / 2;
    int sep1Index = leftKeys;
    int sep2Index = leftKeys + 1 + middleKeys;
    int rightKeys = totalKeys - sep2Index - 1;

    // Safety fallback
    if (leftKeys > maxKeys || middleKeys > maxKeys || rightKeys > maxKeys) {
        fallbackSplitCount++;
        BTree::splitChild(parent, index, child);
        delete right;
        return;
    }

    // 1. Distribute to left node
    left->n = leftKeys;
    for (int i = 0; i < leftKeys; i++) { left->keys[i] = keys[i]; left->rids[i] = rids[i]; }

    // 2. Distribute to middle node
    middle->n = middleKeys;
    for (int i = 0; i < middleKeys; i++) { middle->keys[i] = keys[sep1Index + 1 + i]; middle->rids[i] = rids[sep1Index + 1 + i]; }

    // 3. Distribute to right node
    right->n = rightKeys;
    for (int i = 0; i < rightKeys; i++) { right->keys[i] = keys[sep2Index + 1 + i]; right->rids[i] = rids[sep2Index + 1 + i]; }

    if (!left->isLeaf) {
        int childCursor = 0;
        for (int i = 0; i <= leftKeys; i++) left->children[i] = children[childCursor++];
        for (int i = 0; i <= middleKeys; i++) middle->children[i] = children[childCursor++];
        for (int i = 0; i <= rightKeys; i++) right->children[i] = children[childCursor++];
        
        for (int i = leftKeys + 1; i < 2 * d; i++) left->children[i] = nullptr;
        for (int i = middleKeys + 1; i < 2 * d; i++) middle->children[i] = nullptr;
        for (int i = rightKeys + 1; i < 2 * d; i++) right->children[i] = nullptr;
    }

    // Replace old separator & insert the new separator into the parent
    parent->keys[sepIndex] = keys[sep1Index];
    parent->rids[sepIndex] = rids[sep1Index];

    for (int i = parent->n - 1; i >= sepIndex + 1; i--) {
        parent->keys[i + 1] = parent->keys[i];
        parent->rids[i + 1] = parent->rids[i];
    }

    parent->keys[sepIndex + 1] = keys[sep2Index];
    parent->rids[sepIndex + 1] = rids[sep2Index];

    for (int i = parent->n; i >= sepIndex + 2; i--) parent->children[i + 1] = parent->children[i];

    parent->children[sepIndex] = left;
    parent->children[sepIndex + 1] = middle;
    parent->children[sepIndex + 2] = right;
    parent->n++;
}