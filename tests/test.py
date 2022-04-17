from segm import join_rects
import cv2


def test_join():
    img = cv2.imread('vd_p214.png')
    joined_rects = join_rects(img)
    assert (len(joined_rects) > 0)
