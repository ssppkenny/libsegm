#!/opt/miniconda3/envs/py39/bin/python
from collections import defaultdict
import argparse
from dataclasses import dataclass
import scipy.cluster.hierarchy as hcluster
from itertools import groupby

import sys
import cv2
import distinctipy
import networkx as nx
import numpy as np
from pyflann import FLANN, set_distance_type
from segm import join_rects, find_bounding_rect, glyph_result
from enum import Enum


class Position(int, Enum):
    LEFT = 1
    RIGHT = 2
    BOTTOM = 3
    UP = 4
    UNKNOWN = 5


@dataclass
class PictureResults:
    joined_rects: list[glyph_result]
    bounding_rects: list[list[glyph_result]]
    colors: list[tuple[int, int, int]]
    connected_components: dict[int, int]


def dist(p1: tuple[int, int], p2: tuple[int, int]):
    """
    Euclidean distance bewtween two points
    :param p1: tuple representing a point
    :param p2: tuple representing a point
    :return: float distance between two points
    """
    x1, y1 = p1
    x2, y2 = p2
    return np.sqrt((x1 - x2) ** 2 + (y1 - y2) ** 2)


def dist_rects(r1: glyph_result, r2: glyph_result) -> float:
    """
    Distance between two rectangles
    :param r1: glyph_result, first rectangle
    :param r2: glyph_result, second rectangle
    :return: float, distance between the rectangles
    """
    position = Position.UNKNOWN
    ab, ae = r1.x, r1.x + r1.width
    bb, be = r2.x, r2.x + r2.width
    cb, ce = r1.y, r1.y + r1.height
    db, de = r2.y, r2.y + r2.height
    no_x_overlap = bb > ae or ab > be
    no_y_overlap = db > ce or cb > de
    if no_x_overlap and no_y_overlap:
        points1 = [
            (r1.x, r1.y),
            (r1.x, r1.y + r1.height),
            (r1.x + r1.width, r1.y),
            (r1.x + r1.width, r1.y + r1.height),
        ]
        points2 = [
            (r2.x, r2.y),
            (r2.x, r2.y + r2.height),
            (r2.x + r2.width, r2.y),
            (r2.x + r2.width, r2.y + r2.height),
        ]
        return np.min([dist(p, q) for p in points1 for q in points2]), position
    if no_x_overlap and not no_y_overlap:
        x1 = r1.x
        x2 = r1.x + r1.width
        x3 = r2.x
        x4 = r2.x + r2.width
        if r1.x < r2.x:
            position = Position.LEFT
        else:
            position = Position.RIGHT
        return np.min(
            [
                abs(x1 - x3),
                abs(x1 - x4),
                abs(x2 - x3),
                abs(x2 - x4),
            ]
        ), position
    if not no_x_overlap and no_y_overlap:
        x1 = r1.y
        x2 = r1.y + r1.height
        x3 = r2.y
        x4 = r2.y + r2.height
        if r1.y < r2.y:
            position = Position.UP
        else:
            position = Position.BOTTOM
        return np.min([abs(x1 - x3), abs(x1 - x4), abs(x2 - x3), abs(x2 - x4)]), position
    if not no_x_overlap and not no_y_overlap:
        return 0, position


def distance(s: set[int], pi: int, jr: list[glyph_result]) -> float:
    """
    Distance between a picture rectangle represented by the index and a text block, represented by a set of rectangles
    :param s: set of rectangles
    :param pi: picture index in the jr: joined rectangles
    :param jr: joined_rectangles
    :return: float, distance
    """
    rects = []
    for i, n in enumerate(s):
        r = jr[n]
        rects.append(r)
    br = find_bounding_rect(list(range(len(rects))), rects)

    pic = jr[pi]
    rv, position = dist_rects(br, pic)
    return rv, position


def find_pictures_with_captions(
    filename: str, symbols_from: int = 1, symbols_to: int = 100
) -> PictureResults:
    """
    This function looks for pictures and tries to find the captions,
    :param filename:
    :param symbols_from:
    :param symbols_to:
    :return: PictureResult with all the glyph rectangles, bounding rectagles for pictures with captions,
    distinct colors for all separate blocks and those blocks as graph connected components
    """
    set_distance_type("euclidean")
    img = cv2.imread(filename, 0)
    jr = join_rects(img)
    heights = [j.height for j in jr]
    freqs, sizes = np.histogram(heights, bins=200)
    ind = np.argmax(freqs)
    most_frequent_height = np.mean(heights)#int(sizes[ind + 1])
    ## indexes of picture rectangles are those with the hight > than 5 * most frequent rectangle hight
    pic_inds = [i for i, r in enumerate(jr) if r.height > 7 * most_frequent_height]

    ## centers of the rectangles
    centroids = np.zeros((len(jr), 2))
    for i, r in enumerate(jr):
        centroids[i, :] = np.array([r.x + r.width / 2, r.y + r.height / 2])
    flann = FLANN()
    # params = flann.build_index(centroids, algorithm="kdtree", trees=4)
    # nearest_neighbors, dists = flann.nn_index(centroids, 15, checks=params["checks"])

    print(most_frequent_height)
    ls = np.linspace(2,4,20)
    current_len_cluster = float('inf')
    lengths = []
    for i, d in enumerate(ls):
        clusters = hcluster.fclusterdata(centroids, d * most_frequent_height, criterion="distance")
        ddd = dict((i, c) for i, c in enumerate(clusters))
        res = defaultdict(set)
        for v,k in ddd.items():
            res[k].add(v)
        #print(res)

        #nearest_neighbors, dists = flann.nn(
        #    centroids,
        #    centroids,
        #    30,
        #    algorithm="kmeans",
        #    branching=32,
        #    iterations=7,
        #    checks=16,
        #)

        #g = nx.Graph()
        #for i in range(len(centroids)):
        #    nbs = nearest_neighbors[i, :][1:]
        #    for nb in nbs:
        #        x1, y1 = centroids[nb]
        #        x2, y2 = centroids[i]
        #        # connect rectangles if the distance between them is less then 3 * most frequent rectangle hight
        #        if np.sqrt((x1 - x2) ** 2 + (y1 - y2) ** 2) < 2.5 * most_frequent_height:
        #            g.add_edge(nb, i)
        #        else:
        #            g.add_node(i)

        ##cc = [x for x in nx.connected_components(g)]
        cc = [x for _, x  in res.items()]
        #print(cc)
        
        ##cc = res 
        connected_components = dict()
        for i, c in enumerate(cc):
            for e in c:
                connected_components[e] = i

        colors = get_distinct_colors(len(cc))
        print(f"found {len(cc)} clusters")
        lengths.append(len(cc))
        if len(set(lengths[-3:])) == 1 and len(lengths[-3:])==3:
            break
        #if len(cc) == current_len_cluster:
            break
        #current_len_cluster = len(cc)

    bounding_rects = []

    # small components are those with rectangle count between symbols_from and symbols_to
    small_components = [s for s in cc if symbols_from < len(s) < symbols_to]

    belongs = detect_belonging_captions(jr, pic_inds, small_components, most_frequent_height, multiplier=d*2)

    for pi in pic_inds:
        rects1 = []
        new_rects = []
        sets = belongs[pi]
        for s in sets:
            for t in s:
                rects1.append(jr[t])
        br = find_bounding_rect(list(range(len(rects1))), rects1)
        new_rects.append(br)
        new_rects.append(jr[pi])
        bounding_rects.append(new_rects)
        #bounding_rects.append(br)

    return PictureResults(jr, bounding_rects, colors, connected_components)


def detect_belonging_captions(jr: list[glyph_result], pic_inds: list[int], small_components: list[set[int]], most_frequent_height, multiplier=2.5) -> dict[int,int]:
    """
    :param jr: glyph results, rectangles
    :param pic_inds: list of integer indexes
    :param small_components: list of sets of integers
    :return: dictionary
    """
    ## for every small components find which picture it belongs to
    # for pi in pic_inds:
    #     ds = []
    #     for k, sc in enumerate(small_components):
    #         d, position = distance(sc, pi, jr)
    #         if position != Position.UNKNOWN and d < 10 * most_frequent_height:
    #             ds.append((d, k, position))
    #     if len(ds)>0:
    #         _, k, p = sorted(ds)[0]
    #         belongs[pi].append(small_components[k])
    # return belongs
    belongs = defaultdict(list)
    ## for every small components find which picture it belongs to
    for k, sc in enumerate(small_components):
        ds = []
        for pi in pic_inds:
            d, p = distance(sc, pi, jr)
            if p != Position.UNKNOWN:
                ds.append((d, pi, p))
        if len(ds) > 0:
            dd, k, p = sorted(ds)[0]
            if dd < multiplier * most_frequent_height:
                belongs[k].append(sc)
    return belongs


def get_distinct_colors(n):
    colors = distinctipy.get_colors(n)
    colors = [(int(255 * a), int(255 * b), int(255 * c)) for a, b, c in colors]
    return colors


def draw_results(
    filename: str,
    picture_results: PictureResults,
    regions=False,
    output_filename="out.png",
    draw=-1
) -> None:
    bounding_rects = picture_results.bounding_rects
    jr = picture_results.joined_rects
    ccc = picture_results.connected_components
    colors = picture_results.colors
    orig = cv2.imread(filename)
    orig2 = cv2.imread(filename)
    if regions:
        g_iter = groupby([(k, r) for k, r in enumerate(jr)], lambda x: ccc[x[0]])
        d = defaultdict(list)
        for key, group in g_iter:
            g = list(group)
            for e in g:
                d[key].append(e[1])
        for k, v in d.items():
            j = find_bounding_rect(list(range(len(v))), v)
            #for j in rs:
            cv2.rectangle(orig, (j.x, j.y), (j.x + j.width, j.y + j.height), colors[k], 2)

        #for i, j in enumerate(jr):
        #    c = colors[ccc[i]]
        #    cv2.rectangle(orig, (j.x, j.y), (j.x + j.width, j.y + j.height), c, 2)
    else:
        for k, lbr in enumerate(bounding_rects):
            for br in lbr:
                cv2.rectangle(
                    orig, (br.x, br.y), (br.x + br.width, br.y + br.height), colors[k], 2
                )
    cv2.imwrite(output_filename, orig)
    if draw >= 0:
        for i, j in enumerate(jr):
            if ccc[i] == draw:
                c = colors[ccc[i]]
                cv2.rectangle(orig2, (j.x, j.y), (j.x + j.width, j.y + j.height), c, 2)
    cv2.imwrite("comps.png", orig2)



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--filename", nargs='?', type=str)
    parser.add_argument("-r", "--regions", action='store_true')
    parser.add_argument("-d", "--draw", default=-1, type=int)
    parser.add_argument("-o", "--output", nargs='?', default="out.png", type=str)
    args = parser.parse_args()

    print(args)
    filename = args.filename
    regions = args.regions
    output=args.output
    cn = args.draw
    picture_results = find_pictures_with_captions(filename, symbols_from=2, symbols_to=75)
    draw_results(filename, picture_results, regions=regions, output_filename=output, draw=cn)

