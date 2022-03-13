//
//  LineSpacing.cpp
//  StaticLibrary
//
//  Created by Mikhno Sergey (Galexis) on 21.11.19.
//  Copyright © 2019 Sergey Mikhno. All rights reserved.
//

#include "LineSpacing.h"

std::vector<int> LineSpacing::get_line_heights() {
    std::vector<int> positions;
    for (int i=0;i<heights.size()-1;i++ ){
        float r =  heights.at(i+1)/(float)heights.at(i);
        if (r > 2 || r < 0.5) {
            positions.push_back(i);
        }
    }

    int start_pos = 0;
    int end_pos = (int)heights.size() - 1;

    std::vector<std::pair<int,int> > blocks;

    for (int i = 0; i<positions.size(); i++){
        blocks.push_back(std::make_pair(start_pos,positions.at(i)));
        start_pos = positions.at(i)+1;
    }
    blocks.push_back(std::make_pair(start_pos,end_pos));

    std::vector<int> h;
    for (int i=0;i<blocks.size();i++) {
        std::pair<int,int> p = blocks.at(i);
        int f = std::get<0>(p);
        int s = std::get<1>(p);
        auto maxIter = std::max_element(heights.begin()+f, heights.begin()+s);
        int l = s - f + 1;
        for (int j=0;j<l;j++) {
            h.push_back(*maxIter);
        }
    }
    std::vector<int> new_h;
    
    for (int i=0;i<heights.size();i++) {
        if (h.at(i) >= heights.at(i)) {
            new_h.push_back(h.at(i));
        } else {
            new_h.push_back(heights.at(i));
        }
    }
    
    return new_h;
}
