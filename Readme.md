# Improved page segementation
## segm module is implemented in C++


```python
from segm import join_rects
import matplotlib.pyplot as plt
import cv2

def segment_image(filename):
    img = cv2.imread(filename, 0)
    orig = cv2.imread(filename)
    jr = join_rects(img)
    for i, r in enumerate(jr):
        cv2.rectangle(orig,(r.x,r.y),(r.x + r.width,r.y + r.height),(0,255,0),2)

    return orig
    
filename = 'libsegm/vd_p122.png'

img = segment_image(filename)

plt.rcParams['figure.figsize'] = [20, 20]
plt.imshow(img)
```




    <matplotlib.image.AxesImage at 0x7feb138f58b0>




    
![png](output_1_1.png)
    



```python

```
