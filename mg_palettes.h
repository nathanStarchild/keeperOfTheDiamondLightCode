// 0 — Pink — Participation
DEFINE_GRADIENT_PALETTE( participation_gp ) {
    0,   40,  0, 20,
    64, 180, 20,120,
    102, 64, 1, 34,
   220,200, 0,55,
   225,95,19,40,
   230, 90, 0, 67,
    255, 98, 2, 73,
};

// 1 — Turquoise — Radical Self Expression
DEFINE_GRADIENT_PALETTE( radicalSelfExpression_gp ) {
    0,   0, 40, 40,
    64,  0,120,100,
   128,  0,200,170,
   192, 0,133,122,
   255,180,255,255
};

// 2 — Brown — Leave No Trace
DEFINE_GRADIENT_PALETTE( leaveNoTrace_gp ) {
    0,  20, 10,  0,
    64, 16, 4, 2,
    72,  25, 12,  5,
    92, 17, 8, 2,
    112, 13, 2, 0,
    125, 30, 14, 3,
   128,120, 70, 30,
   129, 26, 12, 13,
   192,9,2, 1,
   255,25,12,3
};


// // 2 — Brown — Leave No Trace (earth → sunlit dust)
// DEFINE_GRADIENT_PALETTE( leaveNoTrace_gp ) {
//     0,  25, 12,  5,
//     64, 70, 40, 20,
//    128,140, 90, 50,
//    192,200,150, 90,
//    255,255,220,170
// };

// 3 — Red — Radical Inclusion
DEFINE_GRADIENT_PALETTE( radicalInclusion_gp ) {
    0,  40,  0,  0,
    64,120,  0,  0,
   128,255, 20,  0,
   192,255, 80, 20,
   255,255,0,0
};

// 4 — Orange — Gifting
DEFINE_GRADIENT_PALETTE( gifting_gp ) {
    0,  40, 10,  0,
    64,120, 20,  0,
   128,255,80,  0,
   192,59,37, 8,
   255,100,15,0
};

// 5 — Yellow — Decommodification
DEFINE_GRADIENT_PALETTE( decommodification_gp ) {
    0,  60, 40,  0,
    64,160,120,  0,
   128,255,200,  0,
   192,255,240,60,
   255,252,164,0
};

// 6 — Green — Radical Self Reliance
DEFINE_GRADIENT_PALETTE( radicalSelfReliance_gp ) {
    0,   0, 40,  0,
    64,  0,120,  0,
   128, 40,200, 20,
   192,100,255, 80,
   255,200,255,180
};

// 7 — Blue — Immediacy
DEFINE_GRADIENT_PALETTE( immediacy_gp ) {
    0,   0,  0, 40,
    64,  0, 40,120,
   128, 20,100,255,
   192,120,180,255,
   255,200,230,255
};

// 8 — Violet — Civic Responsibility
DEFINE_GRADIENT_PALETTE( civicResponsibility_gp ) {
    0,  20,  0, 40,
    64, 80,  0,120,
    98, 143, 0, 255,
   128,121, 0,134,
   192,58,20,97,
   255,127,0,255
};

// 9 — Indigo — Communal Effort
DEFINE_GRADIENT_PALETTE( communalEffort_gp ) {
    0,   8,  2, 24,
    64, 12,  0,20,
   128, 42, 0,213,
   192,59,18,89,
   210, 98, 0, 157,
   255,75,0,130
};



const uint8_t n_mg_palettes = 10;

const TProgmemRGBGradientPalettePtr mg_palettes[n_mg_palettes] = {
  participation_gp,
  radicalSelfExpression_gp,
  leaveNoTrace_gp,
  radicalInclusion_gp,
  gifting_gp,
  decommodification_gp,
  radicalSelfReliance_gp,
  immediacy_gp,
  civicResponsibility_gp,
  communalEffort_gp,
//   Sunset_Real_gp,
//   Dusk_Finds_Us_gp,
};

uint8_t principleHue[10] = {
 235, // Pink
 135, // Turquoise
  5, // Brown
   0, // Red
  18, // Orange
  52, // Yellow
 105, // Green
 164, // Blue
 206, // Violet
 176  // Indigo
};
