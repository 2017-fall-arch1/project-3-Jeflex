/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6


AbRect paddle = {abRectGetBounds, abRectCheck, {20,3}};

unsigned int p1Score = 0;
unsigned int p2Score = 0;



AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer layer3 = {		/**< Layer with an orange circle */
  (AbShape *)&circle4,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  0
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &layer3
};

Layer layer1 = {		/**< Layer with a red square */
  (AbShape *)&paddle,
  {(screenWidth/2), (screenHeight/2) - 64}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &fieldLayer,
};

Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&paddle,
  {(screenWidth/2), (screenHeight/2) + 64}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &layer1,
};
/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = { &layer3, {3,3}, 0 }; /**< not all layers move */
MovLayer ml1 = { &layer1, {0,0}, &ml3 };
MovLayer ml0 = { &layer0, {0,0}, &ml1 };

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1],
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer;
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break;
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color);
      } // for col
    } // for row
  } // for moving layer being updated
}


/** Advances a moving shape within a fence
 *
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */


void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
      if ((shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[axis])){
            p1Score++;
          }
      if(shapeBoundary.botRight.axes[1] > fence->botRight.axes[axis]){
            p2Score++;
          }

	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


void bouncePaddle(MovLayer *ball, MovLayer *paddle){
    Region b1;
    Region b2;
    Vec2 xy;
    u_char x;

    abShapeGetBounds(paddle->layer->abShape, &paddle->layer->posNext, &b1);
    vec2Add(&xy, &ball->layer->pos, &ball->velocity);
    abShapeGetBounds(ball->layer->abShape, &xy, &b2);

    if(abShapeCheck(paddle->layer->abShape, &paddle->layer->pos, &b2.topLeft) ||
        abShapeCheck(paddle->layer->abShape, &paddle->layer->pos, &b2.botRight) ){
        int velocity = ball->velocity.axes[1] = -ball->velocity.axes[1];
        xy.axes[1] += (2*velocity);
        buzzer_set_period(1500);
    }
}

u_int bgColor = COLOR_BLUE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */
char playGame = 0;
char gameOver = 0;

Region fieldFence;		/**< fence around playing field  */

/** Initializes everything, enables
 interrupts and green LED,
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */
  P1OUT |= GREEN_LED;
  buzzer_init();
  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) {
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  buzzer_set_period(0);
  if (count == 15) {

    if((p1Score > 2) || (p2Score > 2)){
      clearScreen(COLOR_BLUE);
      drawString5x7(20,60, "Point. Game. Set. Match.", COLOR_RED, COLOR_WHITE);

    }

    redrawScreen = 1;
    mlAdvance(&ml0, &fieldFence);
    char dir[] = {'0','1','2','3'};
    char tot[] = {dir[p1Score]+dir[p2Score]};
    drawString5x7(60,150, tot, COLOR_RED, COLOR_WHITE);
    bouncePaddle(&ml3,&ml1);
    bouncePaddle(&ml3,&ml0);
    u_int switches = p2sw_read(), i;
    char str[5];
    for (i = 0; i < 4; i++){
        str[i] = (switches & (1<<i)) ? 0 : 1;
      }
    str[4] = 0;

    if(str[0]){
        ml0.velocity.axes[1] = 0;
        ml0.velocity.axes[0] = -3;
    }
    if(str[1]){
        ml0.velocity.axes[1] = 0;
        ml0.velocity.axes[0] = 3;
    }
    if(str[2]){
        ml1.velocity.axes[1] = 0;
        ml1.velocity.axes[0] = -3;
    }
    if(str[3]){
        ml1.velocity.axes[1] = 0;
        ml1.velocity.axes[0] = 3;
    }
    if(!str[0] && !str[1]){
        ml0.velocity.axes[1] = 0;
        ml0.velocity.axes[0] = 0;
    }
    if(!str[2] && !str[3]){
        ml1.velocity.axes[1] = 0;
        ml1.velocity.axes[0] = 0;
    }
    count = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
