#include "BPlusTree.h"
#include <iostream>
#include <algorithm>

using namespace std;

BPlusTree::BPlusTree(int order) : BTree(order) {
    firstLeaf = nullptr; 
}

BPlusTree::~BPlusTree() {
}

//Point Search: B+ Tree always descends to the leaf node.
bool BPlusTree::searchRID(int key, int& outRid) {
    return searchRID(root, key, outRid);
}

bool BPlusTree::searchRID(BTreeNode* node, int key, int& outRid) {
    if (node == nullptr) return false;
    countNodeAccess();

    if (node->isLeaf) {
        int pos = std::distance(node->keys, std::lower_bound(node->keys, node->keys + node->n, key));
        if (pos < node->n && node->keys[pos] == key) {
            outRid = node->rids[pos];
            return true;
        }
        return false;
    }

    // Route search using upper_bound
    int i = std::distance(node->keys, std::upper_bound(node->keys, node->keys + node->n, key));
    return searchRID(node->children[i], key, outRid);
}

void BPlusTree::insert(int key, int rid) {
    if (root == nullptr) {
        root = new BTreeNode(d, true);
        root->keys[0] = key;
        root->rids[0] = rid;
        root->n = 1;
        firstLeaf = root;
        return;
    }

    if (root->n == 2 * d - 1) {
        BTreeNode* s = new BTreeNode(d, false);
        s->children[0] = root;
        splitChild(s, 0, root);

        int i = 0;
        if (key >= s->keys[0]) i++;

        diskIOs++;
        insertNonFull(s->children[i], key, rid);
        root = s;
    } else {
        insertNonFull(root, key, rid);
    }
}

void BPlusTree::insertNonFull(BTreeNode* node, int key, int rid) {
    if (node->isLeaf) {
        int pos = std::distance(node->keys, std::lower_bound(node->keys, node->keys + node->n, key));

        if (pos < node->n && key == node->keys[pos]) {
            node->rids[pos] = rid; 
            return;
        }

        for (int j = node->n - 1; j >= pos; j--) {
            node->keys[j + 1] = node->keys[j];
            node->rids[j + 1] = node->rids[j];
        }

        node->keys[pos] = key;
        node->rids[pos] = rid;
        node->n++;
    } else {
        int i = std::distance(node->keys, std::upper_bound(node->keys, node->keys + node->n, key));

        if (node->children[i]->n == 2 * d - 1) {
            diskIOs++;
            splitChild(node, i, node->children[i]);
            if (key >= node->keys[i]) i++;
        }

        diskIOs++;
        insertNonFull(node->children[i], key, rid);
    }
}

//Split Policy: Copy-up for leaf nodes vs Push-up for internal nodes
void BPlusTree::splitChild(BTreeNode* parent, int i, BTreeNode* y) {
    totalSplits++;
    BTreeNode* z = new BTreeNode(d, y->isLeaf);

    if (y->isLeaf) {
        // [1] Leaf Node: Copy-up policy and maintain linked list
        z->n = d; 
        for (int j = 0; j < d; j++) {
            z->keys[j] = y->keys[j + d - 1];
            z->rids[j] = y->rids[j + d - 1];
        }
        y->n = d - 1;

        z->next = y->next;
        y->next = z;

        for (int j = parent->n; j >= i + 1; j--) parent->children[j + 1] = parent->children[j];
        parent->children[i + 1] = z;

        for (int j = parent->n - 1; j >= i; j--) parent->keys[j + 1] = parent->keys[j];

        parent->keys[i] = z->keys[0];
        parent->n++;

    } else {
        // [2] Internal Node: Standard Push-up policy (same as B-Tree)
        z->n = d - 1;
        for (int j = 0; j < d - 1; j++) z->keys[j] = y->keys[j + d];
        for (int j = 0; j < d; j++) z->children[j] = y->children[j + d];
        y->n = d - 1;

        for (int j = parent->n; j >= i + 1; j--) parent->children[j + 1] = parent->children[j];
        parent->children[i + 1] = z;

        for (int j = parent->n - 1; j >= i; j--) parent->keys[j + 1] = parent->keys[j];

        parent->keys[i] = y->keys[d - 1];
        parent->n++;
    }
}

//Range Query: vertical descent followed by horizontal leaf traversal via next pointers.
std::vector<int> BPlusTree::rangeQuery(int startKey, int endKey) {
    std::vector<int> result;
    if (root == nullptr) return result;

    // 1. Vertical descent to find the starting leaf
    BTreeNode* current = root;
    while (!current->isLeaf) {
        diskIOs++;
        int i = std::distance(current->keys, std::upper_bound(current->keys, current->keys + current->n, startKey));
        current = current->children[i];
    }
    diskIOs++; // Access to the target leaf

    // 2. Horizontal scan across the leaf chain
    while (current != nullptr) {
        for (int i = 0; i < current->n; i++) {
            if (current->keys[i] >= startKey && current->keys[i] <= endKey) {
                result.push_back(current->rids[i]);
            } 
            else if (current->keys[i] > endKey) {
                return result;
            }
        }
        current = current->next;
        if (current != nullptr) diskIOs++; 
    }

    return result;
}

void BPlusTree::remove(int key) {
    if (root == nullptr) return;

    removeRecursive(root, key);

    if (root->n == 0) {
        BTreeNode* oldRoot = root;
        if (root->isLeaf) {
            root = nullptr;
            firstLeaf = nullptr;
        } else {
            root = root->children[0];
            oldRoot->children[0] = nullptr;
        }
        delete oldRoot;
    }

    // Refresh internal separator keys to reflect actual minimums in leaves
    if (root != nullptr) refreshInternalKeys(root);
}

//Deletion: Actual removal occurs ONLY at leaves. Internal nodes are just routing separators.
bool BPlusTree::removeRecursive(BTreeNode* node, int key) {
    if (node == nullptr) return false;
    countNodeAccess();

    if (node->isLeaf) {
        int pos = std::distance(node->keys, std::lower_bound(node->keys, node->keys + node->n, key));
        if (pos >= node->n || node->keys[pos] != key) return false;

        for (int i = pos + 1; i < node->n; i++) {
            node->keys[i - 1] = node->keys[i];
            node->rids[i - 1] = node->rids[i];
        }
        node->n--;
        return true;
    }

    int childIndex = std::distance(node->keys, std::upper_bound(node->keys, node->keys + node->n, key));
    bool removed = removeRecursive(node->children[childIndex], key);

    if (!removed) return false;

    if (node->children[childIndex] != nullptr && node->children[childIndex]->n < d - 1) {
        rebalanceAfterDelete(node, childIndex);
    }

    //Internal separator keys must be refreshed to ensure routing accuracy
    refreshInternalKeys(node);
    return true;
}

void BPlusTree::rebalanceAfterDelete(BTreeNode* parent, int childIndex) {
    BTreeNode* child = parent->children[childIndex];
    if (child == nullptr) return;

    if (childIndex > 0 && parent->children[childIndex - 1]->n > d - 1) {
        if (child->isLeaf) borrowFromPrevLeaf(parent, childIndex);
        else borrowFromPrevInternal(parent, childIndex);
    } else if (childIndex < parent->n && parent->children[childIndex + 1]->n > d - 1) {
        if (child->isLeaf) borrowFromNextLeaf(parent, childIndex);
        else borrowFromNextInternal(parent, childIndex);
    } else {
        if (childIndex > 0) {
            if (child->isLeaf) mergeLeaf(parent, childIndex - 1);
            else mergeInternal(parent, childIndex - 1);
        } else {
            if (child->isLeaf) mergeLeaf(parent, childIndex);
            else mergeInternal(parent, childIndex);
        }
    }
}

void BPlusTree::borrowFromPrevLeaf(BTreeNode* parent, int childIndex) {
    borrowCount++;
    BTreeNode* child = parent->children[childIndex];
    BTreeNode* sibling = parent->children[childIndex - 1];

    for (int i = child->n - 1; i >= 0; i--) {
        child->keys[i + 1] = child->keys[i];
        child->rids[i + 1] = child->rids[i];
    }

    child->keys[0] = sibling->keys[sibling->n - 1];
    child->rids[0] = sibling->rids[sibling->n - 1];

    child->n++;
    sibling->n--;
    parent->keys[childIndex - 1] = child->keys[0];
}

void BPlusTree::borrowFromNextLeaf(BTreeNode* parent, int childIndex) {
    borrowCount++;
    BTreeNode* child = parent->children[childIndex];
    BTreeNode* sibling = parent->children[childIndex + 1];

    child->keys[child->n] = sibling->keys[0];
    child->rids[child->n] = sibling->rids[0];
    child->n++;

    for (int i = 1; i < sibling->n; i++) {
        sibling->keys[i - 1] = sibling->keys[i];
        sibling->rids[i - 1] = sibling->rids[i];
    }

    sibling->n--;
    if (sibling->n > 0) parent->keys[childIndex] = sibling->keys[0];
}

void BPlusTree::mergeLeaf(BTreeNode* parent, int childIndex) {
    mergeCount++;
    BTreeNode* left = parent->children[childIndex];
    BTreeNode* right = parent->children[childIndex + 1];

    for (int i = 0; i < right->n; i++) {
        left->keys[left->n + i] = right->keys[i];
        left->rids[left->n + i] = right->rids[i];
    }

    left->n += right->n;
    left->next = right->next;

    for (int i = childIndex + 1; i < parent->n; i++) parent->keys[i - 1] = parent->keys[i];
    for (int i = childIndex + 2; i <= parent->n; i++) parent->children[i - 1] = parent->children[i];

    parent->children[parent->n] = nullptr;
    parent->n--;

    right->next = nullptr;
    delete right;
}

void BPlusTree::borrowFromPrevInternal(BTreeNode* parent, int childIndex) {
    borrowCount++;
    BTreeNode* child = parent->children[childIndex];
    BTreeNode* sibling = parent->children[childIndex - 1];

    for (int i = child->n; i >= 0; i--) child->children[i + 1] = child->children[i];
    child->children[0] = sibling->children[sibling->n];
    sibling->children[sibling->n] = nullptr;

    child->n++;
    sibling->n--;

    refreshInternalKeys(sibling);
    refreshInternalKeys(child);
    refreshInternalKeys(parent);
}

void BPlusTree::borrowFromNextInternal(BTreeNode* parent, int childIndex) {
    borrowCount++;
    BTreeNode* child = parent->children[childIndex];
    BTreeNode* sibling = parent->children[childIndex + 1];

    child->children[child->n + 1] = sibling->children[0];
    for (int i = 1; i <= sibling->n; i++) sibling->children[i - 1] = sibling->children[i];
    sibling->children[sibling->n] = nullptr;

    child->n++;
    sibling->n--;

    refreshInternalKeys(sibling);
    refreshInternalKeys(child);
    refreshInternalKeys(parent);
}

void BPlusTree::mergeInternal(BTreeNode* parent, int childIndex) {
    mergeCount++;
    BTreeNode* left = parent->children[childIndex];
    BTreeNode* right = parent->children[childIndex + 1];

    int leftChildCount = left->n + 1;
    int rightChildCount = right->n + 1;

    for (int i = 0; i < rightChildCount; i++) {
        left->children[leftChildCount + i] = right->children[i];
        right->children[i] = nullptr;
    }

    left->n = leftChildCount + rightChildCount - 1;
    refreshInternalKeys(left);

    for (int i = childIndex + 1; i < parent->n; i++) parent->keys[i - 1] = parent->keys[i];
    for (int i = childIndex + 2; i <= parent->n; i++) parent->children[i - 1] = parent->children[i];

    parent->children[parent->n] = nullptr;
    parent->n--;

    delete right;
    refreshInternalKeys(parent);
}

//Refreshes internal routing keys using the true minimum of each subtree
int BPlusTree::getSubtreeMinKey(BTreeNode* node) {
    BTreeNode* current = node;
    while (current != nullptr && !current->isLeaf) current = current->children[0];
    return (current == nullptr || current->n == 0) ? -1 : current->keys[0];
}

void BPlusTree::refreshInternalKeys(BTreeNode* node) {
    if (node == nullptr || node->isLeaf) return;
    for (int i = 0; i < node->n; i++) {
        node->keys[i] = getSubtreeMinKey(node->children[i + 1]);
    }
}

// Validation: Ensures monotonic leaf chain structure
bool BPlusTree::validateStructure() const {
    if (root == nullptr) return firstLeaf == nullptr;

    BTreeNode* current = firstLeaf;
    int prevKey = -1;
    bool hasPrev = false;

    while (current != nullptr) {
        if (!current->isLeaf || current->n < 0 || current->n > 2 * d - 1) return false;

        for (int i = 1; i < current->n; i++) {
            if (current->keys[i - 1] > current->keys[i]) return false;
        }

        for (int i = 0; i < current->n; i++) {
            if (hasPrev && prevKey > current->keys[i]) return false;
            prevKey = current->keys[i];
            hasPrev = true;
        }
        current = current->next;
    }
    return true;
}