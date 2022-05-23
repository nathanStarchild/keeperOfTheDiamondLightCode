class Spiraliser {
    public:
        Spiraliser();
        void animate();
        bool enabled;
        void enable();
        void disable();
    private:
        uint8_t rep;
        uint8_t segments;
        uint8_t fibPow;
        uint8_t revs;
        uint8_t rfacInit;
        long fTime;
        float symmetry;
        float tf;
        float rfacDt;
        float rainbowRate;
        float rotRate;
        float repShift;
        boolean rotateOn;
        boolean clockwiseOn;
        boolean counterClockwiseOn;
        uint8_t hue;
        float rotate;
        uint8_t cShift;
        void draw(float xIn, float yIn, float s, uint8_t c);
        void restartIt();
};

Spiraliser::Spiraliser() {
    enabled = false;
    revs = 2;
    symmetry = 4;//0.74;
    segments = 19;
    fibPow = 6;
    rep = 3;
    rainbowRate = 1;//0.0024?
    tf = 7;
    repShift = 0.000001;
    rotateOn = true;
    rfacDt = 1/pow(PHI,12);
    rfacInit = 60;
    rotRate = 2;
    clockwiseOn = true;
    counterClockwiseOn = false;
    fTime = 0;
    rotate = 0;
    cShift = 0;
    void draw();
}

void Spiraliser::animate() {  
  if (fTime > 25 * 60 * 5) {
    restartIt();
  }
  float rfac = rfacInit + pow(fTime*rfacDt, 2);
  if (rotateOn) {
    rotate = fTime * rotRate * PI / 360;
  } else {
    rotate = 0;
  }
  for (int n=0; n<rep; n++) {
    rotate += 2 * PI / rep;
    hue = int(int(n*repShift+(fTime / rainbowRate)) % 256);
    for (int t=0; t<segments*revs; t++) {
      float r =  rfac * pow(fib, (-1 * symmetry * t/float(segments)));
      if (r >= 1 && r <= stripLength) {
        uint8_t c1 = int(hue+(t*tf))%256;
        uint8_t c2 = int(hue+(t*tf)+cShift)%256;
        float theta = t * 2 * PI / (segments);
        float theta2 = 2 * PI - theta;
        float r =  rfac * pow(fib, (-1 * symmetry * t/float(segments)));
        float s = r/pow(fib, fibPow);
        if (clockwiseOn) {
          draw(r*sin(theta+rotate), r*cos(theta+rotate), s, c1);
        }
        if (counterClockwiseOn) {
          draw(r*sin(theta2+rotate), r*cos(theta2+rotate), s, c2);
        }
      }
    }
  }
  
  if (frameCount % stepRate == 0) {
    fTime ++;// Next step.
  }
}

void Spiraliser::restartIt() { 
  fTime = 0;
}

void Spiraliser::draw(float xIn, float yIn, float s, uint8_t c) { 
  uint8_t spacing = 4;
  int minX = max(0, int(xIn + (nStrips * spacing / 2) - s));
  int maxX = min(nStrips * spacing, int(xIn + (nStrips * spacing / 2) + s));
  int minY = max(0, int(yIn + (stripLength / 2) - s));
  int maxY = min(int(stripLength), int(yIn + (stripLength / 2) + s));
  for (int X=minX; X<=maxX; X++) {
    if (X % spacing == 0) {
      for (int Y=minY; Y<=maxY; Y++) {
        int idx = nX(X/spacing, Y);
        leds[idx] = ColorFromPalette(currentPalette, c, 255);
      }
    }
  }
}

void Spiraliser::enable() {
  enabled = true;
}

void Spiraliser::disable() {
  enabled = false;
}
