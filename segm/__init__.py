from ._segm import *

def find_glyphs(img):
    return get_glyphs(img, img.dtype)


def join_rects(img):
    return get_joined_rects(img, img.dtype)


def join_intervals(intervals):
    return get_joined_intervals(intervals)


def find_neighbors(glyphs, nn):
    return get_neighbors(glyphs, nn, nn.dtype)
