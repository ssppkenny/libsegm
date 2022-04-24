from segm import join_rects, join_intervals, glyph_result, find_neighbors
import cv2
import numpy as np


def test_join():
    img = cv2.imread('vd_p214.png', cv2.IMREAD_GRAYSCALE)
    joined_rects = join_rects(img)
    assert (len(joined_rects) > 0)
    letter_e = [r for r in joined_rects if r.x == 2885 and r.y == 3755 and r.width == 33 and r.height == 43][0]


def test_join_intervals():
    rngs = [(0,5,0), (2,3,1)]
    lst = join_intervals(rngs)
    assert(lst[0][0] == 0)
    assert(lst[0][1] == 5)


def test_find_neighbors():
    r1 = glyph_result((1, 1, 1, 1))
    r2 = glyph_result((3, 1, 1, 1))
    r3 = glyph_result((5, 1, 1, 1))
    r4 = glyph_result((1, 3, 1, 1))
    r5 = glyph_result((3, 3, 1, 1))
    r6 = glyph_result((5, 3, 1, 1))
    nn = np.array(
        [
            [0, 1, 3, 4, 2, 5],
            [1, 0, 2, 4, 3, 5],
            [2, 1, 5, 4, 0, 3],
            [3, 0, 4, 1, 5, 2],
            [4, 3, 5, 1, 0, 2],
            [5, 2, 4, 1, 3, 0],
        ]
    )

    d = find_neighbors([r1, r2, r3, r4, r5, r6], nn)
    assert d[2] is None
    assert d[5] is None
    assert d[0][1] == 1
    assert d[1][1] == 2
    assert d[3][1] == 4
    assert d[4][1] == 5
