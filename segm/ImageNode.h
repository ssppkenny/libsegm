//
// Created by sergey on 07.09.19.
//

#ifndef FLOW_READER_IMAGENODE_H
#define FLOW_READER_IMAGENODE_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>

class ImageNode
{

public:
    ImageNode(cv::Mat image, int x, int y)
    {
        this->image = image;
        this->left = NULL;
        this->right = NULL;
        this->x = x;
        this->y = y;
    }
    cv::Mat get_mat()
    {
        return image;
    }
    void set_left(ImageNode* left);
    void set_right(ImageNode* right);
    std::vector<ImageNode> to_vector();
    int get_x()
    {
        return x;
    };
    int get_y()
    {
        return y;
    };

    virtual ~ImageNode()
    {
        if (left != nullptr)
        {
            delete(left);
        }
        if (right != nullptr)
        {
            delete(right);
        }

    }

private:
    cv::Mat image;
    ImageNode* left;
    ImageNode* right;
    int x;
    int y;
};

#endif //FLOW_READER_IMAGENODE_H
