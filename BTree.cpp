#include "BTree.h"
#include <iostream>
#include <algorithm>
#include <limits>

using namespace std;

BTree::BTree(int order) {
    d = order;
    root = nullptr;
    totalSplits = diskIOs = logicalNodeAccesses = mergeCount = borrowCount = 0;
}

BTree::~BTree() {
    clear(root);
    root = nullptr;
}

void BTree::clear(BTreeNode* node) {
    if (node == nullptr) return;
    if (!node->isLeaf) {
        for (int i = 0; i <= node->n; i++) clear(node->children[i]);
    }
    delete node;
}

void BTree::countNodeAccess() {
    logicalNodeAccesses++;
    diskIOs = logicalNodeAccesses;
}

void BTree::resetMetrics() {
    totalSplits = diskIOs = logicalNodeAccesses = mergeCount = borrowCount = 0;
}

int BTree::getTotalSplits() const { return totalSplits; }
int BTree::getDiskIOs() const { return diskIOs; }
int BTree::getLogicalNodeAccesses() const { return logicalNodeAccesses; }
int BTree::getMergeCount() const { return mergeCount; }
int BTree::getBorrowCount() const { return borrowCount; }

// Point Search: In a standard B-Tree, search can terminate at internal nodes.
bool BTree::searchRID(int key, int& outRid) {
    return searchRID(root, key, outRid);
}

bool BTree::searchRID(BTreeNode* node, int key, int& outRid) {
    if (node == nullptr) return false;
    countNodeAccess();

    int i = std::distance(node->keys, std::lower_bound(node->keys, node->keys + node->n, key));

    // Match found at current node
    if (i < node->n && key == node->keys[i]) {
        outRid = node->rids[i];
        return true;
    }

    if (node->isLeaf) return false;
    return searchRID(node->children[i], key, outRid);
}

bool BTree::updateRID(BTreeNode* node, int key, int newRid) {
    if (node == nullptr) return false;
    countNodeAccess();

    int i = std::distance(node->keys, std::lower_bound(node->keys, node->keys + node->n, key));

    if (i < node->n && key == node->keys[i]) {
        node->rids[i] = newRid;
        return true;
    }

    if (node->isLeaf) return false;
    return updateRID(node->children[i], key, newRid);
}

// Preemptive Split Policy: Splits full nodes on the way down to avoid bottom-up cascading splits.
void BTree::insert(int key, int rid) {
    if (root == nullptr) {
        root = new BTreeNode(d, true);
        root->keys[0] = key;
        root->rids[0] = rid;
        root->n = 1;
        countNodeAccess();
        return;
    }

    countNodeAccess();

    if (root->n == 2 * d - 1) {
        BTreeNode* s = new BTreeNode(d, false);
        s->children[0] = root;
        splitChild(s, 0, root);

        int i = 0;
        if (key > s->keys[0]) i++;
        else if (key == s->keys[0]) {
            s->rids[0] = rid;
            root = s;
            return;
        }

        insertNonFull(s->children[i], key, rid);
        root = s;
    } else {
        insertNonFull(root, key, rid);
    }
}

void BTree::insertNonFull(BTreeNode* node, int key, int rid) {
    if (node == nullptr) return;
    countNodeAccess();

    int pos = std::distance(node->keys, std::lower_bound(node->keys, node->keys + node->n, key));

    if (pos < node->n && key == node->keys[pos]) {
        node->rids[pos] = rid;
        return;
    }

    if (node->isLeaf) {
        for (int j = node->n - 1; j >= pos; j--) {
            node->keys[j + 1] = node->keys[j];
            node->rids[j + 1] = node->rids[j];
        }
        node->keys[pos] = key;
        node->rids[pos] = rid;
        node->n++;
    } else {
        if (node->children[pos]->n == 2 * d - 1) {
            splitChild(node, pos, node->children[pos]);
            if (key == node->keys[pos]) {
                node->rids[pos] = rid;
                return;
            }
            if (key > node->keys[pos]) pos++;
        }
        insertNonFull(node->children[pos], key, rid);
    }
}

// Standard 1-to-2 Split Promotes the middle key to the parent node.
void BTree::splitChild(BTreeNode* parent, int i, BTreeNode* y) {
    totalSplits++;
    int mid = d - 1;
    BTreeNode* z = new BTreeNode(d, y->isLeaf);
    z->n = d - 1;

    for (int j = 0; j < d - 1; j++) {
        z->keys[j] = y->keys[j + d];
        z->rids[j] = y->rids[j + d];
    }

    if (!y->isLeaf) {
        for (int j = 0; j < d; j++) z->children[j] = y->children[j + d];
    }

    y->n = d - 1;

    for (int j = parent->n; j >= i + 1; j--) parent->children[j + 1] = parent->children[j];
    parent->children[i + 1] = z;

    for (int j = parent->n - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->rids[j + 1] = parent->rids[j];
    }

    parent->keys[i] = y->keys[mid];
    parent->rids[i] = y->rids[mid];
    parent->n++;
}

void BTree::calculateNodeStats(BTreeNode* node, int& totalKeys, int& totalNodes) {
    if (node == nullptr) return;
    totalNodes++;
    totalKeys += node->n;
    if (!node->isLeaf) {
        for (int i = 0; i <= node->n; i++) calculateNodeStats(node->children[i], totalKeys, totalNodes);
    }
}

double BTree::getUtilization() {
    if (root == nullptr) return 0.0;
    int totalKeys = 0, totalNodes = 0;
    calculateNodeStats(root, totalKeys, totalNodes);
    double maxPossibleKeys = static_cast<double>(totalNodes) * (2 * d - 1);
    return maxPossibleKeys == 0.0 ? 0.0 : (static_cast<double>(totalKeys) / maxPossibleKeys * 100.0);
}

void BTree::remove(int key) {
    if (root == nullptr) return;
    remove(root, key);

    if (root->n == 0) {
        BTreeNode* oldRoot = root;
        if (root->isLeaf) root = nullptr;
        else {
            root = root->children[0];
            oldRoot->children[0] = nullptr;
        }
        delete oldRoot;
    }
}

int BTree::calculateHeight(BTreeNode* node) const {
    if (node == nullptr) return 0;
    if (node->isLeaf) return 1;
    return 1 + calculateHeight(node->children[0]);
}

void BTree::calculateNodeCounts(BTreeNode* node, int& totalNodes, int& leafNodes) const {
    if (node == nullptr) return;
    totalNodes++;
    if (node->isLeaf) {
        leafNodes++;
        return;
    }
    for (int i = 0; i <= node->n; i++) calculateNodeCounts(node->children[i], totalNodes, leafNodes);
}

int BTree::getHeight() const { return calculateHeight(root); }
int BTree::getTotalNodes() const { int t = 0, l = 0; calculateNodeCounts(root, t, l); return t; }
int BTree::getLeafNodes() const { int t = 0, l = 0; calculateNodeCounts(root, t, l); return l; }

size_t BTree::calculateMemoryUsage(BTreeNode* node) const {
    if (node == nullptr) return 0;
    size_t memory = sizeof(BTreeNode) + sizeof(int) * (2 * d - 1) * 2 + sizeof(BTreeNode*) * (2 * d);
    if (!node->isLeaf) {
        for (int i = 0; i <= node->n; i++) memory += calculateMemoryUsage(node->children[i]);
    }
    return memory;
}

size_t BTree::getTotalMemoryUsage() const { return calculateMemoryUsage(root); }

// Deletion Routing & Internal Key Replacements
void BTree::remove(BTreeNode* node, int key) {
    if (node == nullptr) return;
    countNodeAccess();
    int idx = findKey(node, key);

    if (idx < node->n && node->keys[idx] == key) {
        if (node->isLeaf) removeFromLeaf(node, idx);
        else removeFromInternal(node, idx);
    } else {
        if (node->isLeaf) return;
        bool isLastChild = (idx == node->n);

        // Underflow handling before descending
        if (node->children[idx]->n < d) fill(node, idx);

        if (isLastChild && idx > node->n) remove(node->children[idx - 1], key);
        else remove(node->children[idx], key);
    }
}

int BTree::findKey(BTreeNode* node, int key) {
    int idx = 0;
    while (idx < node->n && node->keys[idx] < key) idx++;
    return idx;
}

void BTree::removeFromLeaf(BTreeNode* node, int index) {
    for (int i = index + 1; i < node->n; i++) {
        node->keys[i - 1] = node->keys[i];
        node->rids[i - 1] = node->rids[i];
    }
    node->n--;
}

void BTree::removeFromInternal(BTreeNode* node, int index) {
    int key = node->keys[index];

    if (node->children[index]->n >= d) {
        int predKey = getPredecessorKey(node, index);
        int predRid = getPredecessorRID(node, index);
        node->keys[index] = predKey;
        node->rids[index] = predRid;
        remove(node->children[index], predKey);
    } else if (node->children[index + 1]->n >= d) {
        int succKey = getSuccessorKey(node, index);
        int succRid = getSuccessorRID(node, index);
        node->keys[index] = succKey;
        node->rids[index] = succRid;
        remove(node->children[index + 1], succKey);
    } else {
        merge(node, index);
        remove(node->children[index], key);
    }
}

int BTree::getPredecessorKey(BTreeNode* node, int index) {
    BTreeNode* current = node->children[index];
    while (!current->isLeaf) {
        countNodeAccess();
        current = current->children[current->n];
    }
    return current->keys[current->n - 1];
}

int BTree::getPredecessorRID(BTreeNode* node, int index) {
    BTreeNode* current = node->children[index];
    while (!current->isLeaf) current = current->children[current->n];
    return current->rids[current->n - 1];
}

int BTree::getSuccessorKey(BTreeNode* node, int index) {
    BTreeNode* current = node->children[index + 1];
    while (!current->isLeaf) {
        countNodeAccess();
        current = current->children[0];
    }
    return current->keys[0];
}

int BTree::getSuccessorRID(BTreeNode* node, int index) {
    BTreeNode* current = node->children[index + 1];
    while (!current->isLeaf) current = current->children[0];
    return current->rids[0];
}

void BTree::fill(BTreeNode* node, int index) {
    if (index != 0 && node->children[index - 1]->n >= d) borrowFromPrev(node, index);
    else if (index != node->n && node->children[index + 1]->n >= d) borrowFromNext(node, index);
    else {
        if (index != node->n) merge(node, index);
        else merge(node, index - 1);
    }
}

// Underflow Handling: Borrowing from siblings
void BTree::borrowFromPrev(BTreeNode* node, int index) {
    borrowCount++;
    BTreeNode* child = node->children[index];
    BTreeNode* sibling = node->children[index - 1];

    for (int i = child->n - 1; i >= 0; i--) {
        child->keys[i + 1] = child->keys[i];
        child->rids[i + 1] = child->rids[i];
    }
    if (!child->isLeaf) {
        for (int i = child->n; i >= 0; i--) child->children[i + 1] = child->children[i];
    }

    child->keys[0] = node->keys[index - 1];
    child->rids[0] = node->rids[index - 1];

    if (!child->isLeaf) {
        child->children[0] = sibling->children[sibling->n];
        sibling->children[sibling->n] = nullptr;
    }

    node->keys[index - 1] = sibling->keys[sibling->n - 1];
    node->rids[index - 1] = sibling->rids[sibling->n - 1];

    child->n++;
    sibling->n--;
}

void BTree::borrowFromNext(BTreeNode* node, int index) {
    borrowCount++;
    BTreeNode* child = node->children[index];
    BTreeNode* sibling = node->children[index + 1];

    child->keys[child->n] = node->keys[index];
    child->rids[child->n] = node->rids[index];

    if (!child->isLeaf) child->children[child->n + 1] = sibling->children[0];

    node->keys[index] = sibling->keys[0];
    node->rids[index] = sibling->rids[0];

    for (int i = 1; i < sibling->n; i++) {
        sibling->keys[i - 1] = sibling->keys[i];
        sibling->rids[i - 1] = sibling->rids[i];
    }

    if (!sibling->isLeaf) {
        for (int i = 1; i <= sibling->n; i++) sibling->children[i - 1] = sibling->children[i];
        sibling->children[sibling->n] = nullptr;
    }

    child->n++;
    sibling->n--;
}

// Underflow Handling: Merging two siblings and a separator key
void BTree::merge(BTreeNode* node, int index) {
    mergeCount++;
    BTreeNode* child = node->children[index];
    BTreeNode* sibling = node->children[index + 1];

    child->keys[d - 1] = node->keys[index];
    child->rids[d - 1] = node->rids[index];

    for (int i = 0; i < sibling->n; i++) {
        child->keys[i + d] = sibling->keys[i];
        child->rids[i + d] = sibling->rids[i];
    }

    if (!child->isLeaf) {
        for (int i = 0; i <= sibling->n; i++) {
            child->children[i + d] = sibling->children[i];
            sibling->children[i] = nullptr;
        }
    }

    for (int i = index + 1; i < node->n; i++) {
        node->keys[i - 1] = node->keys[i];
        node->rids[i - 1] = node->rids[i];
    }

    for (int i = index + 2; i <= node->n; i++) node->children[i - 1] = node->children[i];
    node->children[node->n] = nullptr;

    child->n += sibling->n + 1;
    node->n--;
    delete sibling;
}

// Structural Integrity Validation (Invariants checking)
static bool validateNode(BTreeNode* node, int d, int minKey, int maxKey, int depth, int& leafDepth) {
    if (!node) return true;

    if (node->n < 0 || node->n > 2 * d - 1) return false;
    if (depth != 0 && node->n < d - 1) return false;

    for (int i = 1; i < node->n; i++) {
        if (node->keys[i - 1] > node->keys[i]) return false; 
    }

    for (int i = 0; i < node->n; i++) {
        if (node->keys[i] <= minKey || node->keys[i] >= maxKey) return false; 
    }

    if (node->isLeaf) {
        if (leafDepth == -1) leafDepth = depth;
        else if (leafDepth != depth) return false; 

        for (int i = 0; i <= node->n; i++) {
            if (node->children[i] != nullptr) return false;
        }
        return true;
    }

    for (int i = 0; i <= node->n; i++) {
        if (node->children[i] == nullptr) return false;
    }

    for (int i = 0; i <= node->n; i++) {
        int newMin = (i == 0) ? minKey : node->keys[i - 1];
        int newMax = (i == node->n) ? maxKey : node->keys[i];
        if (!validateNode(node->children[i], d, newMin, newMax, depth + 1, leafDepth)) return false;
    }

    return true;
}

bool BTree::validateStructure() const {
    if (root == nullptr) return true;
    int leafDepth = -1;
    return validateNode(root, d, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), 0, leafDepth);
}