from pyflann import *
import numpy as np
import cv2
import networkx as nx
import intervaltree
from collections import defaultdict


from segm import join_rects

def all_pairs(s):
    lst = list(s)
    return [(a, b) for idx, a in enumerate(lst) for b in lst[idx + 1:]]


def join_intervals(rngs):
    """
    join overlapping intervals
    :param rngs: list of intervals
    :return: joined not overlapping intervals
    """
    left_events = [(x[0], 0, x, x[2]) for x in rngs]
    right_events = [(x[1], 1, x, x[2]) for x in rngs]
    sorted_events = sorted(left_events + right_events)
    queue = dict()
    intersecting = set()
    g = nx.Graph()
    for i in range(len(rngs)):
        g.add_node(i)
    for x, t, r, i in sorted_events:
        if t == 0:
            queue[r] = i
            intersecting.add(r)
        else:
            del queue[r]
            for a,b in all_pairs(intersecting):
                g.add_edge(a[2],b[2])
            if len(queue) == 0:
                intersecting = set()

    cc = [x for x in nx.connected_components(g)]

    new_rngs = []
    for c in cc:
        min1 = float('inf')
        max1 = 0
        for i in c:
            mn =rngs[i][0]
            mx = rngs[i][1]
            if mn < min1:
                min1 = mn
            if mx > max1:
                max1 = mx
        new_rngs.append((min1, max1))

    return  new_rngs


def find_neighbors(jr, nearest_neighbors):
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
                right_nbs.append((nb, i))
            right_nbs = sorted(right_nbs, key=lambda x: x[0].x)
        if len(right_nbs) > 0:
            ret_val[j] = right_nbs[0]

    return ret_val


def find_ordered_glyphs(filename):
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
    params = flann.build_index(centroids,
                               algorithm='kdtree',
                               trees=4
                               )
    nearest_neighbors, dists = flann.nn_index(centroids, 20, checks=params['checks'])
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
    for k, v in lines.items():
        new_lines[k] = sorted(v, key=lambda x: x.x)

    t = [v for k,v in new_lines.items()]
    flat_list = [item for sublist in t for item in sublist]
    return flat_list



if __name__ == '__main__':
    filename = 'vd_p108.png'
    orig = cv2.imread(filename)
    glyphs = find_ordered_glyphs(filename)
    counter = 1
    font = cv2.FONT_HERSHEY_SIMPLEX
    for gl in glyphs:
        cv2.rectangle(orig, (gl.x, gl.y), (gl.x + gl.width, gl.y + gl.height), (255,0,0), 2)
        cv2.putText(orig, str(counter), (gl.x, gl.y), font, 1, (255,0,0), 2, cv2.LINE_AA)
        counter += 1

    cv2.imwrite('test1.png', orig)






    # orig = cv2.imread(filename)
    # h, w, c = orig.shape
    # for s,e in rngs:
    #     cv2.line(orig, (0,s), (w,s), (255,0,0), 2)
    #     cv2.line(orig, (0,e), (w,e), (255,0,0), 2)
    #
    # cv2.imwrite('test.png', orig)








