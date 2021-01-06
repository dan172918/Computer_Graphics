#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/timeb.h>
#include <GL/glut.h>
#include "gltb.h"
#include "glm.h"
#include "dirent32.h"

#pragma comment( linker, "/entry:\"mainCRTStartup\"" )  // set the entry point to be main()

#define DATA_DIR "data/"
#define CLK_TCK 1000

char* model_file = NULL;		/* name of the obect file */
GLuint     model_list = 0;		    /* display list for object */
GLMmodel* model;			        /* glm model data structure */
GLfloat    scale;			        /* original scale factor */
GLfloat    smoothing_angle = 90.0;	/* smoothing angle */
GLfloat    weld_distance = 0.00001;	/* epsilon for welding vertices */
GLboolean  facet_normal = GL_FALSE;	/* draw with facet normal? */
GLboolean  bounding_box = GL_FALSE;	/* bounding box on? */
GLboolean  performance = GL_FALSE;	/* performance counter on? */
GLboolean  stats = GL_FALSE;		/* statistics on? */
GLuint     material_mode = 0;		/* 0=none, 1=color, 2=material */
GLint      entries = 0;			    /* entries in model menu */
GLdouble   pan_x = 0.0;
GLdouble   pan_y = 0.0;
GLdouble   pan_z = 0.0;



float elapsed(void)
{
    static long begin = 0;
    static long finish, difference;

    static struct timeb tb;
    ftime(&tb);
    finish = tb.time * 1000 + tb.millitm;

    difference = finish - begin;
    begin = finish;

    return (float)difference / (float)CLK_TCK;
}

void shadowtext(int x, int y, char* s)  //顯示文字
{
    int lines;
    char* p;

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH),
        0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3ub(0, 0, 0);
    glRasterPos2i(x + 1, y - 1);
    for (p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x + 1, y - 1 - (lines * 18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glColor3ub(0, 128, 255);
    glRasterPos2i(x, y);
    for (p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x, y - (lines * 18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

void lists(void)
{
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
    GLfloat specular[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat shininess = 65.0;

    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);

    if (model_list)
        glDeleteLists(model_list, 1);

    /* generate a list */
    if (material_mode == 0) {   //material_mode = 0 = none
        if (facet_normal)   //判斷顯示平滑或凹凸
            model_list = glmList(model, GLM_FLAT);
        else
            model_list = glmList(model, GLM_SMOOTH);
    }
    else if (material_mode == 1) {  //material_mode = 1 = color
        if (facet_normal)   //判斷顯示平滑或凹凸
            model_list = glmList(model, GLM_FLAT | GLM_COLOR);
        else
            model_list = glmList(model, GLM_SMOOTH | GLM_COLOR);
    }
    else if (material_mode == 2) {  //material_mode = 2 = material
        if (facet_normal)   //判斷顯示平滑或凹凸
            model_list = glmList(model, GLM_FLAT | GLM_MATERIAL);
        else
            model_list = glmList(model, GLM_SMOOTH | GLM_MATERIAL);
    }
}

void init(void)
{
    gltbInit(GLUT_LEFT_BUTTON);

    /* read in the model */
    model = glmReadOBJ(model_file);
    scale = glmUnitize(model);
    glmFacetNormals(model);
    glmVertexNormals(model, smoothing_angle);

    if (model->nummaterials > 0)
        material_mode = 2;

    /* create new display lists */
    lists();

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
}

void reshape(int width, int height) //視窗初始設定
{
    gltbReshape(width, height);

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -3.0);
}

#define NUM_FRAMES 5
void display(void)
{
    static char s[256], t[32];
    static char* p;
    static int frames = 0;

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(pan_x, pan_y, 0.0);
    gltbMatrix();

#if 0   /* glmDraw() performance test */
    if (material_mode == 0) {   //material_mode = 0 = none
        if (facet_normal)   //判斷顯示平滑或凹凸
            glmDraw(model, GLM_FLAT);
        else
            glmDraw(model, GLM_SMOOTH);
    }
    else if (material_mode == 1) {  //material_mode = 1 = color
        if (facet_normal)   //判斷顯示平滑或凹凸
            glmDraw(model, GLM_FLAT | GLM_COLOR);
        else
            glmDraw(model, GLM_SMOOTH | GLM_COLOR);
    }
    else if (material_mode == 2) {  //material_mode = 2 = material
        if (facet_normal)   //判斷顯示平滑或凹凸
            glmDraw(model, GLM_FLAT | GLM_MATERIAL);
        else
            glmDraw(model, GLM_SMOOTH | GLM_MATERIAL);
    }
#else
    glCallList(model_list);
#endif

    glDisable(GL_LIGHTING);
    if (bounding_box) { //顯示模型外方塊
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glColor4f(1.0, 1.0, 0.0, 0.25);
        glutSolidCube(2.0);
        glDisable(GL_BLEND);
    }

    glPopMatrix();

    if (stats) {    //模型基本資料
        /* XXX - this could be done a _whole lot_ faster... */
        int height = glutGet(GLUT_WINDOW_HEIGHT);
        glColor3ub(0, 0, 0);
        sprintf(s, "%s\n%d vertices\n%d triangles\n%d normals\n"
            "%d texcoords\n%d groups\n%d materials",
            model->pathname, model->numvertices, model->numtriangles,
            model->numnormals, model->numtexcoords, model->numgroups,
            model->nummaterials);
        shadowtext(5, height - (5 + 18 * 1), s);
    }

    /*顯示幀數*/
    frames++;
    if (frames > NUM_FRAMES) {
        sprintf(t, "%g fps", frames / elapsed());
        frames = 0;
    }
    if (performance) {
        shadowtext(5, 5, t);
    }

    glutSwapBuffers();
    glEnable(GL_LIGHTING);
}

void keyboard(unsigned char key, int x, int y)
{
    GLint params[2];

    switch (key) {
    case 'h':
        printf("help\n\n");
        printf("w         -  Toggle wireframe/filled\n");
        printf("c         -  Toggle culling\n");
        printf("n         -  Toggle facet/smooth normal\n");
        printf("b         -  Toggle bounding box\n");
        printf("r         -  Reverse polygon winding\n");
        printf("m         -  Toggle color/material/none mode\n");
        printf("p         -  Toggle performance indicator\n");
        printf("s/S       -  Scale model smaller/larger\n");
        printf("t         -  Show model stats\n");
        printf("+/-       -  Increase/decrease smoothing angle\n");
        printf("W         -  Write model to file (out.obj)\n");
        printf("q/escape  -  Quit\n\n");
        break;

    case 't':
        stats = !stats;
        break;

    case 'p':
        performance = !performance;
        break;

    case 'm':
        material_mode++;
        if (material_mode > 2)
            material_mode = 0;
        printf("material_mode = %d\n", material_mode);
        lists();
        break;

    case 'd':
        glmDelete(model);
        init();
        lists();
        break;

    case 'w':
        glGetIntegerv(GL_POLYGON_MODE, params);
        if (params[0] == GL_FILL)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;

    case 'c':
        if (glIsEnabled(GL_CULL_FACE))
            glDisable(GL_CULL_FACE);
        else
            glEnable(GL_CULL_FACE);
        break;

    case 'b':
        bounding_box = !bounding_box;
        break;

    case 'n':
        facet_normal = !facet_normal;
        lists();
        break;

    case 'r':
        glmReverseWinding(model);
        lists();
        break;

    case 's':
        glmScale(model, 0.8);
        lists();
        break;

    case 'S':
        glmScale(model, 1.25);
        lists();
        break;

    case '-':
        smoothing_angle -= 1.0;
        printf("Smoothing angle: %.1f\n", smoothing_angle);
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;

    case '+':
        smoothing_angle += 1.0;
        printf("Smoothing angle: %.1f\n", smoothing_angle);
        glmVertexNormals(model, smoothing_angle);
        lists();
        break;

    case 'W':
        glmScale(model, 1.0 / scale);
        glmWriteOBJ(model, "out.obj", GLM_SMOOTH | GLM_MATERIAL);
        break;

    case 'R':
    {
        GLuint i;
        GLfloat swap;
        for (i = 1; i <= model->numvertices; i++) {
            swap = model->vertices[3 * i + 1];
            model->vertices[3 * i + 1] = model->vertices[3 * i + 2];
            model->vertices[3 * i + 2] = -swap;
        }
        glmFacetNormals(model);
        lists();
        break;
    }

    case 27:
        exit(0);
        break;
    }

    glutPostRedisplay();
}

void menu(int item) //右鍵選單以及一些模型的初始設定
{
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;

    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    }
    else {
        dirp = opendir(DATA_DIR);
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;
        name = (char*)malloc(strlen(direntp->d_name) + strlen(DATA_DIR) + 1);
        strcpy(name, DATA_DIR);
        strcat(name, direntp->d_name);
        model = glmReadOBJ(name);
        scale = glmUnitize(model);
        glmFacetNormals(model);
        glmVertexNormals(model, smoothing_angle);

        if (model->nummaterials > 0)
            material_mode = 2;
        else
            material_mode = 0;

        lists();
        free(name);

        glutPostRedisplay();
    }
}

static GLint      mouse_state;
static GLint      mouse_button;

void mouse(int button, int state, int x, int y)
{
    GLdouble model[4 * 4];
    GLdouble proj[4 * 4];
    GLint view[4];

    /* fix for two-button mice -- left mouse + shift = middle mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
        button = GLUT_MIDDLE_BUTTON;

    gltbMouse(button, state, x, y);

    mouse_state = state;
    mouse_button = button;
    //如果滑鼠按下或放開
    if (state == GLUT_DOWN && button == GLUT_MIDDLE_BUTTON) {
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, view);
        gluProject((GLdouble)x, (GLdouble)y, 0.0,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        pan_y = -pan_y;
    }

    glutPostRedisplay();
}

void motion(int x, int y)
{
    GLdouble model[4 * 4];
    GLdouble proj[4 * 4];
    GLint view[4];

    gltbMotion(x, y);
    //如果滑鼠按下或放開
    if (mouse_state == GLUT_DOWN && mouse_button == GLUT_MIDDLE_BUTTON) {
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, view);
        gluProject((GLdouble)x, (GLdouble)y, 0.0,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        pan_y = -pan_y;
    }

    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    int buffering = GLUT_DOUBLE;
    struct dirent* direntp;
    DIR* dirp;
    int models;

    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);

    while (--argc) {
        if (strcmp(argv[argc], "-sb") == 0)
            buffering = GLUT_SINGLE;
        else
            model_file = argv[argc];
    }

    if (!model_file) {
        model_file = "data/soccerball.obj";     //預設使用的模型
    }

    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | buffering);
    glutCreateWindow("HW2");    //視窗名稱

    glutReshapeFunc(reshape);   //視窗大小設置
    glutDisplayFunc(display);   //模型顯示
    glutKeyboardFunc(keyboard); //案件觸發
    glutMouseFunc(mouse);       //滑鼠起始點
    glutMotionFunc(motion);     //滑鼠終點

    models = glutCreateMenu(menu);
    dirp = opendir(DATA_DIR);
    if (!dirp) {
        fprintf(stderr, "%s: can't open data directory.\n", argv[0]);
    }
    else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }

    glutCreateMenu(menu);   //3D模型選單
    glutAddSubMenu("3D Models", models);
    glutAddMenuEntry("[w]   wireframe (線框) / filled (預設:填充)", 'w');
    glutAddMenuEntry("[c]   culling on / off (可看到3D Models內部)", 'c');
    glutAddMenuEntry("[n]   face/smooth (凹凸面/平滑面)", 'n');
    glutAddMenuEntry("[b]   bounding box on/off (3D Models外接透明方塊)", 'b');
    glutAddMenuEntry("[p]   frame rate on/off (顯示FPS幀數)", 'p');
    glutAddMenuEntry("[t]   Toggle model statistics (3D Models的訊息)", 't');
    glutAddMenuEntry("[m]   Toggle color/material/none mode (顏色/材質/無)", 'm');
    glutAddMenuEntry("[r]   Reverse polygon winding (只看到3D Models的內部)", 'r');
    glutAddMenuEntry("[s]   Scale model smaller (變小)", 's');
    glutAddMenuEntry("[S]   Scale model larger (變大)", 'S');
    glutAddMenuEntry("[+]   Increase smoothing angle (平滑程度升高)", '+');
    glutAddMenuEntry("[-]   Decrease smoothing angle (平滑程度降低)", '-');
    glutAddMenuEntry("[W]   Write model to file (out.obj)(匯出3D Models)", 'W');
    glutAddMenuEntry("[Esc] Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    init();

    glutMainLoop();
    return 0;
}
