from segm import join_rects
import cv2


def test_join():
    img = cv2.imread('vd_p214.png', cv2.IMREAD_GRAYSCALE)
    joined_rects = join_rects(img)
    assert (len(joined_rects) > 0)
    letter_e = [r for r in joined_rects if r.x == 2885 and r.y == 3755 and r.width == 33 and r.height == 43][0]
