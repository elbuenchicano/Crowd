/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   PLOT.CPP: class for graph plotting.

   Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

// Data control
#define BLOCK_DATA  128

// Plot control
#define XSEPARATOR        1
#define YSEPARATOR        3
#define MIN_BORDER_X      2
#define MIN_BORDER_Y      2
#define NUMBER_SEPARATOR  3


//////////////////////////////////////////////////////////////////////////////
// CLASS gcgPLOT
//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////
// Object construction
/////////////////////////////////////////////////////////////////////////////////////
gcgPLOT::gcgPLOT() {
  // Variable's buffer
  memset(lines, 0, sizeof(lines));
  nlines = 0;

  // Plot info
  framebox[0] = framebox[1] = 0.0;  framebox[2] = 256; framebox[3] = 128;
  linewidth = 1.0f;
  frameR = frameG = frameB = 0.0; frameA = (float) 0.3;       // Frame color
  plotR = (float) 0.0; plotG = (float) 1.0; plotB = (float) 0.0; plotA = (float) 0.25; // Box color

  gridpixelsX = 40;
  gridpixelsY = 40;
  gridR = (float) 0.0; gridG = (float) 1.0; gridB = (float) 0.0; gridA = (float) 0.25; // Grid color

    // Plot axis
  for(int i = 0; i < GCG_PLOT_MAX_AXIS; i++) {
    axis[i].title = NULL;
    axis[i].r = axis[i].g = axis[i].b = axis[i].a = (float) 1.0;
    axis[i].bitpattern = 0xffff;  // Solid lines
    axis[i].ini = -1;
    axis[i].end = 1;
  }

  textWidthX = textWidthY = 40;
  textHeightX = textHeightY = 13;
  factorX = 1.0;
}

/////////////////////////////////////////////////////////////////////////////////////
// Plot release.
/////////////////////////////////////////////////////////////////////////////////////
gcgPLOT::~gcgPLOT() {
  for(int i = 0; i < GCG_PLOT_MAX_AXIS; i++)
    SAFE_FREE(axis[i].title);

  for(int i = 0; i < GCG_PLOT_MAX_LINES; i++) {
    SAFE_FREE(lines[i].linename);
    SAFE_FREE(lines[i].data);
  }
}

/////////////////////////////////////////////////////////////////////////////////////
// Opens a new variable in the graph.
// Returns the index of this variable for future accesses or 0 if failed.
/////////////////////////////////////////////////////////////////////////////////////
int gcgPLOT::newLine() {
  if(nlines >= GCG_PLOT_MAX_LINES) return 0;
  return ++nlines;
}

/////////////////////////////////////////////////////////////////////////////////////
// Adds a point to the a variable in the plot. Keeps only data in the domain (X).
// Returns true if point was added.
/////////////////////////////////////////////////////////////////////////////////////
bool gcgPLOT::addPoint2D(unsigned int lineindex, float x, float y) {
  if(lineindex < 1 || lineindex > nlines) return false;

  // Adjusts index to [0, MAX_LINES-1] range
  lineindex--;

  // Check space in data buffer
  if(lines[lineindex].ndata + 1 >= lines[lineindex].maxdata) {
    lines[lineindex].maxdata += BLOCK_DATA;
    lines[lineindex].data = (VECTOR3*) REALLOC(lines[lineindex].data, sizeof(VECTOR3) * lines[lineindex].maxdata);
    if(lines[lineindex].data == NULL) return false;
  }

  // Adds the new point
  if(lines[lineindex].ndata > 0) {
    // Try to append the new point, in lexicographic order
//    if(x > lines[lineindex].data[lines[lineindex].ndata-1][0] ||
//       (x == lines[lineindex].data[lines[lineindex].ndata-1][0] && y > lines[lineindex].data[lines[lineindex].ndata-1][1])) {
      lines[lineindex].data[lines[lineindex].ndata][0] = x;
      lines[lineindex].data[lines[lineindex].ndata][1] = y;
      // Update number of points
      lines[lineindex].ndata++;
//    } else {}; // Must position the new point
  } else {
      lines[lineindex].data[lines[lineindex].ndata][0] = x;
      lines[lineindex].data[lines[lineindex].ndata][1] = y;
      // Update number of points
      lines[lineindex].ndata++;
    }

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Removes data points from variable data that are outside the axis range.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::clipOutOfRange(unsigned int lineindex, unsigned int iaxis) {
  if(lineindex < 1 || lineindex > nlines || iaxis >= GCG_PLOT_MAX_AXIS) return;

  // Adjusts index to [0, MAX_LINES-1] range
  lineindex--;

  unsigned int current = 0;
  bool lastfailed = false;
  for(unsigned int i = 0; i < lines[lineindex].ndata; i++)
    if(lines[lineindex].data[i][iaxis] >= axis[iaxis].ini && // Is in range?
       lines[lineindex].data[i][iaxis] <= axis[iaxis].end) {
      // Check last point
      if(lastfailed && i > 0) {
        if(current < i-1) gcgCOPYVECTOR3(lines[lineindex].data[current], lines[lineindex].data[i-1]);
        current++;
      }
      lastfailed = false;

       // This point is in range, copy if it is not at right position.
      if(current < i) gcgCOPYVECTOR3(lines[lineindex].data[current], lines[lineindex].data[i]);
      current++;

      // Add next point
      if(i + 1 < lines[lineindex].ndata) {
        i++;
        if(current < i) gcgCOPYVECTOR3(lines[lineindex].data[current], lines[lineindex].data[i]);
        current++;
      }
    } else lastfailed = true;

  // Now, the size has possibly changed
  lines[lineindex].ndata = current;
}

/////////////////////////////////////////////////////////////////////////////////////
// Sets the range of the specified axis from data of specified variable.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::rangeFromData(unsigned int iaxis) {
  if(iaxis >= GCG_PLOT_MAX_AXIS) return;

  // Adjusts index to [0, MAX_LINES-1] range
  //lineindex--;

  unsigned int count = 0;
  float min = (float) INF, max = (float) -INF;
  for(unsigned int j = 0; j < nlines; j++)
    for(unsigned int i = 0; i < lines[j].ndata; i++, count++) {
      if(min > lines[j].data[i][iaxis]) min = lines[j].data[i][iaxis];
      if(max < lines[j].data[i][iaxis]) max = lines[j].data[i][iaxis];
    }

  // Check if no lines have data
  if(count == 0) {
    setAxisRange(iaxis, -1, 1);
    return;
  }

  if(fabs(min - max) < EPSILON) {
    min -= min / 2;
    max += max / 2;
  }

  // Now, set the new range as min/max
  setAxisRange(iaxis, min, max);
}

/////////////////////////////////////////////////////////////////////////////////////
// Clear the data of a specific line.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::clearData(unsigned int lineindex) {
  if(lineindex < 1 || lineindex > nlines) return;

  // Adjusts index to [0, MAX_LINES-1] range
  lineindex--;

  // Check space in data buffer
  lines[lineindex].ndata = 0;
}


/////////////////////////////////////////////////////////////////////////////////////
// Clear the data of all lines.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::clearAllData() {
  for(unsigned int j = 0; j < nlines; j++) lines[j].ndata = 0;
}


/////////////////////////////////////////////////////////////////////////////////////
// Set the range of an axis.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::setAxisRange(unsigned int iaxis, float ini, float end) {
  if(iaxis >= GCG_PLOT_MAX_AXIS) return;

  if(ini < end) {
    axis[iaxis].ini = ini;
    axis[iaxis].end = end;
  } else {
    axis[iaxis].ini = end;
    axis[iaxis].end = ini;
  }

  if(iaxis == GCG_PLOT_X) {
    char number[128];
    unsigned int strlen1, strlen2;

    // Find best fit font for X
    unsigned int h = 0, w = 0;
    sprintf(number, "%3.2f", ini);
    strlen1 = (unsigned int) strlen(number);
    sprintf(&number[strlen1 + 2], "%3.2f", end);
    strlen2 = (unsigned int) strlen(&number[strlen1 + 2]);
    factorX = textoutput.setBestFitSystemFont((strlen1 > strlen2) ? number : &number[strlen1 + 2], (float) (gridpixelsX - NUMBER_SEPARATOR));
    if(factorX > 1.0) factorX = 1.0;

    // Now get X extents
    if(textoutput.getCurrentFont()) {
      textoutput.getCurrentFont()->textLength(&textWidthX, &textHeightX, number, strlen1); // Must fit at least these characters
      textoutput.getCurrentFont()->textLength(&w, &h, &number[strlen1 + 2], strlen2); // Must fit at least these characters
    }
    if(h > textHeightX) textHeightX = h;
    if(w > textWidthX) textWidthX = w;
    textWidthX += NUMBER_SEPARATOR;
  }

  if(iaxis == GCG_PLOT_X || iaxis == GCG_PLOT_Y) {
    char number[128];
    unsigned int h = 0, w = 0;
    // Now get Y extents
    if(textoutput.getCurrentFont()) {
      sprintf(number, "%3.2f", axis[GCG_PLOT_Y].ini);
      textoutput.getCurrentFont()->textLength(&textWidthY, &textHeightY, number, (int) strlen(number)); // Must fit at least these characters
      sprintf(number, "%3.2f", axis[GCG_PLOT_Y].end);
      textoutput.getCurrentFont()->textLength(&w, &h, number, (int) strlen(number)); // Must fit at least these characters
    }
    if(h > textHeightY) textHeightY = h;
    if(w > textWidthY) textWidthY = w;
    textWidthY += NUMBER_SEPARATOR;
  }
}

/////////////////////////////////////////////////////////////////////////////////////
// Set the frame.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::setFrame(float xbox, float ybox, float width, float height, float linewidth) {
  framebox[0] = xbox; framebox[1] = ybox; framebox[2] = xbox + width; framebox[3] = ybox + height;
  this->linewidth = linewidth;
}

/////////////////////////////////////////////////////////////////////////////////////
// Moves the frame by xy offsets.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::adjustFrame(float xoffset, float yoffset, float widthoffset, float heightoffset) {
  framebox[0] += xoffset;
  framebox[1] += yoffset;
  framebox[2] += xoffset + widthoffset;
  framebox[3] += yoffset + heightoffset;
}


/////////////////////////////////////////////////////////////////////////////////////
// Changes the frame color.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::setFrameColor(float r, float g, float b, float a) {
  frameR = r;  frameG = g;  frameB = b; frameA = a;
}

/////////////////////////////////////////////////////////////////////////////////////
// Changes the plot rectangle color.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::setPlotColor(float r, float g, float b, float a) {
  plotR = r;  plotG = g;  plotB = b; plotA = a;
}

/////////////////////////////////////////////////////////////////////////////////////
// Changes the pixel interval between two parallel lines on grid.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::setGrid(unsigned int pixelsx, unsigned int pixelsy) {
  gridpixelsX = pixelsx;  gridpixelsY = pixelsy;
}

/////////////////////////////////////////////////////////////////////////////////////
// Changes the grid lines color.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::setGridColor(float r, float g, float b, float a) {
  gridR = r;  gridG = g;  gridB = b; gridA = a;
}

/////////////////////////////////////////////////////////////////////////////////////
// Sets the axis drawing information.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::setAxis(unsigned int iaxis, char *title, float r, float g, float b, float a, float lwidth, unsigned short bitpattern) {
  if(iaxis >= GCG_PLOT_MAX_AXIS) return;

  // Copies the title string
  SAFE_FREE(axis[iaxis].title);
  if(title) {
    axis[iaxis].title = (char*) ALLOC(sizeof(char) * strlen(title) + 1);
    if(axis[iaxis].title) strcpy(axis[iaxis].title, title);
  } else axis[iaxis].title = NULL;

  // Set the axis color and bit pattern
  axis[iaxis].r = r;  axis[iaxis].g = g;  axis[iaxis].b = b; axis[iaxis].a = a;
  axis[iaxis].linewidth = lwidth;
  axis[iaxis].bitpattern = bitpattern;
}

/////////////////////////////////////////////////////////////////////////////////////
// Sets the variable drawing information.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::setLine(unsigned char lineindex, char *linename, float r, float g, float b, float a, float lwidth, unsigned short bitpattern) {
  if(lineindex < 1 || lineindex > nlines) return;

  // Adjusts index to [0, MAX_LINES-1] range
  lineindex--;

  // Copies the title string
  SAFE_FREE(lines[lineindex].linename);
  if(linename) {
    lines[lineindex].linename = (char*) ALLOC(sizeof(char) * strlen(linename));
    if(lines[lineindex].linename) strcpy(lines[lineindex].linename, linename);
  } else lines[lineindex].linename = NULL;

  // Set the axis color and bit pattern
  lines[lineindex].r = r;  lines[lineindex].g = g;  lines[lineindex].b = b; lines[lineindex].a = a;
  lines[lineindex].linewidth = lwidth;
  lines[lineindex].bitpattern = bitpattern;
}

/////////////////////////////////////////////////////////////////////////////////////
// Draws the plot.
/////////////////////////////////////////////////////////////////////////////////////
void gcgPLOT::draw() {
  int   viewport[4];
  char  number[256];
  int   length;
  unsigned int H, W;


  // Check frame thickness
  if(fabs(framebox[2] - framebox[0]) < gridpixelsX || fabs(framebox[1] - framebox[3]) < gridpixelsY) return;
  if(textoutput.getCurrentFont()->textureID == 0) return;

  glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

  glGetIntegerv(GL_VIEWPORT, viewport);
  glOrtho(0.0, viewport[2]-viewport[0], (viewport[3] - viewport[1]), 0, 0.f, -1.f);

  glDisable(GL_TEXTURE_2D);
  glFrontFace(GL_CCW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(framebox[0], framebox[1], (float) EPSILON);

  //glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
  glDepthMask(GL_FALSE);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if(frameA > EPSILON && this->linewidth > EPSILON) {
    glColor4f(frameR, frameG, frameB, (float) ((frameA < 0.5) ? frameA + frameA : 1.0));

    glLineWidth(this->linewidth);
    if(fabs(this->linewidth - 1.0f) > EPSILON) glEnable(GL_LINE_SMOOTH);
    else glDisable(GL_LINE_SMOOTH);

    glBegin(GL_LINE_STRIP);
  		glVertex2f(0.f, 0.f);
  		glVertex2f(0.f, framebox[3] - framebox[1]);
  		glVertex2f(framebox[2] - framebox[0], framebox[3] - framebox[1]);
  		glVertex2f(framebox[2] - framebox[0], 0.f);
  		glVertex2f(0.f, 0.f);
  	glEnd();

    glDisable(GL_LINE_SMOOTH);
  	glLineWidth(1.0f);
  }

  glColor4f(frameR, frameG, frameB, frameA);
 	glBegin(GL_QUADS);
 	  glVertex2f(0.f, 0.f);
 	  glVertex2f(0.f, framebox[3]-framebox[1]);
 	  glVertex2f(framebox[2] - framebox[0], framebox[3]-framebox[1]);
		glVertex2f(framebox[2] - framebox[0], 0.f);
	glEnd();

	// Compute borders
	float borderX = textWidthY  * factorX + MIN_BORDER_X,
          borderY = textHeightY * factorX * 0.3f + MIN_BORDER_Y,
          borderwidth  = framebox[2] - framebox[0] - borderX - MIN_BORDER_X - textWidthX * factorX * 0.5f,
          borderheight = framebox[3] - framebox[1] - borderY - MIN_BORDER_Y - textHeightX * factorX;

  // Check border size
  if(borderwidth >= gridpixelsX && borderheight >= gridpixelsY) {
    // Draws X axis and title space to borders
    if(axis[GCG_PLOT_X].title) {
      int length = (int) strlen(axis[GCG_PLOT_X].title);
      textoutput.getCurrentFont()->textLength(&W, &H, axis[GCG_PLOT_X].title, length);
      float factor = (borderwidth / W);
      if(factor > 1.0) factor = 1.0;

      borderheight -= H * factor + XSEPARATOR;

      glColor4f(axis[GCG_PLOT_X].r, axis[GCG_PLOT_X].g, axis[GCG_PLOT_X].b, axis[GCG_PLOT_X].a);
      textoutput.enableTextBox(framebox[0] + borderX + borderwidth/2 + (W * factor) / 2, framebox[1] + borderY + borderheight + H * factorX, W * factor + 2, H * factor + 2);
      textoutput.fontScale(factor, factor);
      textoutput.gcgprintf(axis[GCG_PLOT_X].title);
    }

    // Draws Y axis and title space to borders
    if(axis[GCG_PLOT_Y].title) {
      int length = (int) strlen(axis[GCG_PLOT_Y].title);
      textoutput.getCurrentFont()->textLength(&W, &H, axis[GCG_PLOT_Y].title, length);
      float factor = (framebox[3] - framebox[1] - borderY) / W;
      if(factor > 1.0) factor = 1.0;

      borderX += H * factorX * factor + YSEPARATOR;
      borderwidth -= H * factor + YSEPARATOR;

      glColor4f(axis[GCG_PLOT_Y].r, axis[GCG_PLOT_Y].g, axis[GCG_PLOT_Y].b, axis[GCG_PLOT_Y].a);
      textoutput.enableTextBox(framebox[0], framebox[1] + (framebox[3] - framebox[1]) / 2 + W * factor / 2, W * factor + 2, H * factor);
      textoutput.fontScale(factor, factor);
      textoutput.textOrientation(90);
      textoutput.gcgprintf(axis[GCG_PLOT_Y].title);
      textoutput.textOrientation(0);
    }

    // Check plot area thickness
    glEnable(GL_BLEND);
    if(fabs(borderwidth) > 15 && fabs(borderheight) > 15) {
      // Draws the plot area
      glLoadIdentity();
      glTranslatef(framebox[0] + borderX, framebox[1] + borderY, (float) EPSILON);

      // Draws the plot
      glColor4f(plotR, plotG, plotB, plotA);
      glBegin(GL_QUADS);
        glVertex2f(0.f, 0.f);
        glVertex2f(0.f, borderheight);
        glVertex2f(borderwidth, borderheight);
        glVertex2f(borderwidth, 0.f);
      glEnd();

      // Compute deltas
      float deltaX =  (axis[GCG_PLOT_X].end - axis[GCG_PLOT_X].ini) * (gridpixelsX / borderwidth);
      float deltaY =  (axis[GCG_PLOT_Y].end - axis[GCG_PLOT_Y].ini) * (gridpixelsY / borderheight);
      if(fabs(deltaX) < EPSILON) deltaX = 1;
      if(fabs(deltaY) < EPSILON) deltaY = 1;
      int startX = (int) ceil(axis[GCG_PLOT_X].ini / deltaX);
      int startY = (int) ceil(axis[GCG_PLOT_Y].ini / deltaY);
      int endX   = (int) floor(axis[GCG_PLOT_X].end / deltaX);
      int endY   = (int) floor(axis[GCG_PLOT_Y].end / deltaY);

      // Draws the grid
      if(gridA > EPSILON) {
        glColor4f(gridR, gridG, gridB, (gridA < 0.5f) ? gridA + gridA : 1.0f);

        glBegin(GL_LINES);
          // Draw range grid
          glVertex2f(0.0f, borderheight /*0.f*/);
          glVertex2f(0.0f, (float) (borderheight + 2.0));
          glVertex2f(borderwidth , borderheight /*0.f*/);
          glVertex2f(borderwidth , (float) (borderheight + 2.0));

          glVertex2f(-2.f, 0.0);
          glVertex2f(0.0/*borderwidth*/, 0.0);
          glVertex2f(-2.f, borderheight);
          glVertex2f(0.0/*borderwidth*/, borderheight);
        glEnd();

        // Draw X grid
        glLineStipple(1, axis[GCG_PLOT_X].bitpattern);
        if(axis[GCG_PLOT_X].bitpattern != 0xffff) glEnable(GL_LINE_STIPPLE);
        else glDisable(GL_LINE_STIPPLE);

        glLineWidth(axis[GCG_PLOT_X].linewidth);
        if(fabs(axis[GCG_PLOT_X].linewidth - 1.0f) > EPSILON) glEnable(GL_LINE_SMOOTH);
        else glDisable(GL_LINE_SMOOTH);

        glBegin(GL_LINES);
          for(int i = startX; i <= endX; i++) {
            float x = (float) ((int) (0.5 + borderwidth * (i * deltaX - axis[GCG_PLOT_X].ini) / (axis[GCG_PLOT_X].end - axis[GCG_PLOT_X].ini)));
            glVertex2f(x, 0.f);
            glVertex2f(x, borderheight);
          }
        glEnd();

        // Draw Y grid
        glLineStipple(1, axis[GCG_PLOT_Y].bitpattern);
        if(axis[GCG_PLOT_Y].bitpattern != 0xffff) glEnable(GL_LINE_STIPPLE);
        else glDisable(GL_LINE_STIPPLE);

        glLineWidth(axis[GCG_PLOT_Y].linewidth);
        if(fabs(axis[GCG_PLOT_Y].linewidth - 1.0f) > EPSILON) glEnable(GL_LINE_SMOOTH);
        else glDisable(GL_LINE_SMOOTH);

        glBegin(GL_LINES);
          for(int i = startY; i <= endY; i++) {
            float y = (float) ((int) (0.5 + borderheight - borderheight * (i * deltaY - axis[GCG_PLOT_Y].ini) / (axis[GCG_PLOT_Y].end - axis[GCG_PLOT_Y].ini)));
            glVertex2f(0.f, y);
            glVertex2f(borderwidth, y);
          }
        glEnd();
      }

      // Draws all grid values
      textoutput.fontScale(factorX, factorX);

      // Draws X range values
      glColor4f(axis[GCG_PLOT_X].r, axis[GCG_PLOT_X].g, axis[GCG_PLOT_X].b, axis[GCG_PLOT_X].a);
      sprintf(number, "%3.2f", axis[GCG_PLOT_X].ini);
      length = (int) strlen(number);
      textoutput.getCurrentFont()->textLength(&W, &H, number, length); W = (unsigned int) (factorX * W + 0.5); H = (unsigned int) (factorX * H + 0.5);
      textoutput.enableTextBox((float) ((int) (0.5 + framebox[0] + borderX - W * 0.5)), (float) ((int) (0.5 + framebox[1] + borderY + borderheight) + XSEPARATOR), (float) (W + 2), (float) H);
      textoutput.gcgprintf(number);

      sprintf(number, "%3.2f", axis[GCG_PLOT_X].end);
      length = (int) strlen(number);
      textoutput.getCurrentFont()->textLength(&W, &H, number, length); W = (unsigned int) (factorX * W + 0.5); H = (unsigned int) (factorX * H + 0.5);
      textoutput.enableTextBox((float) ((int) (0.5 + framebox[0] + borderX + borderwidth - W * 0.5)), (float) ((int) (0.5 + framebox[1] + borderY + borderheight) + XSEPARATOR), (float) (W + 2), (float) H);
      textoutput.gcgprintf(number);

      // Draws the X values
      for(int i = startX; i <= endX; i++) {
        float v = i * deltaX, x = borderwidth * (v - axis[GCG_PLOT_X].ini) / (axis[GCG_PLOT_X].end - axis[GCG_PLOT_X].ini);

        sprintf(number, "%3.2f", v);
        length = (int) strlen(number);
        textoutput.getCurrentFont()->textLength(&W, &H, number, length); W = (unsigned int) (factorX * W + 0.5); H = (unsigned int) (factorX * H + 0.5);
        if(x > textWidthX  * factorX && x < borderwidth - textWidthX  * factorX) {
          textoutput.enableTextBox((float) ((int) (0.5 + framebox[0] + borderX + x - W * 0.5)), (float) ((int) (0.5 + framebox[1] + borderY + borderheight) + XSEPARATOR), (float) (W + 2), (float) H);
          textoutput.gcgprintf(number);
        }
      }

      // Draws Y range values
      glColor4f(axis[GCG_PLOT_Y].r, axis[GCG_PLOT_Y].g, axis[GCG_PLOT_Y].b, axis[GCG_PLOT_Y].a);
      sprintf(number, "%3.2f", axis[GCG_PLOT_Y].ini);
      length = (int) strlen(number);
      textoutput.getCurrentFont()->textLength(&W, &H, number, length); W = (unsigned int) (factorX * W + 0.5); H = (unsigned int) (factorX * H + 0.5);
      textoutput.enableTextBox((float) ((int) (0.5 + framebox[0] + borderX - W) - YSEPARATOR), (float) ((int) (0.5 + framebox[1] + borderY + borderheight - H * 0.7)), (float) (W + 2), (float) H);
      textoutput.gcgprintf(number);

      sprintf(number, "%3.2f", axis[GCG_PLOT_Y].end);
      length = (int) strlen(number);
      textoutput.getCurrentFont()->textLength(&W, &H, number, length); W = (unsigned int) (factorX * W + 0.5); H = (unsigned int) (factorX * H + 0.5);
      textoutput.enableTextBox((float) ((int) (0.5 + framebox[0] + borderX - W) - YSEPARATOR), (float) ((int) (0.5 + framebox[1] + borderY - H * 0.5)), (float) (W + 2), (float) H);
      textoutput.gcgprintf(number);

      // Draws the Y values
      for(int i = startY; i <= endY; i++) {
        float v = i * deltaY, y = borderheight - borderheight * (i * deltaY - axis[GCG_PLOT_Y].ini) / (axis[GCG_PLOT_Y].end - axis[GCG_PLOT_Y].ini);
        sprintf(number, "%3.2f", v);
        length = (int) strlen(number);
        textoutput.getCurrentFont()->textLength(&W, &H, number, length); W = (unsigned int) (factorX * W + 0.5); H = (unsigned int) (factorX * H + 0.5);
        if(y > textHeightY  * factorX && y < borderheight - textHeightY  * factorX) {
          textoutput.enableTextBox((float) ((int) (0.5 + framebox[0] + borderX - W) - YSEPARATOR), (float) ((int) (0.5 + framebox[1] + borderY + y - H * 0.5)), (float) (W + 2), (float) H);
          textoutput.gcgprintf(number);
        }
      }

      // Draws all lines
      glEnable(GL_BLEND);
      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      VECTOR3 box2dMin, box2dMax;
      gcgSETVECTOR3(box2dMin, axis[GCG_PLOT_X].ini, axis[GCG_PLOT_Y].ini, 0);
      gcgSETVECTOR3(box2dMax, axis[GCG_PLOT_X].end, axis[GCG_PLOT_Y].end, 0);

      for(unsigned int i = 0; i < nlines; i++) {
        glColor4f(lines[i].r, lines[i].g, lines[i].b, lines[i].a);
        glLineStipple(1, lines[i].bitpattern);
        if(lines[i].bitpattern != 0xffff) glEnable(GL_LINE_STIPPLE);
        else glDisable(GL_LINE_STIPPLE);

        glLineWidth(lines[i].linewidth);
        if(fabs(lines[i].linewidth - 1.0f) > EPSILON) glEnable(GL_LINE_SMOOTH);
        else glDisable(GL_LINE_SMOOTH);

        glBegin(GL_LINES);
          for(unsigned int j = 1; j < lines[i].ndata; j++) {
            VECTOR3  p1, p2;
            gcgCOPYVECTOR3(p1, lines[i].data[j-1]);
            gcgCOPYVECTOR3(p2, lines[i].data[j]);

            // Clip each line against the plot area
            if(gcgIntersectLineBox2D(p1, p2, box2dMin, box2dMax)) {
              glVertex2f(borderwidth  * (p1[0] - axis[GCG_PLOT_X].ini) / (axis[GCG_PLOT_X].end - axis[GCG_PLOT_X].ini),
                         borderheight - borderheight * (p1[1] - axis[GCG_PLOT_Y].ini) / (axis[GCG_PLOT_Y].end - axis[GCG_PLOT_Y].ini));
              glVertex2f(borderwidth  * (p2[0] - axis[GCG_PLOT_X].ini) / (axis[GCG_PLOT_X].end - axis[GCG_PLOT_X].ini),
                         borderheight - borderheight * (p2[1] - axis[GCG_PLOT_Y].ini) / (axis[GCG_PLOT_Y].end - axis[GCG_PLOT_Y].ini));
            }
          }
        glEnd();
      }
    }
  }
  glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_BLEND);
  glDisable(GL_LINE_STIPPLE);
  glColor4f(1.f, 1.f, 1.f, 1.f);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	// Restaura matriz de projecao
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// Restaura matriz de modelo visualizacao
  glMatrixMode(GL_MODELVIEW);
	glPopMatrix();


}
