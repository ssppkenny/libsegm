from segm import join_rects
import cv2
import sys



filenames = ['kf2_p126.png', 'kf2_p18.png', 'kf2_p30.png', 'kf2_p344.png', 'kf2_p571.png', 'kf2_p83.png', 'kf_p15.png', 'lm_p45.png', 'lm_p46.png', 'lm_p47.png', 'mf_p13.png', 'mf_p15.png', 'mf_p7.png', 'mf_p8.png', 'mm_p129.png', 'mm_p15.png', 'mm_p19.png', 'msf_p101.png', 'msf_p105.png', 'msf_p106.png', 'msf_p107.png', 'msf_p108.png', 'msf_p109.png', 'msf_p110.png', 'msf_p112.png', 'msf_p12.png', 'msf_p132.png', 'msf_p134.png', 'msf_p137.png', 'msf_p14.png', 'msf_p15.png', 'msf_p20.png', 'msf_p21.png', 'msf_p22.png', 'msf_p24.png', 'msf_p26.png', 'msf_p28.png', 'msf_p29.png', 'msf_p31.png', 'msf_p33.png', 'msf_p34.png', 'msf_p352.png', 'msf_p37.png', 'msf_p38.png', 'msf_p39.png', 'msf_p53.png', 'msf_p57.png', 'msf_p72.png', 'msf_p73.png', 'msf_p74.png', 'msf_p77.png', 'msf_p86.png', 'msf_p93.png', 'vd_p214.png', 'vd_p277.png']

for filename in filenames:
    img = cv2.imread(filename, 0)
    orig = cv2.imread(filename)

    jr = join_rects(img)

    for i, j in enumerate(jr):
        cv2.rectangle(orig, (j.x, j.y), (j.x + j.width, j.y + j.height), (255,0,0), 2)

    cv2.imwrite(filename + "_out.png", orig)

