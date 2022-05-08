import time

import cv2
from segm import find_bounding_rects_for_words as bounding_rects_for_words
from segm import find_ordered_glyphs
from segm import glyph_result

if __name__ == "__main__":
    filename = "vd_p214.png"
    orig = cv2.imread(filename)
    h, w, _ = orig.shape
    img = cv2.imread(filename, 0)
    pc = time.perf_counter()
    lines_of_words = find_ordered_glyphs(img)
    print(time.perf_counter() - pc)
    for words in lines_of_words:
        for word in words:
            for glyph in word:
                cv2.line(
                    orig,
                    (glyph.x, glyph.y + glyph.height + int(glyph.shift)),
                    (glyph.x + glyph.width, glyph.y + glyph.height + int(glyph.shift)),
                    (0, 0, 255),
                    2,
                )
        rects = bounding_rects_for_words(
            [
                [glyph_result((w.x, w.y, w.width, w.height, w.shift)) for w in word]
                for word in words
            ]
        )
        for gl in rects:
            cv2.rectangle(
                orig, (gl.x, gl.y), (gl.x + gl.width, gl.y + gl.height), (0, 255, 0), 2
            )

    cv2.imwrite("segmented.png", orig)
