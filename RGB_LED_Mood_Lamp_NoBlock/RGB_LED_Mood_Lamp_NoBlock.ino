/*
 RGB LED - Automatic Smooth Color Cycling - non blocking
 
 Marco Colli
 February 2018
 
 Uses the properties of the RGB Colour Cube
 The RGB colour space can be viewed as a cube of colour. If we assume a cube of dimension 1, then the 
 coordinates of the vertices for the cube will range from (0,0,0) to (1,1,1) (all black to all white).
 The transitions between each vertex will be a smooth colour flow and we can exploit this by using the 
 path coordinates as the LED transition effect. 
 */
#define  IDENTIFY_PIN  0
#define  RANDOM_DISPLAY  1
#define  ATTINY2313  0
#define  DEBUG_MODE  0

#if DEBUG_MODE
#define PRINT(s,v){ Serial.print(F(s)); Serial.print(v); }
#define PRINTS(s) { Serial.print(F(s)); }
#else
#define PRINT(s,v)
#define PRINTS(s)
#endif

#if ATTINY2313
// Output pins for PWM using ATTiny 2313 CPU
#define  R_PIN  13  // Red LED
#define  G_PIN  11  // Green LED
#define  B_PIN  12  // Blue LED
#else
// Output pins for PWM using Arduino Uno
#define  R_PIN  3  // Red LED
#define  G_PIN  5  // Green LED
#define  B_PIN  6  // Blue LED
#endif

// Constants for readability are better than magic numbers
// Used to adjust the limits for the LED, especially if it has a lower ON threshold
#define  MIN_RGB_VALUE  10   // no smaller than 0. 
#define  MAX_RGB_VALUE  255  // no bigger than 255.

// Slowing things down we need ...
#define  TRANSITION_DELAY  100   // in milliseconds, between individual light changes
#define  WAIT_DELAY        500   // in milliseconds, at the end of each traverse
//
// Total traversal time is ((MAX_RGB_VALUE - MIN_RGB_VALUE) * TRANSITION_DELAY) + WAIT_DELAY
// eg, ((255-0)*70)+500 = 18350ms = 18.35s

// Structure to contain a 3D coordinate
typedef struct
{
  uint8_t  x, y, z;
} 
coord;

static coord  v; // the current rgb coordinates (colour) being displayed

/*
 Vertices of a cube
 
 2+----------+7
 /|        / |
 3+---------+4 |
 | |       |  |    y   
 |1+-------|--+6   ^  7 z
 |/        | /     | /
 0+---------+5      +--->x
 
 */
const coord vertex[] = 
{
  //x  y  z      name
  { 0, 0, 0  }, // 0
  { 0, 0, 1  }, // 1
  { 0, 1, 1  }, // 2
  { 0, 1, 0  }, // 3
  { 1, 1, 0  }, // 4
  { 1, 0, 0  }, // 5
  { 1, 0, 1  }, // 6
  { 1, 1, 1  }  // 7
};

#define  MAX_VERTICES  (sizeof(vertex)/sizeof(coord))

#if  RANDOM_DISPLAY

uint16_t MD_Random(int nMax)
{
  uint32_t  n = millis(); 
  uint16_t  r = 0;

  while(n > 0)
  {  
    r = r + (n % 10);
    n = n/10;
  } 
  return(r%nMax);
}

#else

/*
 A list of vertex numbers encoded 2 per byte.
 Hex digits are used as vertices 0-7 fit nicely (3 bits 000-111) and have the same visual
 representation as decimal, so bytes 0x12, 0x34 ... should be interpreted as vertex 1 to 
 v2 to v3 to v4 (ie, one continuous path B to C to D to E).
 */
const uint8_t path[] =
{
  0x01, 0x23, 0x45, 0x67, 0x21, 0x65, 0x00,  // trace the edges
  0x26, 0x41, 0x35, 0x71, 0x25, 0x36, 0x14, 0x70,  // do the diagonals
};

#define  MAX_PATH_SIZE  (sizeof(path)/sizeof(path[0]))  // size of the array

#endif

void setup()
{
#if  DEBUG_MODE
  Serial.begin(57600);
#endif
  PRINTS("\n[Mood Lamp]\n");
#if RANDOM_DISPLAY
  PRINTS("Random");
#else
  PRINTS("Path");
#endif
  PRINTS(" traversal enabled");


  pinMode(R_PIN, OUTPUT);   // sets the pins as output
  pinMode(G_PIN, OUTPUT);  
  pinMode(B_PIN, OUTPUT);

#if  IDENTIFY_PIN
  // Useful when setting up to ensure that the R, G abd B are connected 
  // to the right processor pins
  digitalWrite(R_PIN, 1);
  delay(1000);
  digitalWrite(R_PIN, 0);
  digitalWrite(G_PIN, 1);
  delay(1000);
  digitalWrite(G_PIN, 0);
  digitalWrite(B_PIN, 1);
  delay(1000);
  digitalWrite(B_PIN, 0);
#endif

  // initialise the coordinates as the first vertex of the cube
  v.x = (vertex[0].x ? MAX_RGB_VALUE : MIN_RGB_VALUE);
  v.y = (vertex[0].y ? MAX_RGB_VALUE : MIN_RGB_VALUE);
  v.z = (vertex[0].z ? MAX_RGB_VALUE : MIN_RGB_VALUE);
}

bool traverse(int16_t _dx, int16_t _dy, int16_t _dz)
// Move along the colour line from where we are to the next vertex of the cube.
// The transition is achieved by applying the 'delta' value to the coordinate.
// By definition all the coordinates will complete the transition at the same 
// time as we only have one loop index.
// Return true when the traverse is completed
{
  static enum { S_INIT, S_LOOP, S_WAITTRANS, S_WAITDELAY } state = S_INIT;
  static uint32_t timeStart = 0, timeDelay = 0;
  static uint16_t idx;
  static int16_t  dx, dy, dz;

  // no point looping if we are staying in the same spot!
  if ((state == S_INIT) && (_dx == 0) && (_dy == 0) && (_dz == 0))
  {
    PRINT("\nNo transition dx=", _dx);
    PRINT(" dy=", _dy);
    PRINT(" dz=", _dz);
    return(true);
  }

  switch (state)
  {
    case S_INIT:
      {
        PRINTS("\nS_INIT");
        idx = 0;
        dx = _dx;
        dy = _dy;
        dz = _dz;
        state = S_LOOP;
      }
      break;

    case S_LOOP:
      {
        PRINTS("\nS_LOOP");
        if (idx >= MAX_RGB_VALUE-MIN_RGB_VALUE)
        {
          // at the end of the loop, wait for the specified time
          timeStart = millis();
          timeDelay = WAIT_DELAY;
          state = S_WAITDELAY;
        }
        else
        {
          // set the colour in the LED
          analogWrite(R_PIN, v.x);
          analogWrite(G_PIN, v.y);
          analogWrite(B_PIN, v.z);
  
          // increment all indices
          idx++;
          v.x += dx;
          v.y += dy;
          v.z += dz;

          // transition step delay for this transition
          timeStart = millis();
          timeDelay = TRANSITION_DELAY;
          state = S_WAITTRANS;
        }
      }
      break;

    case S_WAITTRANS:
    case S_WAITDELAY:
      {
        PRINT("\nS_WAIT ", timeDelay);
        if (millis() - timeStart >= timeDelay)
          state = (state == S_WAITDELAY) ? S_INIT : S_LOOP;
      }
      break;

    default:  // catch all, reset FSM
      state = S_INIT;
      break;
  }

  return(state == S_INIT);
}

void loop()
{
  static enum { S_LOOP, S_ANIMATE } state = S_LOOP;
  static uint8_t  v1, v2=0;    // the new vertex index and the previous one

  switch (state)
  {
    case S_LOOP:
    {
      v1 = v2;

      // how we select the point to go to depends on the display mode
#if  RANDOM_DISPLAY
      // pick the next vertex as a random one and 
      v2 = MD_Random(MAX_VERTICES);
#else
      // loop through the path, traversing from one point to the next
      static int  i = 0;  // the index for the defined path
    
      if (i++ >= 2*MAX_PATH_SIZE)
      { 
        i = 0;
    
        // (re)initialise the place we start from as the first vertex in the array
        v.x = (vertex[v2].x ? MAX_RGB_VALUE : MIN_RGB_VALUE);
        v.y = (vertex[v2].y ? MAX_RGB_VALUE : MIN_RGB_VALUE);
        v.z = (vertex[v2].z ? MAX_RGB_VALUE : MIN_RGB_VALUE);
      }
      
      // !! loop index is double what the path index is as it is a nybble index !!
      if (i&1)                  // odd number is the second element and ...
        v2 = path[i>>1] & 0xf;  // ... the bottom nybble (index /2) or ...
      else                      // ... even number is the first element and ...
        v2 = path[i>>1] >> 4;   // ... the top nybble
#endif
            
      PRINT("\nv[", v1);
      PRINT("] to v[", v2);
      PRINTS("]");
      state = S_ANIMATE;
    }
    break;
    
    case S_ANIMATE:
    {
      // trace the colour path and then loop repeat
      if (traverse(vertex[v2].x-vertex[v1].x, vertex[v2].y-vertex[v1].y, vertex[v2].z-vertex[v1].z))
        state = S_LOOP;
    }
    break;

    default:
      state = S_LOOP;
  }
}



