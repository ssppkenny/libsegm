from segm import join_rects
import cv2
import sys



filenames = ["vd_p214.png"]

for filename in filenames:
    img = cv2.imread(filename, 0)
    orig = cv2.imread(filename)

    jr = join_rects(img)

    for i, j in enumerate(jr):
        cv2.rectangle(orig, (j.x, j.y), (j.x + j.width, j.y + j.height), (255,0,0), 2)

    cv2.imwrite(filename + "_out.png", orig)

