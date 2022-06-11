from ._segm import *

def find_glyphs(img):
    return get_glyphs(img, img.dtype)


def join_rects(img):
    return get_joined_rects(img, img.dtype)


def join_intervals(intervals):
    return get_joined_intervals(intervals)


def find_neighbors(glyphs, nn):
    return get_neighbors(glyphs, nn, nn.dtype)


def find_interword_gaps(glyphs):
    return get_interword_gaps(glyphs)


def find_bounding_rect(inds, glyphs):
    return get_bounding_rect(inds, glyphs)


def find_word_limits(interword_gaps, glyphs):
    return get_word_limits(interword_gaps, glyphs)


def find_baseline(glyphs):
    return get_baseline(glyphs)


def find_grouped_glyphs(glyphs):
    return get_grouped_glyphs(glyphs)


def find_bounding_rects_for_words(words):
    return get_bounding_rects_for_words(words)


def find_ordered_glyphs(joined_rects):
    return get_ordered_glyphs(joined_rects, joined_rects.dtype)

def find_clusters(img):
    return get_clusters(img, img.dtype)
