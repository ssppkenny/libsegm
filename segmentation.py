import time
from collections import defaultdict
from typing import Tuple, List

import cv2
import intervaltree
import networkx as nx
from pyflann import *
from segm import find_baseline
from segm import find_bounding_rects_for_words as bounding_rects_for_words
from segm import find_grouped_glyphs as group_glyphs
from segm import find_interword_gaps as group_words
from segm import find_word_limits as word_limits
from segm import join_intervals, find_neighbors
from segm import join_rects, glyph_result


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
    t = time.perf_counter()
    neighbors = find_neighbors(jr, nearest_neighbors)
    print(time.perf_counter() - t)
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
        lines_of_words.append([Glyph.from_glyph_results(word, lower) for word in words])
        new_lines[k] = all_glyphs

    t = [v for k, v in new_lines.items()]
    flat_list = [item for sublist in t for item in sublist]
    return flat_list, baselines, lines_of_words


if __name__ == "__main__":
    filename = "math21_1_23.png"
    filename = "fa_p18.png"
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
        rects = bounding_rects_for_words(
            [
                [glyph_result((w.x, w.y, w.width, w.height)) for w in word]
                for word in words
            ]
        )
        ##rects = bounding_rects_for_words(words)
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
