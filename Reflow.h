//
//  Reflow.h
//  StaticLibrary
//
//  Created by Mikhno Sergey (Galexis) on 20.10.19.
//  Copyright © 2019 Sergey Mikhno. All rights reserved.
//



#ifndef Reflow_h
#define Reflow_h

#include "common.h"

class Reflow {
    
public:
    Reflow(cv::Mat image, cv::Mat& rotated_with_pictures, std::vector<glyph> glyphs) : image(image), rotated_with_pictures(rotated_with_pictures), glyphs(glyphs) {
    }
    cv::Mat reflow(float scale, int page_width, float margin);
private:
    cv::Mat image;
    cv::Mat rotated_with_pictures;
    std::vector<glyph> glyphs;
    std::vector<int> calculate_line_heights(std::vector<int> line_heights);
};




#endif /* Reflow_h */
