#ifndef CONFIG_HH
#define CONFIG_HH

/* Fill in your student ID here. */
#define STUDENT_ID 152121358
/* For the final run on the real supercomputer, comment out the following line. */
#define TESTING 1

#ifdef TESTING
/* You can change these freely, but it's probably easiest to discuss timings
 * with others if everyone keeps the same values. In each of these, smaller
 * numbers render faster.
 */
#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 360
#define SAMPLES_PER_PIXEL 256
#define FRAMERATE 30
#define MAX_BOUNCES 4
#else
/* DO NOT TOUCH THESE, THEY'RE THE TESTED PRODUCTION SETTINGS! */
#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080
#define SAMPLES_PER_PIXEL 1024
#define FRAMERATE 30
#define MAX_BOUNCES 5
#endif

/* DO NOT TOUCH THESE, THEY'RE COMMON SETTINGS */
#define SAMPLES_PER_MOTION_BLUR_STEP 8
#define MIN_RAY_DIST 1e-4f
#define MAX_RAY_DIST 1e9f
#define PATH_SPACE_REGULARIZATION_GAMMA 0.15f

#define EARTH_RADIUS 6.3781e6f
#define ATMOSPHERE_PRIMARY_ITERATIONS 8
#define ATMOSPHERE_SECONDARY_ITERATIONS 4
#define ATMOSPHERE_HEIGHT 1.0e5f
#define ATMOSPHERE_RAYLEIGH_COEFFICIENT (float3){5.8e-6f, 13.6e-6f, 33.1e-6f}
#define ATMOSPHERE_RAYLEIGH_SCALE_HEIGHT 7994.0f
#define ATMOSPHERE_MIE_COEFFICIENT (float3){4.0e-6f, 4.0e-6f, 4.0e-6f}
#define ATMOSPHERE_MIE_ANISOTROPY 0.80f
#define ATMOSPHERE_MIE_SCALE_HEIGHT 1200.0f

#endif
