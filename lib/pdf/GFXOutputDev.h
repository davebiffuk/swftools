#ifndef __gfxoutputdev_h__
#define __gfxoutputdev_h__

#include "../gfxdevice.h"
#include "../gfxsource.h"

#include "InfoOutputDev.h"
#include "PDFDoc.h"

typedef struct _fontlist
{
    char*filename;
    gfxfont_t*font;
    _fontlist*next;
} fontlist_t;

class GFXOutputState {
    public:
    int clipping;
    int textRender;
    GFXOutputState();
};

typedef struct _parameter
{
    char*name;
    char*value;
    struct _parameter*next;
} parameter_t;

void addGlobalFont(char*filename);
void addGlobalLanguageDir(char*dir);
void addGlobalFontDir(char*dirname);

class GFXOutputDev:  public OutputDev {
public:
  gfxdevice_t* device;

  // Constructor.
  GFXOutputDev(parameter_t*p);
  void setDevice(gfxdevice_t*dev);

  // Destructor.
  virtual ~GFXOutputDev() ;

  void setMove(int x,int y);
  void setClip(int x1,int y1,int x2,int y2);

  void setInfo(InfoOutputDev*info) {this->info = info;}
  
  // Start a page.
  void startFrame(int width, int height);

  virtual void startPage(int pageNum, GfxState *state, double x1, double y1, double x2, double y2) ;

  void endframe();

  //----- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  virtual GBool upsideDown();

  // Does this device use drawChar() or drawString()?
  virtual GBool useDrawChar();
  
  virtual GBool interpretType3Chars();
  
  //virtual GBool useShadedFills() { return gTrue; }

  //----- initialization and control

  void setXRef(PDFDoc*doc, XRef *xref);

  //----- link borders
  virtual void drawLink(Link *link, Catalog *catalog) ;

  //----- save/restore graphics state
  virtual void saveState(GfxState *state) ;
  virtual void restoreState(GfxState *state) ;

  //----- update graphics state

  virtual void updateFont(GfxState *state);
  virtual void updateFillColor(GfxState *state);
  virtual void updateStrokeColor(GfxState *state);
  virtual void updateLineWidth(GfxState *state);
  virtual void updateLineJoin(GfxState *state);
  virtual void updateLineCap(GfxState *state);
  virtual void updateFillOpacity(GfxState *state);

  
  virtual void updateAll(GfxState *state) 
  {
      updateFont(state);
      updateFillColor(state);
      updateStrokeColor(state);
      updateLineWidth(state);
      updateLineJoin(state);
      updateLineCap(state);
  };

  //----- path painting
  virtual void stroke(GfxState *state) ;
  virtual void fill(GfxState *state) ;
  virtual void eoFill(GfxState *state) ;

  //----- path clipping
  virtual void clip(GfxState *state) ;
  virtual void eoClip(GfxState *state) ;

  //----- text drawing
  virtual void beginString(GfxState *state, GString *s) ;
  virtual void endString(GfxState *state) ;
  virtual void endTextObject(GfxState *state);
  virtual void drawChar(GfxState *state, double x, double y,
			double dx, double dy,
			double originX, double originY,
			CharCode code, int nBytes, Unicode *u, int uLen);

  //----- image drawing
  virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
			     int width, int height, GBool invert,
			     GBool inlineImg);
  virtual void drawImage(GfxState *state, Object *ref, Stream *str,
			 int width, int height, GfxImageColorMap *colorMap,
			 int *maskColors, GBool inlineImg);
  virtual void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
			       int width, int height,
			       GfxImageColorMap *colorMap,
			       Stream *maskStr, int maskWidth, int maskHeight,
			       GBool maskInvert);
  virtual void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
				   int width, int height,
				   GfxImageColorMap *colorMap,
				   Stream *maskStr,
				   int maskWidth, int maskHeight,
				   GfxImageColorMap *maskColorMap);

  //----- transparency groups and soft masks (xpdf > ~ 3.01.16)
  virtual void beginTransparencyGroup(GfxState *state, double *bbox,
				      GfxColorSpace *blendingColorSpace,
				      GBool isolated, GBool knockout,
				      GBool forSoftMask);
  virtual void endTransparencyGroup(GfxState *state);
  virtual void paintTransparencyGroup(GfxState *state, double *bbox);
  virtual void setSoftMask(GfxState *state, double *bbox, GBool alpha, Function *transferFunc, GfxColor *backdropColor);
  virtual void clearSoftMask(GfxState *state);

 
  //----- type 3 chars
  virtual GBool beginType3Char(GfxState *state, double x, double y, double dx, double dy, CharCode code, Unicode *u, int uLen);
  virtual void endType3Char(GfxState *state);

  virtual void type3D0(GfxState *state, double wx, double wy);
  virtual void type3D1(GfxState *state, double wx, double wy, double llx, double lly, double urx, double ury);

  void preparePage(int pdfpage, int outputpage);

  char* searchForSuitableFont(GfxFont*gfxFont);

  void finish();

  private:
  void drawGeneralImage(GfxState *state, Object *ref, Stream *str,
				   int width, int height, GfxImageColorMap*colorMap, GBool invert,
				   GBool inlineImg, int mask, int *maskColors,
				   Stream *maskStr, int maskWidth, int maskHeight, GBool maskInvert, GfxImageColorMap*maskColorMap);
  int setGfxFont(char*id, char*name, char*filename, double quality);
  void strokeGfxline(GfxState *state, gfxline_t*line);
  void clipToGfxLine(GfxState *state, gfxline_t*line);
  void fillGfxLine(GfxState *state, gfxline_t*line);

  char outer_clip_box; //whether the page clip box is still on

  InfoOutputDev*info;
  GFXOutputState states[64];
  int statepos;

  int currentpage;

  PDFDoc*doc;
  XRef*xref;

  char* searchFont(char*name);
  char* substituteFont(GfxFont*gfxFont, char*oldname);
  char* writeEmbeddedFontToFile(XRef*ref, GfxFont*font);
  int t1id;
  int textmodeinfo; // did we write "Text will be rendered as polygon" yet?
  int jpeginfo; // did we write "File contains jpegs" yet?
  int pbminfo; // did we write "File contains jpegs" yet?
  int linkinfo; // did we write "File contains links" yet?
  int ttfinfo; // did we write "File contains TrueType Fonts" yet?
  int gradientinfo; // did we write "File contains Gradients yet?

  int type3active; // are we between beginType3()/endType3()?

  GfxState *laststate;

  char type3Warning;

  char* substitutetarget[256];
  char* substitutesource[256];
  int substitutepos;

  /* upper left corner of clipping rectangle (cropbox)- needs to be
     added to all drawing coordinates to give the impression that all
     pages start at (0,0)*/
  int clipmovex,clipmovey;

  int user_movex,user_movey;
  int user_clipx1,user_clipx2,user_clipy1,user_clipy2;

  gfxline_t* current_text_stroke;
  gfxline_t* current_text_clip;
  char* current_font_id;
  gfxfont_t* current_gfxfont;
  gfxmatrix_t current_font_matrix;

  fontlist_t* fontlist;

  int*pages;
  int pagebuflen;
  int pagepos;

  /* config */
  int forceType0Fonts;
  int config_use_fontconfig;

  int transparencyGroup;
  int createsoftmask;
    
  parameter_t*parameters;
};

#endif //__gfxoutputdev_h__