#!/bin/bash
cd "$(dirname "$0")"
g++ -O2 -o hexacraft main.cpp glad.c -Iopengl/include -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm && ./hexacraft
