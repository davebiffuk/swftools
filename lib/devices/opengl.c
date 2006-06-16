#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../gfxdevice.h"
#include "../gfxtools.h"

#include <time.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include <stdarg.h>

#define ZSTEP (1/65536.0)

typedef struct _fontlist {
    gfxfont_t*font;
    struct _fontlist*next;
} fontlist_t;

typedef struct _internal {
    gfxfont_t*font;
    char*fontid;
    fontlist_t* fontlist;
    int width, height;
    int currentz;
   
    GLUtesselator *tesselator;
    GLUtesselator *tesselator_tex;
} internal_t;

int verbose = 1;

static void dbg(char*format, ...)
{
    char buf[1024];
    int l;
    va_list arglist;
    if(!verbose)
	return;
    va_start(arglist, format);
    vsprintf(buf, format, arglist);
    va_end(arglist);
    l = strlen(buf);
    while(l && buf[l-1]=='\n') {
	buf[l-1] = 0;
	l--;
    }
    printf("(device-opengl) %s\n", buf);
    fflush(stdout);
}

#ifndef CALLBACK 
#define CALLBACK
#endif

typedef void(*callbackfunction_t)();

void CALLBACK errorCallback(GLenum errorCode)
{
   const GLubyte *estring;
   estring = gluErrorString(errorCode);
   dbg("Tessellation Error: %s\n", estring);
   exit(0);
}
void CALLBACK beginCallback(GLenum which)
{
   glBegin(which);
}
void CALLBACK endCallback(void)
{
   glEnd();
}
void CALLBACK vertexCallback(GLvoid *vertex)
{
   double*xyz = (GLdouble*)vertex;
   glVertex3d(xyz[0],xyz[1],xyz[2]);
}
void CALLBACK vertexCallbackTex(GLvoid *vertex)
{
   double*v = (GLdouble*)vertex;
   glTexCoord2f(v[3],v[4]);
   glVertex3d(v[0],v[1],v[2]);
}
void CALLBACK combineCallbackTex(GLdouble coords[3], GLdouble *data[4], GLfloat w[4], GLdouble **out)
{
   GLdouble *vertex, *texCoord;
   vertex = (GLdouble *) malloc(5 * sizeof(GLdouble));
   vertex[0] = coords[0];
   vertex[1] = coords[1];
   vertex[2] = coords[2];
   texCoord = &vertex[3];
   vertex[3] = w[0]*data[0][3] + w[1]*data[1][3] + w[2]*data[2][3] + w[3]*data[3][3];
   vertex[4] = w[0]*data[0][4] + w[1]*data[1][4] + w[2]*data[2][4] + w[3]*data[3][4];
   *out = vertex;
}

int opengl_setparameter(struct _gfxdevice*dev, const char*key, const char*value)
{
    dbg("setparameter %s=%s", key, value);
    return 0;
}

void opengl_startpage(struct _gfxdevice*dev, int width, int height)
{
    dbg("startpage %d %d", width, height);
    internal_t*i = (internal_t*)dev->internal;
    i->width = width;
    i->height = height;
    i->currentz = 0;
}

void opengl_startclip(struct _gfxdevice*dev, gfxline_t*line)
{
    dbg("startclip");
}

void opengl_endclip(struct _gfxdevice*dev)
{
    dbg("endclip");
}

void opengl_stroke(struct _gfxdevice*dev, gfxline_t*line, gfxcoord_t width, gfxcolor_t*color, gfx_capType cap_style, gfx_joinType joint_style, gfxcoord_t miterLimit)
{
    internal_t*i = (internal_t*)dev->internal;
    char running = 0;
    gfxline_t*l=0;
    dbg("stroke");
    glColor4f(color->r/255.0, color->g/255.0, color->b/255.0, color->a/255.0);
    glLineWidth(width);

    l = line;
    while(l) {
	if(l->type == gfx_moveTo) {
	    if(running) {
		running = 0;
		glEnd();
	    }
	}
	if(!running) {
	    running = 1;
	    glBegin(GL_LINE_STRIP);
	}
	glVertex3d(l->x, l->y, (i->currentz*ZSTEP));
	l=l->next;
    }
    if(running) {
	running = 0;
	glEnd();
    }
    i->currentz ++;
}

void opengl_fill(struct _gfxdevice*dev, gfxline_t*line, gfxcolor_t*color)
{
    internal_t*i = (internal_t*)dev->internal;
    char running = 0;
    int len = 0;
    double*xyz=0;
    gfxline_t*l=0;
    dbg("fill");
    glColor4f(color->r/255.0, color->g/255.0, color->b/255.0, color->a/255.0);

    gluTessBeginPolygon(i->tesselator, NULL);
    l = line;
    len = 0;
    while(l) {
	len++;
	l = l->next;
    }
    xyz = malloc(sizeof(double)*3*len);
    l = line;
    len = 0;
    while(l) {
	if(l->type == gfx_moveTo) {
	    if(running) {
		running = 0;
		gluTessEndContour(i->tesselator);
	    }
	}
	if(!running) {
	    running = 1;
	    gluTessBeginContour(i->tesselator);
	}

	xyz[len*3+0] = l->x;
	xyz[len*3+1] = l->y;
	xyz[len*3+2] = (i->currentz*ZSTEP);
	gluTessVertex(i->tesselator, &xyz[len*3], &xyz[len*3]);
	len++;

	l=l->next;
    }
    if(running) {
	running = 0;
	gluTessEndContour(i->tesselator);
    }
    gluTessEndPolygon(i->tesselator);
    i->currentz ++;
    free(xyz);
}

void opengl_fillbitmap(struct _gfxdevice*dev, gfxline_t*line, gfximage_t*img, gfxmatrix_t*matrix, gfxcxform_t*cxform)
{
    internal_t*i = (internal_t*)dev->internal;
    char running = 0;
    int len = 0;
    double*xyz=0;
    gfxline_t*l=0;
    glColor4f(1.0,0,0,1.0);
    
    GLuint texIDs[1];
    glGenTextures(1, texIDs);

    int width_bits = 0;
    int height_bits = 0;
    while(1<<width_bits < img->width)
	width_bits++;
    while(1<<height_bits < img->height)
	height_bits++;
    int newwidth = 1<<width_bits;
    int newheight = 1<<height_bits;

    unsigned char*data = malloc(newwidth*newheight*4);
    int x,y;
    for(y=0;y<img->height;y++) {
	for(x=0;x<img->width;x++) {
	    data[(y*newwidth+x)*4+0] = img->data[y*img->width+x].r;
	    data[(y*newwidth+x)*4+1] = img->data[y*img->width+x].g;
	    data[(y*newwidth+x)*4+2] = img->data[y*img->width+x].b;
	    data[(y*newwidth+x)*4+3] = img->data[y*img->width+x].a;
	}
    }

    gfxmatrix_t m2;
    gfxmatrix_invert(matrix, &m2);
    m2.m00 /= newwidth;
    m2.m10 /= newwidth;
    m2.tx /= newwidth;
    m2.m01 /= newheight;
    m2.m11 /= newheight;
    m2.ty /= newheight;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texIDs[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newwidth, newheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glEnable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    gluTessBeginPolygon(i->tesselator_tex, NULL);
    l = line;
    len = 0;
    while(l) {
	len++;
	l = l->next;
    }
    xyz = malloc(sizeof(double)*5*len);
    l = line;
    len = 0;
    while(l) {
	if(l->type == gfx_moveTo) {
	    if(running) {
		running = 0;
		gluTessEndContour(i->tesselator_tex);
	    }
	}
	if(!running) {
	    running = 1;
	    gluTessBeginContour(i->tesselator_tex);
	}

	xyz[len*5+0] = l->x;
	xyz[len*5+1] = l->y;
	xyz[len*5+2] = (i->currentz*ZSTEP);
	xyz[len*5+3] = 0;
	xyz[len*5+4] = 0;
	gfxmatrix_transform(&m2, /*src*/&xyz[len*5+0], /*dest*/&xyz[len*5+3]);

	gluTessVertex(i->tesselator_tex, &xyz[len*5], &xyz[len*5]);
	len++;

	l=l->next;
    }
    if(running) {
	running = 0;
	gluTessEndContour(i->tesselator_tex);
    }
    gluTessEndPolygon(i->tesselator_tex);
    i->currentz ++;
    free(xyz);
    
    glDisable(GL_TEXTURE_2D);
}

void opengl_fillgradient(struct _gfxdevice*dev, gfxline_t*line, gfxgradient_t*gradient, gfxgradienttype_t type, gfxmatrix_t*matrix)
{
    dbg("fillgradient");
}

void opengl_addfont(gfxdevice_t*dev, gfxfont_t*font)
{
    internal_t*i = (internal_t*)dev->internal;
    
    fontlist_t*last=0,*l = i->fontlist;
    while(l) {
	last = l;
	if(!strcmp((char*)l->font->id, font->id)) {
	    return; // we already know this font
	}
	l = l->next;
    }
    l = (fontlist_t*)rfx_calloc(sizeof(fontlist_t));
    l->font = font;
    l->next = 0;
    if(last) {
	last->next = l;
    } else {
	i->fontlist = l;
    }
}

void opengl_drawchar(gfxdevice_t*dev, gfxfont_t*font, int glyphnr, gfxcolor_t*color, gfxmatrix_t*matrix)
{
    internal_t*i = (internal_t*)dev->internal;

    if(i->font && i->font->id && !strcmp(font->id, i->font->id)) {
	// current font is correct
    } else {
	fontlist_t*l = i->fontlist;
	i->font = 0;
	while(l) {
	    if(!strcmp((char*)l->font->id, font->id)) {
		i->font = l->font;
		break;
	    }
	    l = l->next;
	}
	if(i->font == 0) {
	    fprintf(stderr, "Unknown font id: %s", font->id);
	    return;
	}
    }

    gfxglyph_t*glyph = &i->font->glyphs[glyphnr];
    
    gfxline_t*line2 = gfxline_clone(glyph->line);
    gfxline_transform(line2, matrix);
    opengl_fill(dev, line2, color);
    gfxline_free(line2);
    
    return;
}



void opengl_drawlink(struct _gfxdevice*dev, gfxline_t*line, char*action)
{
    dbg("link");
}

void opengl_endpage(struct _gfxdevice*dev)
{
    dbg("endpage");
}

int opengl_result_save(struct _gfxresult*gfx, char*filename)
{
    dbg("result:save");
    return 0;
}
void* opengl_result_get(struct _gfxresult*gfx, char*name)
{
    dbg("result:get");
    return 0;
}
void opengl_result_destroy(struct _gfxresult*gfx)
{
    dbg("result:destroy");
    free(gfx);
}

gfxresult_t*opengl_finish(struct _gfxdevice*dev)
{
    dbg("finish");
    internal_t*i = (internal_t*)dev->internal;
    gluDeleteTess(i->tesselator);i->tesselator=0;
    gluDeleteTess(i->tesselator_tex);i->tesselator_tex=0;
    gfxresult_t*result = (gfxresult_t*)malloc(sizeof(gfxresult_t));
    memset(result, 0, sizeof(gfxresult_t));
    result->save = opengl_result_save;
    result->get = opengl_result_get;
    result->destroy = opengl_result_destroy;
    return result;
}

void gfxdevice_opengl_init(gfxdevice_t*dev)
{
    dbg("init");
    internal_t*i = (internal_t*)rfx_calloc(sizeof(internal_t));
    memset(dev, 0, sizeof(gfxdevice_t));

    dev->internal = i;
    
    dev->setparameter = opengl_setparameter;
    dev->startpage = opengl_startpage;
    dev->startclip = opengl_startclip;
    dev->endclip = opengl_endclip;
    dev->stroke = opengl_stroke;
    dev->fill = opengl_fill;
    dev->fillbitmap = opengl_fillbitmap;
    dev->fillgradient = opengl_fillgradient;
    dev->addfont = opengl_addfont;
    dev->drawchar = opengl_drawchar;
    dev->drawlink = opengl_drawlink;
    dev->endpage = opengl_endpage;
    dev->finish = opengl_finish;

    i->tesselator = gluNewTess();
    gluTessCallback(i->tesselator, GLU_TESS_ERROR, (callbackfunction_t)errorCallback);
    gluTessCallback(i->tesselator, GLU_TESS_VERTEX, (callbackfunction_t)vertexCallback);
    gluTessCallback(i->tesselator, GLU_TESS_BEGIN, (callbackfunction_t)beginCallback);
    gluTessCallback(i->tesselator, GLU_TESS_END, (callbackfunction_t)endCallback);
    
    i->tesselator_tex = gluNewTess();
    gluTessCallback(i->tesselator_tex, GLU_TESS_ERROR, (callbackfunction_t)errorCallback);
    gluTessCallback(i->tesselator_tex, GLU_TESS_VERTEX, (callbackfunction_t)vertexCallbackTex);
    gluTessCallback(i->tesselator_tex, GLU_TESS_BEGIN, (callbackfunction_t)beginCallback);
    gluTessCallback(i->tesselator_tex, GLU_TESS_END, (callbackfunction_t)endCallback);
    gluTessCallback(i->tesselator_tex, GLU_TESS_COMBINE, (callbackfunction_t)combineCallbackTex);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}