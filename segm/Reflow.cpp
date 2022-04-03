//
//  Reflow.cpp
//  StaticLibrary
//
//  Created by Mikhno Sergey (Galexis) on 20.10.19.
//  Copyright © 2019 Sergey Mikhno. All rights reserved.
//

#include "Reflow.h"
#include "LineSpacing.h"

std::vector<int> Reflow::calculate_line_heights(std::vector<int> line_heights)
{

    std::vector<int> new_line_heights;
    std::vector<int>::iterator result = std::min_element(line_heights.begin(), line_heights.end());
    int min_height = *result;
    int addition = (float)min_height/3.0;

    for (int i=0; i<line_heights.size(); i++)
    {
        new_line_heights.push_back(line_heights.at(i) + addition);
    }

    LineSpacing ls(new_line_heights);
    return ls.get_line_heights();
}


cv::Mat Reflow::reflow(float scale, int page_width, float margin)
{

    int new_width = page_width;
    //scale = portrait ? scale : scale * screen_ratio;
    //int new_width = ceil(image.size().width);
    //int new_width = ceil(page_width);
    int left_margin = ceil(page_width * 0.075 * margin);
    int paragraph_indent = 30;
    int max_symbol_height = 0;
    std::vector<int> line_heights;
    std::map<int,int> glyph_number_to_line_number;
    std::map<int,std::vector<glyph>> lines;
    std::map<int,int> scaled_glyphs;

    //---------------------------
    // calculate line  heights
    //---------------------------

    int line_sum = left_margin;
    int line_number = 0;
    std::vector<glyph> line;
    bool last = false;

    for (int i=0; i<glyphs.size(); i++)
    {
        glyph g = glyphs.at(i);


        bool indented = g.indented;
        int new_symbol_width = ceil(g.width * scale);
        int new_symbol_height = ceil(g.height * scale);
        if (new_symbol_height > max_symbol_height)
        {
            max_symbol_height = new_symbol_height;
        }

        if (g.is_space && line.empty())
        {
            continue;
        }

        if (last || g.indented)
        {
            line_sum += paragraph_indent;
        }

        if (line_sum + new_symbol_width < new_width - left_margin && !indented)
        {
            line.push_back(g);
            line_sum += new_symbol_width;
            glyph_number_to_line_number.insert(std::make_pair(i, line_number));

        }
        else
        {

            if (line.size() > 0)
            {
                lines.insert(std::make_pair(line_number, line));
            }

            if (new_symbol_width <= new_width - 2*left_margin)
            {

                if (line.size() > 0)
                {
                    line_number++;
                }

                line = std::vector<glyph>();
                line_sum = left_margin + new_symbol_width;
                max_symbol_height = new_symbol_height;
                glyph_number_to_line_number.insert(std::make_pair(i, line_number));
                line.push_back(g);
                if (last || g.indented)
                {
                    line_sum += paragraph_indent;
                }
            }
            else
            {
                // this big glyph must be on a new line

                int scaled_symbol_width = new_width - 2*left_margin;
                float scaling_coef = scaled_symbol_width/(float)new_symbol_width;
                int scaled_symbol_height = new_symbol_height*scaling_coef;
                scaled_glyphs.insert(std::make_pair(i, scaled_symbol_height));

                line = std::vector<glyph>();
                line.push_back(g);
                if (line_number > 0)
                {
                    line_number++;
                }

                lines.insert(std::make_pair(line_number, line));
                line = std::vector<glyph>();
                max_symbol_height = 0;
                line_sum = left_margin;
                line_number++;
            }
        }

        last = g.is_last;

    }

    if (line.size() > 0)
    {
        lines.insert(std::make_pair(line_number, line));
    }

    std::vector<int> line_numbers;;
    for (auto const& element : lines)
    {
        line_numbers.push_back(element.first);
    }

    auto it = std::max_element(line_numbers.begin(), line_numbers.end());
    line_number = (*it);

    line_heights = std::vector<int>();
    int g_counter = 0;
    for (int k=0; k<=line_number; k++)
    {
        std::vector<glyph> glyphs = lines.at(k);
        int m = 0;
        for (int l=0; l<glyphs.size(); l++)
        {
            glyph g = glyphs.at(l);
            if (l==0 && g.is_space)
            {
                g_counter++;
                continue;
            }
            int new_symbol_height = ceil(g.height * scale);
            if (scaled_glyphs.find(g_counter) != scaled_glyphs.end())
            {
                new_symbol_height = scaled_glyphs.at(g_counter);
            }
            if (new_symbol_height > m)
            {
                m = new_symbol_height;
            }
            g_counter++;
        }
        line_heights.push_back(m);
    }

    line_heights = calculate_line_heights(line_heights);
    int new_height = std::accumulate(line_heights.begin(), line_heights.end(), 0);
    line_sum = left_margin;
    int top_margin = std::min(ceil(new_height * 0.075), left_margin * 1.25);
    int current_vert_pos = top_margin;

    // new image to copy pixels to

    cv::Mat new_image(new_height + 2*top_margin, new_width, image.type());
    new_image.setTo(cv::Scalar(0));

    current_vert_pos = top_margin;


    for (int i=0; i<=line_number; i++)
    {
        std::vector<glyph> glyphs = lines.at(i);
        int line_height = line_heights.at(i);
        //cv::line(new_image, cv::Point(0,current_vert_pos), cv::Point(new_width, current_vert_pos), cv::Scalar(255), 5);
        line_sum = left_margin ;
        last = false;
        for (int j=0; j<glyphs.size(); j++)
        {
            glyph g = glyphs.at(j);

            if (j==0 && g.is_space)
            {
                continue;
            }

            if (last || g.indented)
            {
                line_sum += paragraph_indent;
            }

            cv::Mat symbol_mat = g.is_picture ? rotated_with_pictures(cv::Rect(g.x, g.y, g.width, g.height)) : image(cv::Rect(g.x, g.y, g.width, g.height));
            int new_symbol_width = ceil(symbol_mat.size().width * scale);
            int new_symbol_height = ceil(symbol_mat.size().height * scale);

            cv::Mat dst(new_symbol_height, new_symbol_width, symbol_mat.type());
            cv::resize(symbol_mat, dst, dst.size(), 0,0, cv::INTER_CUBIC);
            int x_pos = line_sum;

            int y_pos = (current_vert_pos + line_height) + (g.baseline_shift - g.height)*scale;
            if (x_pos + new_symbol_width < new_width - left_margin)
            {
                cv::Rect dstRect(x_pos, y_pos, new_symbol_width, new_symbol_height);
                if (!g.is_space)
                {
                    dst.copyTo(new_image(dstRect));
                }
            }
            else
            {
                int scaled_symbol_width = (new_width - left_margin) - x_pos;
                if (scaled_symbol_width > 0)
                {

                    // calucalte new symbol height

                    float scale_coef = scaled_symbol_width/(float)new_symbol_width;
                    int y_pos = (current_vert_pos + line_height) + (g.baseline_shift - g.height)*scale*scale_coef;
                    int scaled_symbol_height = scale_coef * new_symbol_height;
                    cv::Mat dst(scaled_symbol_height, scaled_symbol_width, symbol_mat.type());
                    cv::resize(symbol_mat, dst, dst.size(), 0,0, cv::INTER_CUBIC);
                    cv::Rect dstRect(x_pos, y_pos, scaled_symbol_width, scaled_symbol_height);
                    if (!g.is_space)
                    {
                        dst.copyTo(new_image(dstRect));
                    }
                }

            }
            line_sum += new_symbol_width;
            last = g.is_last;
        }
        current_vert_pos += line_height;


    }

    //cv::imwrite(std::string("/data/local/tmp/im.png"), new_image);
    //cv::imwrite(std::string("/storage/emulated/0/Download/im.png"), new_image);

    //cv::bitwise_not(new_image, new_image);
    return new_image;
}
