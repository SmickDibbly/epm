#include "src/misc/epm_includes.h"
#include "src/draw/xspans.h"
#include "src/draw/drawtri_functions.h"

/////////////////////////////////////////////////////////////////////////////

extern uint32_t drawtri_skydraw(Window *win, ScreenTri *tri);

#define PARAM Window*, ScreenTri*
static uint32_t drawtri_persp_____________________________(PARAM);
static uint32_t drawtri_persp________________________TEXEL(PARAM);
static uint32_t drawtri_persp___________________VBRI______(PARAM);
static uint32_t drawtri_persp___________________VBRI_TEXEL(PARAM);
static uint32_t drawtri_persp______________FBRI___________(PARAM);
static uint32_t drawtri_persp______________FBRI______TEXEL(PARAM);
static uint32_t drawtri_persp______________FBRI_VBRI______(PARAM);
static uint32_t drawtri_persp______________FBRI_VBRI_TEXEL(PARAM);
static uint32_t drawtri_persp________HILIT________________(PARAM);
static uint32_t drawtri_persp________HILIT___________TEXEL(PARAM);
static uint32_t drawtri_persp________HILIT______VBRI______(PARAM);
static uint32_t drawtri_persp________HILIT______VBRI_TEXEL(PARAM);
static uint32_t drawtri_persp________HILIT_FBRI___________(PARAM);
static uint32_t drawtri_persp________HILIT_FBRI______TEXEL(PARAM);
static uint32_t drawtri_persp________HILIT_FBRI_VBRI______(PARAM);
static uint32_t drawtri_persp________HILIT_FBRI_VBRI_TEXEL(PARAM);
static uint32_t drawtri_persp__COUNT______________________(PARAM);
static uint32_t drawtri_persp__COUNT_________________TEXEL(PARAM);
static uint32_t drawtri_persp__COUNT____________VBRI______(PARAM);
static uint32_t drawtri_persp__COUNT____________VBRI_TEXEL(PARAM);
static uint32_t drawtri_persp__COUNT_______FBRI___________(PARAM);
static uint32_t drawtri_persp__COUNT_______FBRI______TEXEL(PARAM);
static uint32_t drawtri_persp__COUNT_______FBRI_VBRI______(PARAM);
static uint32_t drawtri_persp__COUNT_______FBRI_VBRI_TEXEL(PARAM);
static uint32_t drawtri_persp__COUNT_HILIT________________(PARAM);
static uint32_t drawtri_persp__COUNT_HILIT___________TEXEL(PARAM);
static uint32_t drawtri_persp__COUNT_HILIT______VBRI______(PARAM);
static uint32_t drawtri_persp__COUNT_HILIT______VBRI_TEXEL(PARAM);
static uint32_t drawtri_persp__COUNT_HILIT_FBRI___________(PARAM);
static uint32_t drawtri_persp__COUNT_HILIT_FBRI______TEXEL(PARAM);
static uint32_t drawtri_persp__COUNT_HILIT_FBRI_VBRI______(PARAM);
static uint32_t drawtri_persp__COUNT_HILIT_FBRI_VBRI_TEXEL(PARAM);


static uint32_t drawtri_ortho_____________________________(PARAM);
static uint32_t drawtri_ortho________________________TEXEL(PARAM);
static uint32_t drawtri_ortho___________________VBRI______(PARAM);
static uint32_t drawtri_ortho___________________VBRI_TEXEL(PARAM);
static uint32_t drawtri_ortho______________FBRI___________(PARAM);
static uint32_t drawtri_ortho______________FBRI______TEXEL(PARAM);
static uint32_t drawtri_ortho______________FBRI_VBRI______(PARAM);
static uint32_t drawtri_ortho______________FBRI_VBRI_TEXEL(PARAM);
static uint32_t drawtri_ortho________HILIT________________(PARAM);
static uint32_t drawtri_ortho________HILIT___________TEXEL(PARAM);
static uint32_t drawtri_ortho________HILIT______VBRI______(PARAM);
static uint32_t drawtri_ortho________HILIT______VBRI_TEXEL(PARAM);
static uint32_t drawtri_ortho________HILIT_FBRI___________(PARAM);
static uint32_t drawtri_ortho________HILIT_FBRI______TEXEL(PARAM);
static uint32_t drawtri_ortho________HILIT_FBRI_VBRI______(PARAM);
static uint32_t drawtri_ortho________HILIT_FBRI_VBRI_TEXEL(PARAM);
static uint32_t drawtri_ortho__COUNT______________________(PARAM);
static uint32_t drawtri_ortho__COUNT_________________TEXEL(PARAM);
static uint32_t drawtri_ortho__COUNT____________VBRI______(PARAM);
static uint32_t drawtri_ortho__COUNT____________VBRI_TEXEL(PARAM);
static uint32_t drawtri_ortho__COUNT_______FBRI___________(PARAM);
static uint32_t drawtri_ortho__COUNT_______FBRI______TEXEL(PARAM);
static uint32_t drawtri_ortho__COUNT_______FBRI_VBRI______(PARAM);
static uint32_t drawtri_ortho__COUNT_______FBRI_VBRI_TEXEL(PARAM);
static uint32_t drawtri_ortho__COUNT_HILIT________________(PARAM);
static uint32_t drawtri_ortho__COUNT_HILIT___________TEXEL(PARAM);
static uint32_t drawtri_ortho__COUNT_HILIT______VBRI______(PARAM);
static uint32_t drawtri_ortho__COUNT_HILIT______VBRI_TEXEL(PARAM);
static uint32_t drawtri_ortho__COUNT_HILIT_FBRI___________(PARAM);
static uint32_t drawtri_ortho__COUNT_HILIT_FBRI______TEXEL(PARAM);
static uint32_t drawtri_ortho__COUNT_HILIT_FBRI_VBRI______(PARAM);
static uint32_t drawtri_ortho__COUNT_HILIT_FBRI_VBRI_TEXEL(PARAM);

fn_draw_Triangle drawtri_table[256] = {
    [TRIFLAG_NONE] =
    drawtri_persp_____________________________,
    drawtri_persp________________________TEXEL,
    drawtri_persp___________________VBRI______,
    drawtri_persp___________________VBRI_TEXEL,
    drawtri_persp______________FBRI___________,
    drawtri_persp______________FBRI______TEXEL,
    drawtri_persp______________FBRI_VBRI______,
    drawtri_persp______________FBRI_VBRI_TEXEL,
    drawtri_persp________HILIT________________,
    drawtri_persp________HILIT___________TEXEL,
    drawtri_persp________HILIT______VBRI______,
    drawtri_persp________HILIT______VBRI_TEXEL,
    drawtri_persp________HILIT_FBRI___________,
    drawtri_persp________HILIT_FBRI______TEXEL,
    drawtri_persp________HILIT_FBRI_VBRI______,
    drawtri_persp________HILIT_FBRI_VBRI_TEXEL,
    drawtri_persp__COUNT______________________,
    drawtri_persp__COUNT_________________TEXEL,
    drawtri_persp__COUNT____________VBRI______,
    drawtri_persp__COUNT____________VBRI_TEXEL,
    drawtri_persp__COUNT_______FBRI___________,
    drawtri_persp__COUNT_______FBRI______TEXEL,
    drawtri_persp__COUNT_______FBRI_VBRI______,
    drawtri_persp__COUNT_______FBRI_VBRI_TEXEL,
    drawtri_persp__COUNT_HILIT________________,
    drawtri_persp__COUNT_HILIT___________TEXEL,
    drawtri_persp__COUNT_HILIT______VBRI______,
    drawtri_persp__COUNT_HILIT______VBRI_TEXEL,
    drawtri_persp__COUNT_HILIT_FBRI___________,
    drawtri_persp__COUNT_HILIT_FBRI______TEXEL,
    drawtri_persp__COUNT_HILIT_FBRI_VBRI______,
    drawtri_persp__COUNT_HILIT_FBRI_VBRI_TEXEL,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    NULL,

    [TRIFLAG_ORTHO] =
    drawtri_ortho_____________________________,
    drawtri_ortho________________________TEXEL,
    drawtri_ortho___________________VBRI______,
    drawtri_ortho___________________VBRI_TEXEL,
    drawtri_ortho______________FBRI___________,
    drawtri_ortho______________FBRI______TEXEL,
    drawtri_ortho______________FBRI_VBRI______,
    drawtri_ortho______________FBRI_VBRI_TEXEL,
    drawtri_ortho________HILIT________________,
    drawtri_ortho________HILIT___________TEXEL,
    drawtri_ortho________HILIT______VBRI______,
    drawtri_ortho________HILIT______VBRI_TEXEL,
    drawtri_ortho________HILIT_FBRI___________,
    drawtri_ortho________HILIT_FBRI______TEXEL,
    drawtri_ortho________HILIT_FBRI_VBRI______,
    drawtri_ortho________HILIT_FBRI_VBRI_TEXEL,
    drawtri_ortho__COUNT______________________,
    drawtri_ortho__COUNT_________________TEXEL,
    drawtri_ortho__COUNT____________VBRI______,
    drawtri_ortho__COUNT____________VBRI_TEXEL,
    drawtri_ortho__COUNT_______FBRI___________,
    drawtri_ortho__COUNT_______FBRI______TEXEL,
    drawtri_ortho__COUNT_______FBRI_VBRI______,
    drawtri_ortho__COUNT_______FBRI_VBRI_TEXEL,
    drawtri_ortho__COUNT_HILIT________________,
    drawtri_ortho__COUNT_HILIT___________TEXEL,
    drawtri_ortho__COUNT_HILIT______VBRI______,
    drawtri_ortho__COUNT_HILIT______VBRI_TEXEL,
    drawtri_ortho__COUNT_HILIT_FBRI___________,
    drawtri_ortho__COUNT_HILIT_FBRI______TEXEL,
    drawtri_ortho__COUNT_HILIT_FBRI_VBRI______,
    drawtri_ortho__COUNT_HILIT_FBRI_VBRI_TEXEL,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    drawtri_skydraw,
    NULL,
};
//#define SHOWN_SUBSPAN_ENDPOINTS
#define SUBSPAN_SHIFT 4u
#define TEXEL_PREC 5
#define SOURCE_INCLUSION

#define DRAWTRI_FUNCTION drawtri_persp_____________________________
#undef TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________________________TEXEL
#define TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp___________________VBRI______
#undef TEXEL
#define VBRI
#undef FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp___________________VBRI_TEXEL
#define TEXEL
#define VBRI
#undef FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp______________FBRI___________
#undef TEXEL
#undef VBRI
#define FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp______________FBRI______TEXEL
#define TEXEL
#undef VBRI
#define FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp______________FBRI_VBRI______
#undef TEXEL
#define VBRI
#define FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp______________FBRI_VBRI_TEXEL
#define TEXEL
#define VBRI
#define FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________HILIT________________
#undef TEXEL
#undef VBRI
#undef FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________HILIT___________TEXEL
#define TEXEL
#undef VBRI
#undef FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________HILIT______VBRI______
#undef TEXEL
#define VBRI
#undef FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________HILIT______VBRI_TEXEL
#define TEXEL
#define VBRI
#undef FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________HILIT_FBRI___________
#undef TEXEL
#undef VBRI
#define FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________HILIT_FBRI______TEXEL
#define TEXEL
#undef VBRI
#define FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________HILIT_FBRI_VBRI______
#undef TEXEL
#define VBRI
#define FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp________HILIT_FBRI_VBRI_TEXEL
#define TEXEL
#define VBRI
#define FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT______________________
#undef TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_________________TEXEL
#define TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT____________VBRI______
#undef TEXEL
#define VBRI
#undef FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT____________VBRI_TEXEL
#define TEXEL
#define VBRI
#undef FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_______FBRI___________
#undef TEXEL
#undef VBRI
#define FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_______FBRI______TEXEL
#define TEXEL
#undef VBRI
#define FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_______FBRI_VBRI______
#undef TEXEL
#define VBRI
#define FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_______FBRI_VBRI_TEXEL
#define TEXEL
#define VBRI
#define FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_HILIT________________
#undef TEXEL
#undef VBRI
#undef FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_HILIT___________TEXEL
#define TEXEL
#undef VBRI
#undef FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_HILIT______VBRI______
#undef TEXEL
#define VBRI
#undef FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_HILIT______VBRI_TEXEL
#define TEXEL
#define VBRI
#undef FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_HILIT_FBRI___________
#undef TEXEL
#undef VBRI
#define FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_HILIT_FBRI______TEXEL
#define TEXEL
#undef VBRI
#define FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_HILIT_FBRI_VBRI______
#undef TEXEL
#define VBRI
#define FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_persp__COUNT_HILIT_FBRI_VBRI_TEXEL
#define TEXEL
#define VBRI
#define FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_persp.c"
#undef DRAWTRI_FUNCTION




#define VA_PREC 8

#define DRAWTRI_FUNCTION drawtri_ortho_____________________________
#undef TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________________________TEXEL
#define TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho___________________VBRI______
#undef TEXEL
#define VBRI
#undef FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho___________________VBRI_TEXEL
#define TEXEL
#define VBRI
#undef FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho______________FBRI___________
#undef TEXEL
#undef VBRI
#define FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho______________FBRI______TEXEL
#define TEXEL
#undef VBRI
#define FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho______________FBRI_VBRI______
#undef TEXEL
#define VBRI
#define FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho______________FBRI_VBRI_TEXEL
#define TEXEL
#define VBRI
#define FBRI
#undef HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________HILIT________________
#undef TEXEL
#undef VBRI
#undef FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________HILIT___________TEXEL
#define TEXEL
#undef VBRI
#undef FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________HILIT______VBRI______
#undef TEXEL
#define VBRI
#undef FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________HILIT______VBRI_TEXEL
#define TEXEL
#define VBRI
#undef FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________HILIT_FBRI___________
#undef TEXEL
#undef VBRI
#define FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________HILIT_FBRI______TEXEL
#define TEXEL
#undef VBRI
#define FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________HILIT_FBRI_VBRI______
#undef TEXEL
#define VBRI
#define FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho________HILIT_FBRI_VBRI_TEXEL
#define TEXEL
#define VBRI
#define FBRI
#define HILIT
#undef COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT______________________
#undef TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_________________TEXEL
#define TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT____________VBRI______
#undef TEXEL
#define VBRI
#undef FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT____________VBRI_TEXEL
#define TEXEL
#define VBRI
#undef FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_______FBRI___________
#undef TEXEL
#undef VBRI
#define FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_______FBRI______TEXEL
#define TEXEL
#undef VBRI
#define FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_______FBRI_VBRI______
#undef TEXEL
#define VBRI
#define FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_______FBRI_VBRI_TEXEL
#define TEXEL
#define VBRI
#define FBRI
#undef HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_HILIT________________
#undef TEXEL
#undef VBRI
#undef FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_HILIT___________TEXEL
#define TEXEL
#undef VBRI
#undef FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_HILIT______VBRI______
#undef TEXEL
#define VBRI
#undef FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_HILIT______VBRI_TEXEL
#define TEXEL
#define VBRI
#undef FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_HILIT_FBRI___________
#undef TEXEL
#undef VBRI
#define FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_HILIT_FBRI______TEXEL
#define TEXEL
#undef VBRI
#define FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_HILIT_FBRI_VBRI______
#undef TEXEL
#define VBRI
#define FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION

#define DRAWTRI_FUNCTION drawtri_ortho__COUNT_HILIT_FBRI_VBRI_TEXEL
#define TEXEL
#define VBRI
#define FBRI
#define HILIT
#define COUNT
#include "src/draw/templates/drawtri_ortho.c"
#undef DRAWTRI_FUNCTION


#undef TEXEL
#undef VBRI
#undef FBRI
#undef HILIT
#undef COUNT

#undef SUBSPAN_SHIFT
#undef TEXEL_PREC
#undef SOURCE_INCLUSION
