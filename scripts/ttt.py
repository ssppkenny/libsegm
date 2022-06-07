from segm import join_rects
import cv2

img = cv2.imread("msf_p20.png", 0)
orig = cv2.imread("msf_p20.png")

jr = join_rects(img)

print(len(jr))

cl = [348,374,393,420,440,462,481,502,537,556,581,598,616,633,660,692,709,735,775,798,837,852,879,930,971,980,988]

for i, j in enumerate(jr):
    #if i in cl:
    cv2.rectangle(orig, (j.x, j.y), (j.x + j.width, j.y + j.height), (255,0,0), 2)

cv2.imwrite("cl.png", orig)

