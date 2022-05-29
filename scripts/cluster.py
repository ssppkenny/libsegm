from collections import defaultdict
from dataclasses import dataclass

import cv2
import distinctipy
import networkx as nx
import numpy as np
from pyflann import FLANN, set_distance_type
from segm import join_rects, find_bounding_rect, glyph_result


@dataclass
class PictureResults:
    joined_rects: list[glyph_result]
    bounding_rects: list[glyph_result]
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
        return np.min([dist(p, q) for p in points1 for q in points2])
    if no_x_overlap and not no_y_overlap:
        x1 = r1.x
        x2 = r1.x + r1.width
        x3 = r2.x
        x4 = r2.x + r2.width
        return np.min(
            [
                abs(x1 - x3),
                abs(x1 - x4),
                abs(x2 - x3),
                abs(x2 - x4),
            ]
        )
    if not no_x_overlap and no_y_overlap:
        x1 = r1.y
        x2 = r1.y + r1.height
        x3 = r2.y
        x4 = r2.y + r2.height
        return np.min(
            [
                abs(x1 - x3),
                abs(x1 - x4),
                abs(x2 - x3),
                abs(x2 - x4),
            ]
        )
        return np.min([dist(p, q) for p in points1 for q in points2])
    if not no_x_overlap and not no_y_overlap:
        return 0


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
    rv = dist_rects(br, pic)
    return rv


def find_pictures_with_captions(filename, symbols_from=1, symbols_to=200):
    set_distance_type("euclidean")
    img = cv2.imread(filename, 0)
    jr = join_rects(img)
    heights = [j.height for j in jr]
    freqs, sizes = np.histogram(heights, bins=100)
    ind = np.argmax(freqs)
    most_frequent_height = int(sizes[ind + 1])
    pic_inds = [i for i, r in enumerate(jr) if r.height > 5 * most_frequent_height]

    centroids = np.zeros((len(jr), 2))
    for i, r in enumerate(jr):
        centroids[i, :] = np.array([r.x + r.width / 2, r.y + r.height / 2])
    flann = FLANN()
    # params = flann.build_index(centroids, algorithm="kdtree", trees=4)
    # nearest_neighbors, dists = flann.nn_index(centroids, 15, checks=params["checks"])

    nearest_neighbors, dists = flann.nn(
        centroids,
        centroids,
        30,
        algorithm="kmeans",
        branching=32,
        iterations=7,
        checks=16,
    )

    g = nx.Graph()
    for i in range(len(centroids)):
        nbs = nearest_neighbors[i, :][1:]
        for nb in nbs:
            x1, y1 = centroids[nb]
            x2, y2 = centroids[i]
            if np.sqrt((x1 - x2) ** 2 + (y1 - y2) ** 2) < 3 * most_frequent_height:
                g.add_edge(nb, i)
            else:
                g.add_node(i)

    cc = [x for x in nx.connected_components(g)]

    colors = distinctipy.get_colors(len(cc))
    colors = [(int(255 * a), int(255 * b), int(255 * c)) for a, b, c in colors]

    ccc = dict()
    for i, c in enumerate(cc):
        for e in c:
            ccc[e] = i

    bounding_rects = []

    small_components = [s for s in cc if 0 < len(s) < 200]

    belongs = defaultdict(list)
    for sc in small_components:
        ds = []
        for pi in pic_inds:
            d = distance(sc, pi, jr)
            ds.append(d)
        min_ind = np.argmin(ds)
        if ds[min_ind] < 3 * most_frequent_height:
            belongs[pic_inds[min_ind]].append(sc)

    for pi in pic_inds:
        rects1 = []
        sets = belongs[pi]
        for s in sets:
            for t in s:
                rects1.append(jr[t])
        rects1.append(jr[pi])
        br = find_bounding_rect(list(range(len(rects1))), rects1)
        bounding_rects.append(br)

    return PictureResults(jr, bounding_rects, colors, ccc)


def draw_results(
    filename, bounding_rects, jr, ccc, colors, regions=False, output_filename="out.png"
):
    orig = cv2.imread(filename)
    if regions:
        for i, j in enumerate(jr):
            c = colors[ccc[i]]
            cv2.rectangle(orig, (j.x, j.y), (j.x + j.width, j.y + j.height), c, 2)
    else:
        for k, br in enumerate(bounding_rects):
            cv2.rectangle(
                orig, (br.x, br.y), (br.x + br.width, br.y + br.height), colors[k], 2
            )
    cv2.imwrite(output_filename, orig)


if __name__ == "__main__":
    filename = "vd_p277.png"
    picture_results = find_pictures_with_captions(filename)
    draw_results(
        filename,
        picture_results.bounding_rects,
        picture_results.joined_rects,
        picture_results.connected_components,
        picture_results.colors,
    )