typedef struct {
  int brightness;
  int saturation;
  int hue;
  bool on;
} Lamp;

int interpolateLinear(int start, int end, double position);
int interpolateHue(int start, int end, double position);
