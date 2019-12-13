#include "lamp.h"

int interpolateLinear(int start, int end, double position) {
  int length = end - start;
  int offset = length * position;
  return start + offset;
}

int interpolateHue(int p1, int p2, double position) {
  int difference = p2 - p1;
  if (difference > 180) {
    difference -= 360;
  } else if (difference < -180) {
    difference += 360;
  }

  int start, end;
  double pos;

  if (difference > 0) {
    start = p1;
    end = p2;
    pos = position;
  } else {
    start = p2;
    end = p1;
    pos = 1 - position;
  }

  while (end < start) {
    end += 360;
  }

  int result = interpolateLinear(start, end, pos);
  return (result + 360) % 360;
}
