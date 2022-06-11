import cv2
from segm import find_clusters
import sys
from collections import defaultdict
import distinctipy


def get_distinct_colors(n):
    colors = distinctipy.get_colors(n, pastel_factor=0.7)
    colors = [(int(255 * a), int(255 * b), int(255 * c)) for a, b, c in colors]
    return colors

args = sys.argv

filename = args[1]

img = cv2.imread(filename, 0)
orig = cv2.imread(filename)


rects, clusters = find_clusters(img)

d = defaultdict(list)

for k, v in clusters.items():
    d[v].append(k)


colors = get_distinct_colors(len(d))

for i, r in enumerate(rects):
    cv2.rectangle(orig, (r.x, r.y), (r.x + r.width, r.y + r.height), colors[clusters[i]], 3)


cv2.imwrite("clusters.png", orig)


