//
//  LineSpacing.hpp
//  StaticLibrary
//
//  Created by Mikhno Sergey (Galexis) on 21.11.19.
//  Copyright © 2019 Sergey Mikhno. All rights reserved.
//

#ifndef LineSpacing_h
#define LineSpacing_h

#include "common.h"

class LineSpacing
{

public:
    LineSpacing(std::vector<int> heights) : heights(heights) {}
    std::vector<int> get_line_heights();
private:
    std::vector<int> heights;

};



#endif /* LineSpacing_hpp */
