#%%
import cv2
import matplotlib.pyplot as plt
import numpy as np

#%%
sz = 35 # size of area to calculate standard deviation
image = cv2.imread(image_name)
image_float = image.astype('float') / 255.0
image_blurred = cv2.GaussianBlur(image_float, [sz, sz], float(sz) / 3.0)
image_std = cv2.sqrt((image_float - image_blurred) ** 2)
image_std = cv2.max(cv2.max(image_std[:,:,0], image_std[:,:,1]), image_std[:,:,2])
image_std = (255 * image_std).astype('uint8')
if (image.shape[2] == 3):
    image_ = np.zeros((image.shape[0], image.shape[1], 4))
    image_[:,:,0:3] = image
    image_[:,:,3] = image_std
    image = image_
cv2.imwrite(image_out_name, image)
# %%
