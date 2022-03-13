//
// Created by sergey on 01.09.19.
//

#include "Xycut.h"
#include <iostream>
#include <vector>

#include "common.h"


Output Xycut::image_without_white_borders(cv::Mat& img) {
    cv::Mat hist;
    //horizontal
    reduce(img, hist, 0, cv::REDUCE_SUM, CV_32F);
    std::vector<cv::Point> locations;
    cv::findNonZero(hist, locations);


    int left = locations.at(0).x;
    int right = locations.at(locations.size()-1).x;
    //vertical
    reduce(img, hist, 1, cv::REDUCE_SUM, CV_32F);
    cv::findNonZero(hist, locations);

    int upper = locations.at(0).y;
    int lower = locations.at(locations.size()-1).y;

    cv::Rect rect(left, upper, right-left, lower-upper);

    Output out;
    out.image = img(rect);
    out.x = left;
    out.y = upper;
    return out;
}
void Xycut::xycut_vertical_find(cv::Mat img, double threshold, int level, int height, int width, std::vector<int>& lst){
    if (level == 3) {
        return;
    }
    cv::Size s = img.size();
    int h = s.height;
    int w = s.width;

    cv::Mat hist;
    reduce(img, hist, 0, cv::REDUCE_SUM, CV_32F);
    std::vector<std::tuple<int,int>> zr = zero_runs(hist);

    std::vector<std::tuple<int,int>> zr1;
    for (int i=0;i<zr.size();i++){
        int f = std::get<0>(zr.at(i));
        int t = std::get<1>(zr.at(i));
        if (t-f>threshold){
            zr1.push_back(zr.at(i));
        }
    }

    if (zr1.size() > 0) {
        int maxind = max_ind(zr1);
        int _from = std::get<0>(zr1.at(maxind));
        int _to = std::get<1>(zr1.at(maxind));
        lst.push_back(_to - _from);
        cv::Rect left_rect(0,0,_from,h);
        cv::Rect right_rect(_to,0,w-_to,h);
        cv::Mat left_image = img(left_rect);
        cv::Size s = left_image.size();
        int h = s.height;
        int w = s.width;
        if (h > height && w > 1) {
            xycut_horizontal_find(left_image, threshold, 1, height, width, lst);
        }
        cv::Mat right_image = img(right_rect);
        s = right_image.size();
        h = s.height;
        w = s.width;

        if (h>height && w >1) {
            xycut_vertical_find(right_image, threshold, 1, height, width, lst);
        }
        return;
    }

    xycut_horizontal_find(img, threshold, level+1, height, width, lst);
}
void Xycut::xycut_horizontal_find(cv::Mat img, double threshold, int level, int height, int width, std::vector<int>& lst){
    if (level == 3) {
        return;
    }
    cv::Size s = img.size();
    int h = s.height;
    int w = s.width;

    cv::Mat hist;
    reduce(img, hist, 1, cv::REDUCE_SUM, CV_32F);
    std::vector<std::tuple<int,int>> zr = zero_runs(hist);

    std::vector<std::tuple<int,int>> zr1;
    for (int i=0;i<zr.size();i++){
        int f = std::get<0>(zr.at(i));
        int t = std::get<1>(zr.at(i));
        if (t-f>threshold){
            zr1.push_back(zr.at(i));
        }
    }

    if (zr1.size() > 0 && w > width / 2) {
        int maxind = max_ind(zr1);
        int _from = std::get<0>(zr1.at(maxind));
        int _to = std::get<1>(zr1.at(maxind));
        lst.push_back(_to - _from);
        cv::Rect upper_rect(0, 0, w, _from);
        cv::Rect lower_rect(0, _to, w, h-_to);
        cv::Mat upper_image = img(upper_rect);
        cv::Size s = upper_image.size();
        int h = s.height;
        int w = s.width;
        if (h > height && w > 1) {
            xycut_vertical_find(upper_image, threshold, 1, height, width, lst);
        }
        cv::Mat lower_image = img(lower_rect);
        s = lower_image.size();
        h = s.height;
        w = s.width;
        if (h > height && w > 1)  {
            xycut_horizontal_find(lower_image, threshold, 1, height, width, lst);
        }
        return;
    }
    xycut_vertical_find(img, threshold, level+1, height, width, lst);
}

void Xycut::xycut_vertical_cut(cv::Mat img, double threshold, ImageNode* tree, int level,  int height, int width){
    if (level == 3) {
        return;
    }
    cv::Size s = img.size();
    int h = s.height;
    int w = s.width;

    cv::Mat hist;
    reduce(img, hist, 0, cv::REDUCE_SUM, CV_32F);
    std::vector<std::tuple<int,int>> zr = zero_runs(hist);

    std::vector<std::tuple<int,int>> zr1;
    for (int i=0;i<zr.size();i++){
        int f = std::get<0>(zr.at(i));
        int t = std::get<1>(zr.at(i));
        if (t-f>threshold){
            zr1.push_back(zr.at(i));
        }
    }

    if (zr1.size() > 0 && w > width / 2) {
        int maxind = max_ind(zr1);
        int _from = std::get<0>(zr1.at(maxind));
        int _to = std::get<1>(zr1.at(maxind));
        cv::Rect left_rect(0,0,_from,h);
        cv::Rect right_rect(_to,0,w-_to,h);
        cv::Mat left_image = img(left_rect);
        cv::Size s = left_image.size();
        int h = s.height;
        int w = s.width;
        if (h > height && w > 1) {
            ImageNode lt(left_image, tree->get_x(), tree->get_y());
            ImageNode *left_tree = new ImageNode(lt);
            tree->set_left(left_tree);
            xycut_horizontal_cut(left_image, threshold, left_tree, 1, height, width);
        }
        cv::Mat right_image = img(right_rect);
        s = right_image.size();
        h = s.height;
        w = s.width;

        if (h>height && w >1) {
            ImageNode rt(right_image, _to + tree->get_x(), tree->get_y());
            ImageNode* right_tree = new ImageNode(rt);
            tree->set_right(right_tree);
            xycut_vertical_cut(right_image, threshold, right_tree, 1, height, width);
        }
        return;
    }

    xycut_horizontal_cut(img, threshold, tree, level+1, height, width);
};


void Xycut::xycut_horizontal_cut(cv::Mat img, double threshold, ImageNode* tree, int level, int height, int width){
    if (level == 3) {
        return;
    }
    cv::Size s = img.size();
    int h = s.height;
    int w = s.width;

    cv::Mat hist;
    reduce(img, hist, 1, cv::REDUCE_SUM, CV_32F);
    std::vector<std::tuple<int,int>> zr = zero_runs(hist);

    std::vector<std::tuple<int,int>> zr1;
    for (int i=0;i<zr.size();i++){
        int f = std::get<0>(zr.at(i));
        int t = std::get<1>(zr.at(i));
        if (t-f>threshold){
            zr1.push_back(zr.at(i));
        }
    }

    if (zr1.size() > 0) {
        int maxind = max_ind(zr1);
        int _from = std::get<0>(zr1.at(maxind));
        int _to = std::get<1>(zr1.at(maxind));
        cv::Rect upper_rect(0, 0, w, _from);
        cv::Rect lower_rect(0, _to, w, h-_to);
        cv::Mat upper_image = img(upper_rect);
        cv::Size s = upper_image.size();
        int h = s.height;
        int w = s.width;
        if (h > height && w > 1) {
            ImageNode ut(upper_image, tree->get_x(), tree->get_y());
            ImageNode* upper_tree = new ImageNode(ut);
            tree->set_left(upper_tree);
            xycut_vertical_cut(upper_image, threshold, upper_tree, 1, height, width);
        }
        cv::Mat lower_image = img(lower_rect);
        s = lower_image.size();
        h = s.height;
        w = s.width;
        if (h > height && w > 1)  {
            ImageNode lt(lower_image, tree->get_x(), _to + tree->get_y());
            ImageNode* lower_tree = new ImageNode(lt);
            tree->set_right(lower_tree);
            xycut_horizontal_cut(lower_image, threshold, lower_tree, 1, height, width);
        }
        return;
    }
    xycut_vertical_cut(img, threshold, tree, level+1, height, width);
};

std::vector<ImageNode> Xycut::xycut() {
    cv::Mat copy = image.clone();
    //cv::threshold(copy, copy, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    //cv::bitwise_not(copy,copy);

    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(copy, copy, cv::MORPH_CLOSE, kernel);

    //cv::adaptiveThreshold(copy,copy, 255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY_INV,11,2);
    // th3 = cv.adaptiveThreshold(img,255,cv.ADAPTIVE_THRESH_GAUSSIAN_C,\
    //            cv.THRESH_BINARY,11,2)

    cv::Mat labeled(copy.size(), copy.type());
    cv::Mat rectComponents = cv::Mat::zeros(cv::Size(0, 0), 0);
    cv::Mat centComponents = cv::Mat::zeros(cv::Size(0, 0), 0);
    connectedComponentsWithStats(copy, labeled, rectComponents, centComponents);

    int count = rectComponents.rows - 1;
    std::vector<double> heights;

    std::vector<cv::Rect> big_rects;

    for (int i = 1; i < rectComponents.rows; i++) {
        int h = rectComponents.at<int>(cv::Point(3, i));
        int w = rectComponents.at<int>(cv::Point(2, i));
        int x = rectComponents.at<int>(cv::Point(0, i));
        int y = rectComponents.at<int>(cv::Point(1, i));
        if (h/w < 50) {
            heights.push_back(h);
        } else {
            big_rects.push_back(cv::Rect(x,y,w,h));
        }
    }

    if (count == 0) {
        ImageNode node(image, 0,0);
        std::vector<ImageNode> images;
        images.push_back(node);
        return images;
    }

    for (int i=0;i<big_rects.size();i++) {
        copy(big_rects.at(i)).setTo(0);
    }

    cv::Scalar m, stdv;
    cv::Mat hist(1, heights.size(), CV_64F, &heights[0]);
    meanStdDev(hist, m, stdv);
    int height = m(0);

    Output out = image_without_white_borders(copy);
    cv::Mat wo_borders = out.image;

    ImageNode tree(wo_borders, out.x, out.y);
    std::vector<int> lst;
    xycut_vertical_find(wo_borders, height, 0, height, image.size().width, lst);
    std::vector<int>::iterator result = std::min_element(lst.begin(), lst.end());

    if (result != lst.end()) {
        int threshold = (*result) - 1 ;
        xycut_vertical_cut(wo_borders, threshold, &tree, 0, height, image.size().width);
        int size = tree.to_vector().size();
        if (size > 20) {
            ImageNode node(image, 0,0);
            std::vector<ImageNode> v;
            v.push_back(node);
            return v;
        }
        return tree.to_vector();
    } else {
        ImageNode node(image, 0,0);
        std::vector<ImageNode> v;
        v.push_back(node);
        return v;
    }

}


