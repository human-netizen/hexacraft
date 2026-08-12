#!/bin/bash
cd "$(dirname "$0")"
g++ -O2 -o hexacraft src/main.cpp src/glad.c -Ithird_party/opengl/include -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm && ./hexacraft
