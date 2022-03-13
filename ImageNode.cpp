//
// Created by sergey on 07.09.19.
//

#include "ImageNode.h"


void ImageNode::set_right(ImageNode* right) {
    this->right = right;
}
void ImageNode::set_left(ImageNode* left) {
    this->left = left;
}

std::vector<ImageNode> ImageNode::to_vector() {
    std::vector<ImageNode> out_list;
    if (nullptr == left && nullptr == right) {
        std::vector<ImageNode> rv;
        rv.push_back(*this);
        return rv;
    }

    if (left != nullptr) {
        std::vector<ImageNode> rv = left->to_vector();
        out_list.insert(out_list.end(), rv.begin(), rv.end());
    }

    if (right != nullptr) {
        std::vector<ImageNode> rv = right->to_vector();
        out_list.insert(out_list.end(), rv.begin(), rv.end());
    }
    return out_list;
}
