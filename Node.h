#ifndef NODE_H
#define NODE_H

struct alignas(64) BTreeNode {
    int* keys;
    int* rids;
    BTreeNode** children;
    int n;
    bool isLeaf;
    BTreeNode* next;

    BTreeNode(int d, bool leaf) {
        keys = new int[2 * d - 1];
        rids = new int[2 * d - 1];
        children = new BTreeNode*[2 * d];

        for (int i = 0; i < 2 * d; i++) {
            children[i] = nullptr;
        }

        n = 0;
        isLeaf = leaf;
        next = nullptr;
    }

    ~BTreeNode() {
        delete[] keys;
        delete[] rids;
        delete[] children;
    }
};

#endif