/************************************************************
 * Fullstendig kode:
 *  - Leser en 22-tegns binærstreng fra Serial (1 = aktiv stjerne)
 *  - Bygger en dynamisk naboliste (adjacency) fra LEDMapping-arrayet
 *  - Bruker BFS med en egendefinert kø for å finne sammenhengende aktive
 *    stjerner (komponenter)
 *  - Tennler alle linjene (NeoPixel-piksler) for de koblingene der begge
 *    endene er i en komponent med ≥ 3 aktive stjerner.
 ************************************************************/

#include <Adafruit_NeoPixel.h>

// *********************
// Egendefinert kø (IntQueue) for BFS – maks 22 noder
// *********************
#define QUEUE_SIZE 22

struct IntQueue {
  int data[QUEUE_SIZE];
  int head;
  int tail;
};

void initQueue(IntQueue* q) {
  q->head = 0;
  q->tail = 0;
}

bool isQueueEmpty(IntQueue* q) {
  return (q->head == q->tail);
}

bool pushQueue(IntQueue* q, int value) {
  int nextTail = (q->tail + 1) % QUEUE_SIZE;
  if (nextTail == q->head) {
    // Køen er full – for 22 noder skal dette ikke skje
    return false;
  }
  q->data[q->tail] = value;
  q->tail = nextTail;
  return true;
}

int popQueue(IntQueue* q) {
  int value = q->data[q->head];
  q->head = (q->head + 1) % QUEUE_SIZE;
  return value;
}

// *********************
// NeoPixel-oppsett
// *********************
#define NEOPIXEL_PIN 6     
#define NUM_PIXELS   644   
Adafruit_NeoPixel strip(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// *********************
// Definisjoner for stjerner og koblinger
// *********************
#define NUM_SPOTS 22    // Totalt antall stjerner (noder)

// LEDMapping definerer en kobling mellom to noder og hvilke NeoPixel-piksler
// som skal lyse opp for denne linjen.
#define MAX_LEDS_PER_MAPPING 20
struct LEDMapping {
  int startSpot;    // f.eks. 0 for plass 1
  int endSpot;      // f.eks. 1 for plass 2
  int numLEDs;
  int ledPixels[MAX_LEDS_PER_MAPPING];
};

// Vi bruker her 39 koblinger (de med "SKIP" utelatt) fra Excel-arket.
// Merk: Plassene i Excel er 1-indeksert – vi trekker fra 1 for 0-indeksering.
#define NUM_LED_MAPPINGS 39
LEDMapping ledMappings[NUM_LED_MAPPINGS] = {
  {0, 1, 13, {3,4,5,6,7,8,9,10,11,12,13,14,15}},
  {1, 2, 13, {20,21,22,23,24,25,26,27,28,29,30,31,32,33}},
  {2, 3, 13, {37,38,39,40,41,42,43,44,45,46,47,48,49,50}},
  {3, 8, 9,  {54,55,56,57,58,59,60,61,62,63,64}},
  {8, 7, 13, {70,71,72,73,74,75,76,77,78,79,80,81,82,83,84}},
  {7, 6, 13, {88,89,90,91,92,93,94,95,96,97,98,99,100}},
  {6, 5, 13, {105,106,107,108,109,110,111,112,113,114,115,116,117}},
  {5, 4, 13, {122,123,124,125,126,127,128,129,130,131,132,133,134}},
  {4, 9, 9,  {141,142,143,144,145,146,147,148,149}},
  {9, 10, 13, {154,155,156,157,158,159,160,161,162,163,164,165,166}},
  {10, 11, 12, {170,171,172,173,174,175,176,177,178,179,180,181,182,183}},
  {11, 12, 12, {187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202}},
  {12, 17, 8,  {206,207,208,209,210,211,212,213,214,215}},
  {17, 16, 12, {222,223,224,225,226,227,228,229,230,231,232}},
  {16, 15, 13, {237,238,239,240,241,242,243,244,245,246,247,248,249}},
  {15, 14, 13, {254,255,256,257,258,259,260,261,262,263,264,265,266}},
  {14, 13, 11, {270,271,272,273,274,275,276,277,278,279,280,281}},
  {13, 18, 7,  {285,286,287,288,289,290,291,292,293,294}},
  {18, 19, 11, {298,299,300,301,302,303,304,305,306,307,308,309,310}},
  {19, 20, 12, {313,314,315,316,317,318,319,320,321,322,323,324}},
  {20, 21, 12, {329,330,331,332,333,334,335,336,337,338,339,340,341}},
  {21, 17, 7,  {346,347,348,349,350,351,352,353,354}},
  {17, 8, 14, {359,360,361,362,363,364,365,366,367,368,369,370,371,372}},
  {8, 12, 8,  {379,380,381,382,383,384,385,386}},
  {12, 16, 7,  {391,392,393,394,395,396,397,398,399}},
  {16, 20, 8,  {404,405,406,407,408,409,410,411,412}},
  {19, 15, 8,  {433,434,435,436,437,438,439,440}},
  {15, 11, 8,  {445,446,447,448,449,450,451,452,453}},
  {11, 7, 8,  {458,459,460,461,462,463,463,464,465,466}},
  {7, 3, 9,  {470,471,472,473,474,475,476,477,478}},
  {2, 6, 8,  {502,503,504,505,506,507,508,509,510,511,512}},
  {6, 10, 8, {516,517,518,519,520,521,522,523}},
  {10, 14, 8, {528,529,530,531,532,533,534,535,536,537}},
  {14, 18, 6, {540,541,542,543,544,545,546,547,548}},
  {13, 9, 8,  {565,566,567,568,569,570,571,572,573}},
  {9, 5, 8,  {578,579,580,581,582,583,584,585}},
  {5, 1, 8,  {590,591,592,593,594,595,596,597}},
  {0, 4, 8,  {620,621,622,623,624,625,626,627}},
  {4, 13, 14,{633,634,635,636,637,638,639,640,641,642,643,644,645,646,647,648,649,650,651}}
};

// *********************
// Bygg en dynamisk adjacency-liste fra LEDMapping
// *********************
#define MAX_NEIGHBORS 10
int neighborList[NUM_SPOTS][MAX_NEIGHBORS];
int neighborCount[NUM_SPOTS];

void buildAdjacencyFromMappings() {
  // Nullstill nabolisten
  for (int i = 0; i < NUM_SPOTS; i++) {
    neighborCount[i] = 0;
    for (int j = 0; j < MAX_NEIGHBORS; j++) {
      neighborList[i][j] = -1;
    }
  }
  // Legg til naboer basert på hver LEDMapping
  for (int i = 0; i < NUM_LED_MAPPINGS; i++) {
    int a = ledMappings[i].startSpot;
    int b = ledMappings[i].endSpot;
    
    // Legg til b som nabo for a hvis den ikke allerede finnes
    bool exists = false;
    for (int j = 0; j < neighborCount[a]; j++) {
      if (neighborList[a][j] == b) { exists = true; break; }
    }
    if (!exists && neighborCount[a] < MAX_NEIGHBORS) {
      neighborList[a][neighborCount[a]++] = b;
    }
    
    // Legg til a som nabo for b
    exists = false;
    for (int j = 0; j < neighborCount[b]; j++) {
      if (neighborList[b][j] == a) { exists = true; break; }
    }
    if (!exists && neighborCount[b] < MAX_NEIGHBORS) {
      neighborList[b][neighborCount[b]++] = a;
    }
  }
}

// *********************
// BFS-funksjon for å finne sammenhengende aktive komponenter
// *********************
void bfsComponent(int startNode, int compID,
                  const bool occupied[],
                  int componentIndex[]) {
  // Bruk vår egendefinerte kø
  IntQueue q;
  initQueue(&q);
  pushQueue(&q, startNode);
  componentIndex[startNode] = compID;
  
  while (!isQueueEmpty(&q)) {
    int current = popQueue(&q);
    for (int i = 0; i < neighborCount[current]; i++) {
      int neighbor = neighborList[current][i];
      if (occupied[neighbor] && componentIndex[neighbor] == -1) {
        componentIndex[neighbor] = compID;
        pushQueue(&q, neighbor);
      }
    }
  }
}

// Array som holder gjeldende lysstyrke (0-255) for hver piksel (twinkling)
uint8_t brightness[NUM_PIXELS] = {0};

// For å "overstyre" twinklingen på piksler som skal tennes av Serial-kommando
bool connectionOverride[NUM_PIXELS] = { false };

// Tidsstyring for non-blocking oppdatering av twinkling
unsigned long previousMillis = 0;
const unsigned long interval = 20;  // Oppdateringsintervall i ms

// *********************
// Setup og Loop
// *********************
void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Slå av alle piksler ved oppstart
  buildAdjacencyFromMappings();  // Bygg nabolisten fra LEDMapping
  randomSeed(analogRead(0));
  Serial.println("System klargjort. Skriv inn en 22-tegns streng (f.eks. 101000...):");
}

void loop() {
  unsigned long currentMillis = millis();


  if (Serial.available() > 0) {
    Serial.println("SLO UT");


  //____________
    // --- Non-blocking twinkling-effekt på de pikslene som IKKE er overstyrt ---
  if (currentMillis - previousMillis >= interval) {
    
    previousMillis = currentMillis;
    for (int i = 0; i < NUM_PIXELS; i++) {
      // Kun oppdater de pikslene som IKKE er overstyrt av Serial-mapping
      if (!connectionOverride[i]) {
        if (brightness[i] == 0) {
          // Ca. 0,5% sjanse for å starte blink (full lysstyrke)
          if (random(0, 1000) < 5) {
            brightness[i] = 255;
          }
        } else {
          if (brightness[i] > 10) {
            brightness[i] -= 10;
          } else {
            brightness[i] = 0;
          }
        }
        strip.setPixelColor(i, strip.Color(brightness[i], brightness[i], brightness[i]));
      }
    }
  }


    // 1) Les en 22-tegns binærstreng fra Serial
    String occupancyString = Serial.readStringUntil('\n');
    occupancyString.trim();
    while (occupancyString.length() < NUM_SPOTS) {
      occupancyString += "0";
    }
    
    // Konverter til boolsk array: true = aktiv stjerne
    bool occupied[NUM_SPOTS];
    for (int i = 0; i < NUM_SPOTS; i++) {
      occupied[i] = (occupancyString.charAt(i) == '1');
    }
    
    // 2) Finn sammenhengende aktive komponenter med BFS
    int componentIndex[NUM_SPOTS];
    for (int i = 0; i < NUM_SPOTS; i++) {
      componentIndex[i] = -1;
    }
    int compCount = 0;
    for (int node = 0; node < NUM_SPOTS; node++) {
      if (occupied[node] && componentIndex[node] == -1) {
        bfsComponent(node, compCount, occupied, componentIndex);
        compCount++;
      }
    }
    
    // 3) Tell antall noder i hver komponent
    int componentSize[compCount];
    for (int c = 0; c < compCount; c++) {
      componentSize[c] = 0;
    }
    for (int node = 0; node < NUM_SPOTS; node++) {
      if (componentIndex[node] != -1) {
        componentSize[componentIndex[node]]++;
      }
    }
    
    // 4) Oppdater LED-linjene (koblingene) basert på BFS-resultatene.
    // Tenn en kobling dersom:
    // - Begge enden er aktive.
    // - Begge enden er i samme komponent.
    // - Komponenten har minst 3 aktive stjerner.
    for (int i = 0; i < NUM_LED_MAPPINGS; i++) {
      int s = ledMappings[i].startSpot;
      int e = ledMappings[i].endSpot;
      // Sørg for at s er mindre enn e (for konsistens)
      if (s > e) { int tmp = s; s = e; e = tmp; }
      
      if (!occupied[s] || !occupied[e]) {
        // Slå av LED-ene for denne koblingen
        for (int j = 0; j < ledMappings[i].numLEDs; j++) {
          strip.setPixelColor(ledMappings[i].ledPixels[j], 0);
        }
        continue;
      }
      
      int compS = componentIndex[s];
      int compE = componentIndex[e];
      if (compS == -1 || compE == -1 || compS != compE) {
        for (int j = 0; j < ledMappings[i].numLEDs; j++) {
          strip.setPixelColor(ledMappings[i].ledPixels[j], 0);
        }
        continue;
      }
      
      if (componentSize[compS] >= 3) {
        // Tenn koblingslinjen (bruk hvit farge her)
        for (int j = 0; j < ledMappings[i].numLEDs; j++) {
          strip.setPixelColor(ledMappings[i].ledPixels[j], strip.Color(60,60,60));
        }
      } else {
        // Mindre enn 3 aktive stjerner – slukk linjen
        for (int j = 0; j < ledMappings[i].numLEDs; j++) {
          strip.setPixelColor(ledMappings[i].ledPixels[j], 0);
        }
      }
    }


  
    
    // 5) Oppdater hele NeoPixel-stripen
    strip.show();
  }

  
  
  delay(5);

}
