//
// Created by Sergey Mikhno on 2019-05-13.
//

#include "common.h"
#include "Enclosure.h"
#include "PageSegmenter.h"
#include "RectJoin.h"


line_limit PageSegmenter::find_baselines(vector<double_pair> &cc)
{

    sort(cc.begin(), cc.end(), PairXOrder());
    vector<Rect> line_rects;

    for (int i = 0; i < cc.size(); i++)
    {
        double_pair t = cc.at(i);
        line_rects.push_back(rd.at(t));
    }

    int max = numeric_limits<int>::min();
    int min = numeric_limits<int>::max();

    vector<int> lowerData;//[line_rects.size()];

    for (int i = 0; i < line_rects.size(); i++)
    {
        Rect rect = line_rects.at(i);
        if (rect.y < min)
        {
            min = rect.y;
        }
        if (rect.y + rect.height > max)
        {
            max = rect.y + rect.height;
        }
        lowerData.push_back(rect.y + rect.height);
    }

    int size = line_rects.size();

    map<int,int> countsLower;

    for (int c = 0; c < size; c++)
    {
        double m = lowerData[c];
        if (countsLower.find(m) == countsLower.end())
        {
            countsLower.insert(make_pair(m, 1));
        }
        else
        {
            countsLower.at(m) +=1;
        }

    }

    int maxLower = numeric_limits<int>::min();
    int maxLowerIndex = 0;

    for ( auto it = countsLower.begin(); it != countsLower.end(); it++ )
    {
        if (it->second > maxLower)
        {
            maxLower = it->second;
            maxLowerIndex = it->first;
        }
    }

    return line_limit(min, 0, maxLowerIndex, max);

}

map<int, vector<double_pair>>
                           PageSegmenter::get_connected_components(vector<double_pair> &center_list, double average_height)
{

    map<int, vector<double_pair>> return_value;

    int size = center_list.size();
    double data[size][2];

    for (int i = 0; i < size; i++)
    {
        data[i][0] = get<0>(center_list.at(i));
        data[i][1] = get<1>(center_list.at(i));
    }

    ::flann::Matrix<double> dataset(&data[0][0], size, 2);

    int k = std::min(100,size);

    ::flann::Index<::flann::L2<double>> index(dataset, ::flann::KDTreeIndexParams(1));
    index.buildIndex();

    ::flann::Matrix<int> indices(new int[size*k], size, k);
    ::flann::Matrix<double> dists(new double[size*k], size, k);

    double q[size][2];

    for (int i = 0; i < size; i++)
    {
        auto p = center_list[i];
        q[i][0] = get<0>(p);
        q[i][1] = get<1>(p);
    }

    ::flann::Matrix<double> query(&q[0][0], size, 2);
    index.knnSearch(query, indices, dists, k, ::flann::SearchParams());

    map<int,bool> verts;

    for (int i = 0; i < size; i++)
    {
        auto p = center_list[i];

        vector<double_pair> neighbors;
        bool found_neighbor = false;
        double_pair right_nb;

        double mindist = numeric_limits<double>::max();

        std::vector<int> right_intersections;
        for (int j = 0; j < k; j++)
        {
            int ind = indices[i][j];
            double_pair nb = center_list[ind];
            if (get<0>(nb) > get<0>(p))
            {
                cv::Rect rect1 = rd.at(nb);
                cv::Rect rect2 = rd.at(p);
                int a_s = rect1.y;
                int a_e = rect1.y + rect1.height;
                int b_s = rect2.y;
                int b_e = rect2.y + rect2.height;
                if (b_s <= a_e && a_s <= b_e)
                {
                    int o_s = std::max(a_s, b_s);
                    int o_e = std::min(a_e, b_e);
                    int diff = o_e - o_s;

                    if (std::min(rect1.height, rect2.height) > average_height && diff > 0.5 * std::max(rect2.height, rect1.height))
                    {
                        right_intersections.push_back(j);
                    }
                }
            }
        }

        for (int j = 0; j < k; j++)
        {
            int ind = indices[i][j];
            double_pair nb = center_list[ind];
            if (get<0>(nb) > get<0>(p))
            {
                double dist = sqrt(((get<1>(nb) - get<1>(p)) * (get<1>(nb) - get<1>(p))) +
                                   (get<0>(nb) - get<0>(p)) * (get<0>(nb) - get<0>(p)));

                if (std::find(right_intersections.begin(), right_intersections.end(), j) != right_intersections.end())
                {
                    if (dist < mindist)
                    {
                        mindist = dist;
                        right_nb = make_tuple(get<0>(nb), get<1>(nb));
                        found_neighbor = true;
                    }
                }

            }
        }

        if (found_neighbor)
        {
            //cv::line(mat, cv::Point(get<0>(p), get<1>(p)), cv::Point(get<0>(right_nb), get<1>(right_nb)), cv::Scalar(255));
            double_pair point(get<0>(p), get<1>(p));

            int n = center_numbers.at(point);
            int m = center_numbers.at(right_nb);

            vertex_t v1 = vertex(n, g);
            vertex_t v2 = vertex(m, g);
            add_edge(v1, v2, g);

        }

    }

    delete [] indices.ptr();
    delete [] dists.ptr();

    std::vector<int> c(num_vertices(g));

    connected_components(g, make_iterator_property_map(c.begin(), get(vertex_index, g), c[0]));

    for (int i = 0; i < c.size(); i++)
    {
        int cn = c[i];
        if (return_value.find(cn) == return_value.end())
        {
            return_value.insert(make_pair(cn, vector<double_pair>()));
        }
        return_value.at(cn).push_back(g[i]);

    }



    return return_value;
}


vector<line_limit> PageSegmenter::get_line_limits()
{

    const cc_result cc_results = get_cc_results();

    Mat hist;
    reduce(mat, hist, 1, REDUCE_SUM, CV_32F);
    auto runs = one_runs(hist);
    //hist.release();

    std::vector<line_limit> limits = std::vector<line_limit>();
    if (runs.size()==1)
    {
        //line_limit ll(0, 0, mat.rows, mat.rows);
        //limits.push_back(ll);
        //return limits;
    }

    if (cc_results.whole_page)
    {
        vector<line_limit> v;
        line_limit ll(0,0,mat.rows,mat.rows);
        line_height = mat.size().height;
        v.push_back(ll);
        return v;
    }
    else
    {
        double average_height = cc_results.average_hight;
        vector<double_pair> centers = cc_results.centers;

        const map<int, vector<double_pair>> components = get_connected_components(centers, average_height);

        line_height = (int) average_height * 2;

        vector<int> keys;

        boost::copy(components | boost::adaptors::map_keys,
                    std::back_inserter(keys));

        vector<line_limit> v;
        for (int i=0; i<keys.size(); i++)
        {
            int cn = keys.at(i);
            vector<double_pair> cc = components.at(cn);
            line_limit ll = find_baselines(cc);
            if(cc.size() > 1)
            {
                v.push_back(ll);
            }

        }

        return v;
    }


}

cc_result PageSegmenter::get_cc_results()
{

    Mat image;
    const Mat kernel = getStructuringElement(MORPH_RECT, Size(1, 1));
    //dilate(mat, image, kernel, Point(-1, -1), 1);


    Mat labeled(mat.size(), mat.type());
    Mat rectComponents = Mat::zeros(Size(0, 0), 0);
    Mat centComponents = Mat::zeros(Size(0, 0), 0);
    connectedComponentsWithStats(image, labeled, rectComponents, centComponents);

    int count = rectComponents.rows - 1;

    double heights[count];


    vector<double_pair> center_list;
    vector<Rect> rects;
    vector<int> v_heights;
    int min_height = INT_MAX;
    int max_height = 0;

    for (int i = 1; i < rectComponents.rows; i++)
    {
        int x = rectComponents.at<int>(Point(0, i));
        int y = rectComponents.at<int>(Point(1, i));
        int w = rectComponents.at<int>(Point(2, i));
        int h = rectComponents.at<int>(Point(3, i));

        //int area = rectComponents.at<int>(i, cv::CC_STAT_AREA);

        //if (area > 200) {
        Rect rectangle(x, y, w, h);
        rects.push_back(rectangle);
        v_heights.push_back(h);
        heights[i-1] = h;
        if (h > max_height) {
            max_height = h;
        }
        if (h < min_height) {
            min_height = h;
        }
        //}
    }

    std::pair<std::vector<int>, std::vector<float>> h_hist = make_hist(v_heights, 50, min_height, max_height);


    std::vector<int> freqs = h_hist.first;
    std::vector<float> height_limits = h_hist.second;

    std::vector<int>::iterator max_height_iter = std::max_element(freqs.begin(), freqs.end()); // [2, 4)
    int argmaxVal = std::distance(freqs.begin(), max_height_iter);
    
    int most_freq_height = height_limits[argmaxVal];

  
    int pos = freqs.size()-1;
    for (int i=freqs.size()-1; i>=0; i--) {
        int f = freqs[i];
        int h = height_limits[i];
        if (f > 0 && h > 5*most_freq_height) {
           pos = i;         
        } 
    }
    
    int cut = height_limits[pos];

    std::vector<cv::Rect> filtered_rects;
    std::vector<cv::Rect> pictures;
    for (int i=0;i<rects.size(); i++) {
        cv::Rect rect = rects[i];
        if (rect.height < cut) {
            filtered_rects.push_back(rect);
        } else {
           mat(rect).setTo(0);
           this->pictures.push_back(rect); 
        }
    }

    rects = join_rects(filtered_rects);

    if (count == 0)
    {
        cc_result v;
        v.whole_page = true;
        v.average_hight = mat.size().height;
        v.centers = center_list;
        return v;
    }

    Scalar m, stdv;
    Mat hist(1, count, CV_64F, &heights);
    meanStdDev(hist, m, stdv);

    double min, max;
    cv::minMaxLoc(hist, &min, &max);

    std::pair<std::vector<int>,std::vector<float>> p = make_hist(v_heights, std::min((int)v_heights.size(), 5), min, max);

    std::vector<int> hst = std::get<0>(p);
    std::vector<float> steps = std::get<1>(p);

    auto max_iter = std::max_element(hst.begin(), hst.end());
    int max_ind = (int)std::distance(hst.begin(), max_iter);
    double average_height = steps[max_ind] ;

    double std = stdv(0);

    bool whole_page = false;

    Enclosure enc(rects);
    const set<array<int, 4>> &s = enc.solve();

    int i = 0;
    for (auto it = s.begin(); it != s.end(); ++it)
    {
        array<int, 4> a = *it;
        int x = -get<0>(a);
        int y = -get<1>(a);
        int width = get<2>(a) - x;
        int height = get<3>(a) - y;
        if (height > average_height - std)
        {
            double cx = (x + width) / 2.0;
            double cy = (y + height) / 2.0;
            rd[make_tuple((x + width) / 2.0, (y + height) / 2.0)] = Rect(x, y, width, height);
            double_pair center = std::tuple<double,double>(cx, cy);
            center_list.push_back(center);
            center_numbers.insert(make_pair(center, i));
            vertex_t v = add_vertex(g);
            g[v] = center;
            i++;
        }
    }

    if (center_list.size() < 30)
    {
        whole_page = true;
    }

    cc_result v;
    v.whole_page = whole_page;
    v.average_hight = average_height;
    v.centers = center_list;
    return v;

}


std::map<int,int> PageSegmenter::calculate_left_indents(std::vector<int> lefts)
{

    std::map<int,int> left_indents;

    if (lefts.empty())
    {
        return left_indents;
    }

    int w = mat.size().width;

    std::vector<int> current_inds;
    std::vector<int> current_points;
    int cur_min = lefts.at(0);
    current_inds.push_back(0);
    current_points.push_back(lefts.at(0));

    for (int i=1; i<lefts.size(); i++)
    {
        if (abs(lefts.at(i) - cur_min) > w/6. )
        {
            for (int j=0; j<current_inds.size(); j++)
            {
                int cur_ind = current_inds.at(j);
                left_indents.insert(std::make_pair(cur_ind, cur_min));
            }
            current_inds = std::vector<int>();
            current_points = std::vector<int>();
            cur_min = lefts.at(i);
        }
        current_inds.push_back(i);
        current_points.push_back(lefts.at(i));
        cur_min = lefts.at(i) < cur_min ? lefts.at(i) : cur_min;
    }

    auto it = std::min_element(current_points.begin(), current_points.end());
    for (int j=0; j<current_inds.size(); j++)
    {
        int cur_ind = current_inds.at(j);
        left_indents.insert(std::make_pair(cur_ind, (*it)));
    }


    return left_indents;


}

std::vector<line_limit> PageSegmenter::join_lines(std::vector<line_limit> line_limits)
{
    set<int> small_line_limits;
    for (int i=0; i<line_limits.size(); i++)
    {
        for (int j=0; i!=j && j<line_limits.size(); j++)
        {
            line_limit lli = line_limits.at(i);
            line_limit llj = line_limits.at(j);

            if ( (llj.upper <= lli.upper && llj.lower >= lli.lower))
            {
                small_line_limits.insert(i);
            }

            if ( (lli.upper <= llj.upper && lli.lower >= llj.lower))
            {
                small_line_limits.insert(j);
            }
        }
    }

    std::vector<line_limit> new_line_limits;
    for (int i=0; i<line_limits.size(); i++)
    {
        if (small_line_limits.find(i) == small_line_limits.end() )
        {
            new_line_limits.push_back(line_limits.at(i));
        }
    }

    line_limits = new_line_limits;

    sort(line_limits.begin(), line_limits.end(), SortLineLimits());

    new_line_limits = std::vector<line_limit>();

    std::set<int> added_lines;

    if (line_limits.size() > 1)
    {
        for (int i=0; i<line_limits.size()-1; i++)
        {
            line_limit lf = line_limits.at(i);
            line_limit ls = line_limits.at(i+1);
            if (lf.lower > ls.upper)
            {
                int intersection = lf.lower - std::max(lf.upper, ls.upper);
                if (intersection > 0.5*std::max(ls.lower-ls.upper, lf.lower - lf.upper))
                {
                    line_limit joined(std::min(lf.upper,ls.upper), lf.upper_baseline, ls.lower_baseline, ls.lower);
                    new_line_limits.push_back(joined);
                    added_lines.insert(i);
                    added_lines.insert(i+1);
                }
                else
                {
                    if (added_lines.count(i) == 0)
                    {
                        new_line_limits.push_back(lf);
                        added_lines.insert(i);
                    }
                }
            }
            else
            {
                if (added_lines.count(i) == 0)
                {
                    new_line_limits.push_back(lf);
                    added_lines.insert(i);
                }
            }
        }
        int last_index = (int)line_limits.size()-1;
        if (added_lines.count(last_index) == 0)
        {
            new_line_limits.push_back(line_limits.at(last_index));
        }
        line_limits = new_line_limits;
    }

    return line_limits;
}


std::vector<glyph> PageSegmenter::get_glyphs()
{

    vector<glyph> return_value;
    //std::vector<cv::Rect> big_rects;
    vector<line_limit> line_limits = get_line_limits();

    line_limits = join_lines(line_limits);


    Mat hist;
    reduce(mat, hist, 0, REDUCE_SUM, CV_32F);

    int w = hist.cols;

    std::vector<cv::Point> points;

    Mat horHist;

    std::vector<int> lefts;

    for (int i=0; i<line_limits.size(); i++)
    {
        line_limit ll = line_limits.at(i);
        int l = ll.lower;
        int bl = ll.lower_baseline;
        int u = ll.upper;
        int bu = ll.upper_baseline;

        Mat lineimage(mat, Rect(0, u, w, l - u));
        reduce(lineimage, horHist, 0, REDUCE_SUM, CV_32F);

        int w = horHist.cols;

        for (int i = 0; i < w; i++)
        {
            if (horHist.at<int>(0, i) > 0)
            {
                horHist.at<int>(0, i) = 1;
            }
            else
            {
                horHist.at<int>(0, i) = 0;
            }
        }

        const vector<std::tuple<int, int>> &oneRuns = one_runs(horHist);

        int x1 = oneRuns.size() > 0 ? get<0>(oneRuns.at(0)) : 0;
        lefts.push_back(x1);
        int y1 = u;

        points.push_back(cv::Point(x1,y1));

    }

    std::map<int,int> left_indents = calculate_left_indents(lefts);
    int counter = 0;
    for (line_limit &ll : line_limits)
    {

        if (line_limits.size() > left_indents.size())
        {
            std::cout << " problem " << left_indents.size() << " " << line_limits.size() << std::endl;
        }

        int left_indent = left_indents.at(counter);
        counter++;

        int l = ll.lower;
        int bl = ll.lower_baseline;
        int u = ll.upper;
        int bu = ll.upper_baseline;

        Mat lineimage(mat, Rect(0, u, w, l - u));
        Mat horHist;
        reduce(lineimage, horHist, 0, REDUCE_SUM, CV_32F);
        const vector<std::tuple<int, int>> &oneRuns = one_runs(horHist);

        //horHist.release();

        int space_width = 0;
        std::vector<double> spaces;
        for (int k=0; k<oneRuns.size(); k++)
        {
            std::tuple<int, int> r = oneRuns.at(k);
            int left = get<0>(r);
            int right = get<1>(r);

            glyph g;

            if (k == 0 &&  (left - left_indent) > 0.02 * w)
            {
                g.indented = 1;
            }
            else
            {
                g.indented = 0;
            }

            // detect real height

            Mat hist;
            cv::Mat letter = lineimage(cv::Rect(left, 0, right-left, l-u));
            cv::Size size = letter.size();
            if (size.width < 2 || size.height < 2)
            {
                continue;
            }
            reduce(letter, hist, 1, REDUCE_SUM, CV_32F);
            const vector<std::tuple<int, int>> vert_one_runs = one_runs(hist);

            int shift = vert_one_runs.size() > 0 ? get<0>(vert_one_runs.at(0)) : 0;

            // end detect

            g.x = left;
            g.y = u + shift;
            g.width = right - left;

            g.height = l - u - shift;
            g.baseline_shift = l - bl   ;
            g.line_height = line_height;
            g.is_space = 0;
            if (k == oneRuns.size()-1)
            {
                g.is_last = 1;
            }
            if (g.width > 0)
            {
                return_value.push_back(g);
            }
            else
            {
                continue;
            }

            glyph space;
            space.x = right;
            int lastspacewidth = std::min(5, w - right);
            int nextx = k != oneRuns.size()-1 ? get<0>(oneRuns.at(k+1)) : right + lastspacewidth;
            space_width = nextx - right;
            spaces.push_back(space_width);
            space.width = space_width;
            space.y = u + shift;
            space.baseline_shift = l - bl;
            space.line_height = line_height;
            space.height = l - u - shift;
            space.indented = 0;
            space.is_space = 1;
            if (k != oneRuns.size()-1 && (float)space.height / space.width < 10)
            {
                return_value.push_back(space);
            }
        }
    }

    //mat.release();
    //
    for (cv::Rect pic_rect : this->pictures) {
            int y = pic_rect.y - pic_rect.height;
            auto it = std::find_if(return_value.begin(), return_value.end(), [y] (const glyph& gl) {return gl.y - gl.height > y;} );
            glyph g;
            g.x = pic_rect.x;
            g.y = pic_rect.y;
            g.width = pic_rect.width;
            g.height = pic_rect.height;
            g.is_picture = true;
            return_value.insert(it, g);
    }

    return return_value;


}

