import copy
from collections import defaultdict
from typing import Tuple, List, Dict, Set, Any

import cv2
import intervaltree
import networkx as nx
from pyflann import *
from segm import join_rects, glyph_result, join_intervals


class Glyph:
    def __init__(self, gr, baseline):
        self.baseline_shift = baseline - gr.y
        self.x = gr.x
        self.y = gr.y
        self.width = gr.width
        self.height = gr.height

    @staticmethod
    def from_glyph_results(
        glyph_results: List[glyph_result], baseline: float
    ) -> List["Glyph"]:
        return [Glyph(gr, baseline) for gr in glyph_results]

    def __str__(self) -> str:
        return f"x = {self.x}, y = {self.y}, width = {self.width}, height = {self.height}, shift = {self.baseline_shift}"


def find_baseline(glyphs: List[glyph_result]) -> float:
    lowers = [g.y + g.height for g in glyphs]
    hist, bin_edges = np.histogram(lowers, bins=50)
    maxind = np.argmax(hist)
    return bin_edges[maxind]


def all_pairs(s: Set[Any]) -> List[Tuple[Any, Any]]:
    lst = list(s)
    return [(a, b) for idx, a in enumerate(lst) for b in lst[idx + 1 :]]


def find_neighbors(
    jr: List[glyph_result], nearest_neighbors: np.ndarray
) -> Dict[int, Tuple[glyph_result, int, float]]:
    """
    :param jr:
    :param nearest_neighbors:
    :return: a dict with key = index of the rectangle and value is index of its neighbor to the right
    """
    ret_val = dict([(i, None) for i, _ in enumerate(jr)])

    for j in range(len(jr)):
        r = jr[j]
        nbs = nearest_neighbors[j, :]
        right_nbs = []
        for i in nbs[1:]:
            nb = jr[i]
            a, b = (r.y, r.y + r.height)
            c, d = (nb.y, nb.y + nb.height)
            left, right = max(a, c), min(b, d)
            if left < right and nb.x > r.x:
                right_nbs.append((nb, i, (right - left) / nb.height))
        right_nbs = sorted(right_nbs, key=lambda x: x[0].x / x[2])
        # right_nbs = sorted(right_nbs, key=lambda x: x[0].x)
        if len(right_nbs) > 0:
            ret_val[j] = right_nbs[0]

    return ret_val


def word_limits(
    interword_gaps: Dict[int, int], glyphs: List[glyph_result]
) -> List[List[glyph_result]]:
    glyphs = copy.copy(glyphs)
    arr = list(interword_gaps.keys())
    words = []
    if len(arr) == 0:
        words.append(copy.copy(glyphs[:]))
        return words

    limits = [arr[0]]
    for i, el in enumerate(arr):
        if i != len(arr) - 1:
            limits.append(arr[i + 1] - el)
        else:
            limits.append(len(glyphs) - el)

    for l in limits:
        words.append(copy.copy(glyphs[:l]))
        glyphs[:l] = []

    return words


def group_words(glyphs: List[glyph_result]) -> Dict[int, int]:
    av_height = np.mean([g.height for g in glyphs])
    gaps = []
    interword_gaps = dict()
    for i in range(1, len(glyphs)):
        gc = glyphs[i]
        gp = glyphs[i - 1]
        gap = gc.x - (gp.x + gp.width)
        gaps.append(gap)
        if gap > 0.5 * av_height:
            interword_gaps[i] = gap
    return interword_gaps


def group_glyphs(glyphs: List[glyph_result]) -> List[glyph_result]:
    numbered_glyphs = defaultdict(list)
    for i, g in enumerate(glyphs):
        numbered_glyphs[(g.x, g.x + g.width)].append(i)
    t = intervaltree.IntervalTree()
    d = dict([(i, (c.x, c.x + c.width, i)) for i, c in enumerate(glyphs)])
    intervals = list(d.values())
    for intr in intervals:
        t.add(intervaltree.Interval(intr[0], intr[1]))
    glyph_limits = join_intervals(intervals)
    new_rects = []
    for gl in glyph_limits:
        x = t.overlap(gl[0], gl[1])
        all_inds = set()
        for intr in x:
            inds = numbered_glyphs[(intr.begin, intr.end)]
            all_inds.update(inds)
        maxx, maxy, minx, miny = bounding_rect(all_inds, glyphs)
        new_rects.append(glyph_result((minx, miny, maxx - minx, maxy - miny)))

    return new_rects


def bounding_rect(
    all_inds: Set[int], glyphs: List[glyph_result]
) -> Tuple[int, int, int, int]:
    minx = float("inf")
    maxx = 0
    miny = float("inf")
    maxy = 0
    for ind in all_inds:
        g = glyphs[ind]
        if g.x < minx:
            minx = g.x
        if g.x + g.width > maxx:
            maxx = g.x + g.width
        if g.y < miny:
            miny = g.y
        if g.y + g.height > maxy:
            maxy = g.y + g.height
    return maxx, maxy, minx, miny


def find_ordered_glyphs(
    filename: str,
) -> Tuple[List[glyph_result], List[float], List[List[List[Glyph]]]]:
    """
    finds all rectangles
    :param filename: image filename
    :return:
    """
    img = cv2.imread(filename, 0)
    jr = join_rects(img)
    tree = intervaltree.IntervalTree()
    centroids = np.zeros((len(jr), 2))
    for i, r in enumerate(jr):
        centroids[i, :] = np.array([r.x + r.width / 2, r.y + r.height / 2])

    flann = FLANN()
    params = flann.build_index(centroids, algorithm="kdtree", trees=4)
    nearest_neighbors, dists = flann.nn_index(centroids, 20, checks=params["checks"])
    neighbors = find_neighbors(jr, nearest_neighbors)
    g = nx.Graph()
    for k, v in neighbors.items():
        if v:
            g.add_edge(k, v[1])
        else:
            g.add_node(k)

    cc = [x for x in nx.connected_components(g)]
    line_limits = []
    for i, c in enumerate(cc):
        heights1 = [(jr[x].y, jr[x].y + jr[x].height) for x in c]
        min1, _ = min(heights1, key=lambda x: x[0])
        _, max1 = max(heights1, key=lambda x: x[1])
        line_limits.append((min1, max1, i))

    line_limits = join_intervals(line_limits)
    for ll in line_limits:
        tree.add(intervaltree.Interval(ll[0], ll[1]))

    lines = defaultdict(list)
    for r in jr:
        rt = tree[r.y]
        interval = list(rt)[0]
        lines[interval].append(r)

    lines = dict(sorted(lines.items(), key=lambda x: x[0].begin))
    new_lines = dict()
    baselines = []
    lines_of_words = []
    for k, v in lines.items():
        grouped_glyphs = group_glyphs(v)
        lower = find_baseline(grouped_glyphs)
        baselines.append(lower)
        all_glyphs = sorted(grouped_glyphs, key=lambda x: x[0])
        interword_gaps = group_words(all_glyphs)
        words = word_limits(interword_gaps, all_glyphs)
        ##aaa = [Glyph.from_glyph_results(word, lower) for word in words]
        lines_of_words.append([Glyph.from_glyph_results(word, lower) for word in words])
        new_lines[k] = all_glyphs

    t = [v for k, v in new_lines.items()]
    flat_list = [item for sublist in t for item in sublist]
    return flat_list, baselines, lines_of_words


def bounding_rects_for_words(words: List[List[Glyph]]) -> List[glyph_result]:
    new_rects = []
    for w in words:
        maxx, maxy, minx, miny = bounding_rect(range(len(w)), w)
        new_rects.append(glyph_result((minx, miny, maxx - minx, maxy - miny)))
    return new_rects


if __name__ == "__main__":
    filename = "funcan_p15.png"
    orig = cv2.imread(filename)
    h, w, _ = orig.shape
    glyphs, baselines, lines_of_words = find_ordered_glyphs(filename)

    for words in lines_of_words:
        for word in words:
            for glyph in word:
                cv2.line(
                    orig,
                    (glyph.x, glyph.y + int(glyph.baseline_shift)),
                    (glyph.x + glyph.width, glyph.y + int(glyph.baseline_shift)),
                    (0, 0, 255),
                    2,
                )
        rects = bounding_rects_for_words(words)
        for gl in rects:
            cv2.rectangle(
                orig, (gl.x, gl.y), (gl.x + gl.width, gl.y + gl.height), (0, 255, 0), 2
            )

    # counter = 1
    # font = cv2.FONT_HERSHEY_SIMPLEX
    # for gl in glyphs:
    #     cv2.rectangle(
    #         orig, (gl.x, gl.y), (gl.x + gl.width, gl.y + gl.height), (255, 0, 0), 2
    #     )
    #     cv2.putText(
    #         orig, str(counter), (gl.x, gl.y), font, 1, (255, 0, 0), 2, cv2.LINE_AA
    #     )
    #     counter += 1
    #
    # for l in baselines:
    #    cv2.line(orig, (0, int(l)), (w, int(l)), (0, 0, 255), 2)

    cv2.imwrite("segmented.png", orig)
