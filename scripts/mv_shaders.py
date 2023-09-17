#!/usr/bin/env python

# Shaders are defined in the shaders/ directory as .vert, .frag, etc. files. I would like to iterate over each file and convert it to .XXX.glsl files, where XXX is the name of the shader. For example, shaders/vert/vertex.vert would be converted to shaders/vert/vertex.vert.glsl. I would like to do this for all shaders in the shaders/ directory.

import os
import sys
import shutil


SUPPORTED_GLSL_EXTENSIONS = [".vert", ".tesc", ".tese", ".geom", ".frag", ".comp", ".rchit", ".rahit", ".rmiss", ".rint", ".rcall", ".rgen", ".task", ".mesh"]

# Get the path to the shaders/ directory
shaders_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "shaders")

# Iterate over each shader in the shaders/ directory
for shader in os.listdir(shaders_dir):
    ext = os.path.splitext(shader)[1]
    if ext in SUPPORTED_GLSL_EXTENSIONS:
        shutil.move(os.path.join(shaders_dir, shader), os.path.join(shaders_dir, shader + ".glsl"))