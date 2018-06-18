"""
Script to generate arrays of RGB values to make rainbows.

Acknowledgment: <http://krazydad.com/tutorials/makecolors.php>
"""

import math

def make_color_gradient(freq1, freq2, freq3,
                        phase1, phase2, phase3,
                        center=128, width=127, len=50):
    for i in range(len):
        red = math.sin(freq1*i + phase1) * width + center
        green = math.sin(freq1*i + phase2) * width + center
        blue = math.sin(freq2*i + phase3) * width + center
        print "{ 0x%x, 0x%x, 0x%x }," % (red/4, green/4, blue/4)

if __name__ == "__main__":
    make_color_gradient(0.3, 0.3, 0.3, 0, 2*math.pi/3, 4*math.pi/3, len=18)
    print "// next!"
    make_color_gradient(0.3, 0.3, 0.3, 0, 2*math.pi/3, 4*math.pi/3, len=6)
    