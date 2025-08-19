#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <GL/freeglut.h>
#include <ctime>
#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

using namespace std;

/* ====== Ventana (resolución lógica) ====== */
const int winWidth = 512, winHeight = 600;
const int numCarriles = 4;
const int anchoCarril = 105;
float carriles[numCarriles];

/* ====== Jugador / juego ====== */
float jugadorX;
float jugadorY = 50;
bool  gameOver = false;
float desplazamientoScroll = 0.0f;
int   puntaje = 0;
int   nivelDificultad = 0;
float velocidadScrollBase = 5.0f;

const float jugadorWidth  = 42.0f;
const float jugadorHeight = 75.0f;

// Tamaño del SPRITE TXT del jugador
const int   JUGADOR_SPRITE_ALTO   = 27;
const float JUGADOR_SPRITE_ESCALA = 2.9f;

// HITBOX
float jugadorHitboxScale = 1.0f;

bool teclasPresionadas[256] = { false };
bool bonusActivo = false;
bool puedeActivarBonus = true;
int  tiempoBonusRestante = 0;
float bonusY = 0.0f;

/* ====== Estados de menús ====== */
bool enMenuInicio = true;          // Menú de inicio / intro
bool animLogoActiva = true;        // Animación del logo
bool mostrarOpcionesMenu = false;  // Se muestra cuando el logo ya subió lo suficiente
bool enInstrucciones = false;      // Pantalla de instrucciones
bool enPuntuaciones = false;       // Pantalla de puntuaciones
int  opcionMenuInicio = 0;         // 0..3

int  opcionGameOver = 0;           // 0=Sí, 1=Salir a menú

// Entrada de nombre tras choque
bool  solicitandoNombre = false;
string nombreBuffer = "";

/* ====== Pausa ====== */
bool juegoPausado = false;
bool mostrarMenuPausa = false;
bool menuOpcionesActivo = false;
bool menuConfirmResetActivo = false;
bool menuConfirmSalirActivo = false;   // confirmar salir al menú
int  opcionSeleccionada = 0;
int  opcionSeleccionadaOpc = 0;    // (0=musica,1=efectos,2=volver)
int  opcionConfirmReset = 0;
int  opcionConfirmSalir = 1;        // 0=Si, 1=No
const int totalOpciones = 4;

/* ====== Audio ====== */
Mix_Music* musicaIntro     = nullptr; // Música para menú/intro
Mix_Music* musicaFondo     = nullptr; // Música de juego
Mix_Music* musicaGameOver  = nullptr; // Música de game over

int volumenMusica  = MIX_MAX_VOLUME;
int volumenEfectos = MIX_MAX_VOLUME;

Mix_Chunk* sfxCambio   = nullptr;   // cambio por puntaje 60/120/...
Mix_Chunk* sfxPowerUp  = nullptr;   // ambos poderes
Mix_Chunk* sfxChoque   = nullptr;   // choque
Mix_Chunk* sfxMenuMove = nullptr;    // pasar entre opciones
Mix_Chunk* sfxMenuSel  = nullptr;   // seleccionar opción

/* ====== Power-Ups ====== */
const float FPS_APROX = 60.0f;

/* 2X */
const float X2_DURACION_SEG    = 6.75f;
const int   X2_DURACION_FRAMES = (int)(X2_DURACION_SEG * FPS_APROX);
const float X2_SPAWN_MIN_SEG   = 8.5f;
const float X2_SPAWN_MAX_SEG   = 12.5f;

/* Invisibilidad */
const float INVIS_DURACION_SEG    = 8.0f;
const int   INVIS_DURACION_FRAMES = (int)(INVIS_DURACION_SEG * FPS_APROX);
const int   INVIS_MIN_SCORE       = 180;
const float INVIS_SPAWN_PROB      = 0.50f;

/* Opacidad invisibilidad */
const float INVIS_ALPHA = 0.45f;

/* Letterbox (barras superior/inferior del HUD) */
const float BARRA_ALTO  = 40.0f;
const float BARRA_ALPHA = 1.0f;

/* Barra de poder (HUD) */
const float POWER_BAR_W      = 160.0f;
const float POWER_BAR_H      = 12.0f;
const float POWER_BAR_MARGIN = 10.0f;
const float POWER_BAR_GAP    = 6.0f;

/* Icono HUD */
const float POWER_ICON_W   = 36.0f;
const float POWER_ICON_H   = 22.0f;
const float POWER_ICON_GAP = 6.0f;

const char* HUD_X2_TXT    = "X2.txt";
const char* HUD_GHOST_TXT = "Fantasma.txt";

/* Estados de poderes */
bool x2Activo = false;  int x2FramesRestantes = 0;
bool invisActivo = false; int invisFramesRestantes = 0;

/* ====== Banner START ====== */
bool startBannerActivo = false;
int  startBannerFrames = 0;
const int START_BANNER_FRAMES = (int)(1.0f * FPS_APROX);

/* ====== TRANSICIONES (FADE) ====== */
const int FADE_TOTAL_FRAMES = (int)(0.50f * FPS_APROX); // 0.5s
enum FadePhase { FADE_IDLE = 0, FADE_OUT = 1, FADE_IN = 2 };
FadePhase fadePhase = FADE_IDLE;
int   fadeFrames = 0;
float fadeAlpha  = 0.0f;

/* ====== Acciones pendientes durante el fade ====== */
enum PendingAction {
    ACT_NONE = 0,
    ACT_TO_PLAY_FROM_MENU,   // Iniciar desde menú
    ACT_TO_NAME_ENTRY,       // Tras choque -> panel nombre
    ACT_TO_GAMEOVER_MENU,    // (no se usa)
    ACT_RESTART_LEVEL,       // Reintentar / Reiniciar nivel
    ACT_TO_MAIN_MENU         // Salir al menú
};
PendingAction pendingAction = ACT_NONE;

/* ====== Congelar juego al chocar ====== */
bool congeladoPorChoque = false;

/* ====== Menú: separaciones respecto del logo ====== */
const float MENU_OPTIONS_OFFSET = 100.0f; // principales más abajo
const float MENU_INFO_OFFSET    = 135.0f; // info (instr/puntos) más abajo

/* ====== Explosión (al chocar) ====== */
bool  explosionActiva = false;
int   explosionFrames = 0;
const int EXPLOSION_TOTAL_FRAMES = (int)(0.75f * FPS_APROX); // ~0.75s
float explosionX = 0.0f, explosionY = 0.0f;
int   enemigoChocado = -1; // índice del enemigo que chocó (para ocultarlo)

/* ====== Game Over sprite ajustes ====== */
vector<string> goLines;
int   goCols = 0, goRows = 0;
float goEscala = 1.0f;
float goAlpha  = 0.0f;     // 0..1
int   goDir    = +1;       // +1 fade-in, -1 fade-out
const char* GO_TXT_PATH = "Game_Over.txt";
/* offsets */
const float GO_X_OFFSET         = -22.0f;   // desplazar un poco a la derecha
const float GO_OPTIONS_Y_EXTRA  = -60.0f;  // bajar las opciones

/* ====== Posición fija de pantallas de ayuda ====== */
const float INSTR_TITLE_Y      = 300.0f; // Título "Instrucciones"
const float INSTR_LINE_STEP    = 28.0f;  // Espaciado entre líneas
const float PUNTS_TITLE_Y      = 300.0f; // Título "Puntuaciones"
const float PUNTS_FIRST_OFFSET = 38.0f;  // Distancia del título a la 1ª fila
const float PUNTS_ROW_STEP     = 24.0f;  // Espaciado entre filas

/* Bola poder */
enum PowerType { POWER_NONE = 0, POWER_X2 = 1, POWER_INVIS = 2 };
struct PowerUp {
    float cx, cy, radio, velocidad;
    bool  activo;
    PowerType tipo;
};
PowerUp powerOrb = {0,0,12.0f,4.0f,false, POWER_NONE};
int framesParaPowerUp = (int)(4.0f * FPS_APROX);

/* ====== Puntuaciones ====== */
const char* SCORES_FILE = "scores.txt";
int hiScore = 0; // para HUD

/* ====== LOGO (TXT) ====== */
const char* LOGO_TXT_PATH = "ENDLESS_RUNNER.txt";
vector<string> logoLines;
int   logoCols = 0, logoRows = 0;
float logoEscala = 1.0f;
float logoY = -200.0f;             // empieza por debajo
const float LOGO_VEL = 1.0f;       // velocidad de subida
const float LOGO_ANCLA_FRACCION = 0.80f; // tope del logo (0..1 pant)

/* ====== Prototipos ====== */
bool hayColision(float x1, float y1, float w1, float h1,
                 float x2, float y2, float w2, float h2);
bool colisionCirculoRect(float cx, float cy, float r,
                         float rx, float ry, float rw, float rh);
bool respetaSeparacionVertical(float nuevoY, float altura);
static void drawPanel(float x1, float y1, float x2, float y2);
void comenzarConStartBanner();
static void dibujarExplosion(float cx, float cy, float t);
void reshape(int w, int h); // mantener aspecto

/* ====== Utilidad dibujo TXT ====== */
static inline bool colorDeChar(char c) {
    switch (c) {
        case 'N': case 'n': glColor3f(0.12f, 0.12f, 0.12f); return true; // negro
        case 'W': case 'w': glColor3f(1.0f, 1.0f, 1.0f);     return true; // blanco
        case 'T': case 't': case 'R': case 'r': glColor3f(0.85f, 0.00f, 0.00f); return true; // rojo
        case 'A': case 'a': case 'Y': case 'y': glColor3f(1.00f, 0.90f, 0.20f); return true; // amarillo
        case 'E': case 'e': glColor3f(0.95f, 0.55f, 0.10f);  return true; // naranja
        case 'L': case 'l': case 'B': case 'b': case 'C': case 'c':
            glColor3f(0.20f, 0.60f, 1.00f);  return true; // azul claro (color del logo)
        case '_': case ' ': case '.': return false;        // transparente
        default:            return false;
    }
}
static inline bool colorDeCharAlpha(char c, float a) {
    switch (c) {
        case 'N': case 'n': glColor4f(0.12f, 0.12f, 0.12f, a); return true;
        case 'W': case 'w': glColor4f(1.0f, 1.0f, 1.0f, a);     return true;
        case 'T': case 't': case 'R': case 'r': glColor4f(0.90f, 0.05f, 0.05f, a); return true;
        case 'A': case 'a': case 'Y': case 'y': glColor4f(1.00f, 0.90f, 0.20f, a); return true;
        case 'E': case 'e': glColor4f(0.95f, 0.55f, 0.10f, a);  return true;
        case 'L': case 'l': case 'B': case 'b': case 'C': case 'c':
            glColor4f(0.20f, 0.60f, 1.00f, a);  return true;
        case '_': case ' ': case '.': return false;
        default:            return false;
    }
}
static void drawText(float x, float y, const char* txt, void* font = GLUT_BITMAP_HELVETICA_18){
    glRasterPos2f(x,y);
    for (int i=0; txt[i]; ++i) glutBitmapCharacter(font, txt[i]);
}
static int textWidth(const char* s, void* font = GLUT_BITMAP_HELVETICA_18){
    int w = 0; const unsigned char* p = (const unsigned char*)s;
    while (*p) { w += glutBitmapWidth(font, *p); ++p; }
    return w;
}

/* TXT pre-cargado (logo / go) */
bool cargarTxt(const char* path, vector<string>& outLines, int& cols, int& rows){
    ifstream f(path);
    if (!f.is_open()) return false;
    outLines.clear(); cols = 0; rows = 0;
    string line;
    while (getline(f, line)){
        cols = max(cols, (int)line.size());
        outLines.push_back(line);
    }
    rows = (int)outLines.size();
    return true;
}
void dibujarTxtPreloaded(const vector<string>& lines, float x, float y, float escala){
    int rows = (int)lines.size();
    for (int r=0; r<rows; ++r){
        const string& L = lines[r];
        for (int c=0; c<(int)L.size(); ++c){
            char ch = L[c];
            if (!colorDeChar(ch)) continue;
            float px = x + c * escala;
            float py = y + (rows - 1 - r) * escala;
            glBegin(GL_QUADS);
                glVertex2f(px,            py);
                glVertex2f(px + escala,   py);
                glVertex2f(px + escala,   py + escala);
                glVertex2f(px,            py + escala);
            glEnd();
        }
    }
}
void dibujarTxtPreloadedAlpha(const vector<string>& lines, float x, float y, float escala, float alpha){
    int rows = (int)lines.size();
    for (int r=0; r<rows; ++r){
        const string& L = lines[r];
        for (int c=0; c<(int)L.size(); ++c){
            char ch = L[c];
            if (!colorDeCharAlpha(ch, alpha)) continue;
            float px = x + c * escala;
            float py = y + (rows - 1 - r) * escala;
            glBegin(GL_QUADS);
                glVertex2f(px,            py);
                glVertex2f(px + escala,   py);
                glVertex2f(px + escala,   py + escala);
                glVertex2f(px,            py + escala);
            glEnd();
        }
    }
}
/* === NUEVO: versión con paleta del logo para Game Over === */
void dibujarTxtPreloadedAlphaLogoColor(const vector<string>& lines, float x, float y, float escala, float alpha){
    int rows = (int)lines.size();
    for (int r=0; r<rows; ++r){
        const string& L = lines[r];
        for (int c=0; c<(int)L.size(); ++c){
            char ch = L[c];
            if (ch==' ' || ch=='_') continue;

            if (ch=='W'||ch=='w')          glColor4f(.089f,0.079f,0.089f,alpha);   // "Brillo"
            else if (ch=='N'||ch=='n')     glColor4f(0.12f,0.12f,0.12f,alpha);     // "Sombra"
            else                           glColor4f(0.95f, 0.55f, 0.10f, alpha);  // Naranja   

            float px = x + c * escala;
            float py = y + (rows - 1 - r) * escala;
            glBegin(GL_QUADS);
                glVertex2f(px,            py);
                glVertex2f(px + escala,   py);
                glVertex2f(px + escala,   py + escala);
                glVertex2f(px,            py + escala);
            glEnd();
        }
    }
}

/* ====== PUNTUACIONES (con nombre) ====== */
struct Entry { string name; int score; };

static inline string trim(const string& s){
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a==string::npos) return "";
    return s.substr(a, b-a+1);
}

void leerPuntuaciones(vector<Entry>& v){
    v.clear();
    ifstream in(SCORES_FILE);
    if (!in.is_open()) return;
    string line;
    while (getline(in, line)){
        line = trim(line);
        if (line.empty()) continue;
        int score = 0;
        string name = "PLAYER";
        size_t p = line.find_last_of("; \t");
        if (p != string::npos){
            string left = trim(line.substr(0,p));
            string right = trim(line.substr(p+1));
            try { score = stoi(right); } catch(...) { continue; }
            name = left.empty() ? "PLAYER" : left;
        } else {
            try { score = stoi(line); } catch(...) { continue; }
            name = "PLAYER";
        }
        v.push_back({name, score});
    }
    sort(v.begin(), v.end(), [](const Entry& a, const Entry& b){ return a.score > b.score; });
    if (v.size() > 10) v.resize(10);
}

int cargarHiScore(){
    vector<Entry> v; leerPuntuaciones(v);
    return v.empty() ? 0 : v.front().score;
}

void guardarPuntuaciones(const vector<Entry>& v){
    ofstream out(SCORES_FILE, ios::trunc);
    for (const auto& e : v) out << e.name << ";" << e.score << "\n";
}

void registrarPuntajeConNombre(const string& name, int s){
    vector<Entry> v; leerPuntuaciones(v);
    v.push_back({ name.empty() ? string("PLAYER") : name, max(0, s) });
    sort(v.begin(), v.end(), [](const Entry& a, const Entry& b){ return a.score > b.score; });
    if (v.size() > 10) v.resize(10);
    guardarPuntuaciones(v);
    hiScore = v.front().score;
}

/* ====== Enemigos / sprites ====== */
static void dibujarSpriteEnemigoTXT(const char* path, float x, float y, float escala) {
    ifstream f(path);
    if (!f.is_open()) return;
    vector<string> lines; string line; int maxCols = 0;
    while (getline(f, line)) { maxCols = max(maxCols, (int)line.size()); lines.push_back(line); }
    f.close();

    const int rows = (int)lines.size();
    for (int r = 0; r < rows; ++r) {
        const string& L = lines[r];
        for (int c = 0; c < (int)L.size(); ++c) {
            char ch = L[c];
            if (!colorDeChar(ch)) continue;
            float px = x + c * escala;
            float py = y + (rows - 1 - r) * escala;
            glBegin(GL_QUADS);
                glVertex2f(px,            py);
                glVertex2f(px + escala,   py);
                glVertex2f(px + escala,   py + escala);
                glVertex2f(px,            py + escala);
            glEnd();
        }
    }
}
enum HudIconStyle { ICON_NORMAL = 0, ICON_TINT_RED = 1, ICON_GHOST_GRAY = 2 };
static void dibujarSpriteHUDTXT(const char* path, float x, float y, float boxW, float boxH, HudIconStyle style) {
    ifstream f(path);
    if (!f.is_open()) return;
    vector<string> lines; string line; int maxCols = 0;
    while (getline(f, line)) {
        maxCols = max(maxCols, (int)line.size());
        if (!line.empty()) lines.push_back(line);
    }
    f.close();
    if (lines.empty() || maxCols == 0) return;

    int rows = (int)lines.size();
    float escala = min(boxW / maxCols, boxH / rows);

    float dibW = maxCols * escala;
    float dibH = rows    * escala;
    float ox = x + (boxW - dibW) * 0.5f;
    float oy = y + (boxH - dibH) * 0.5f;

    for (int r = 0; r < rows; ++r) {
        const string& L = lines[r];
        for (int c = 0; c < (int)L.size(); ++c) {
            char ch = L[c];
            if (ch == ' ' || ch == '_') continue;

            if (style == ICON_TINT_RED) {
                glColor3f(0.90f, 0.20f, 0.20f);
            } else if (style == ICON_GHOST_GRAY) {
                if      (ch == 'W' || ch == 'w') glColor3f(1.0f, 1.0f, 1.0f);
                else if (ch == 'R' || ch == 'r') glColor3f(0.90f, 0.15f, 0.15f);
                else if (ch == 'N' || ch == 'n') glColor3f(0.12f, 0.12f, 0.12f);
                else                              glColor3f(0.60f, 0.60f, 0.60f);
            } else {
                if (!colorDeChar(ch)) continue;
            }

            float px = ox + c * escala;
            float py = oy + (rows - 1 - r) * escala;
            glBegin(GL_QUADS);
                glVertex2f(px,            py);
                glVertex2f(px + escala,   py);
                glVertex2f(px + escala,   py + escala);
                glVertex2f(px,            py + escala);
            glEnd();
        }
    }
}

/* ====== Enemigos ====== */
struct AutoEnemigo {
    float x, y;
    float width, height;
    float velocidad;
    float colorR, colorG, colorB;
    int carril;
    bool activo;

    bool usaTXT = false;
    const char* txtPath = nullptr;
    float txtEscala = 1.8f;

    float hitboxScale = 0.90f;
};
AutoEnemigo enemigos[numCarriles];

void inicializarCarriles() {
    int espacioTotal = numCarriles * anchoCarril;
    int margenIzquierdo = (winWidth - espacioTotal) / 2;
    for (int i = 0; i < numCarriles; i++) {
        carriles[i] = margenIzquierdo + i * anchoCarril;
    }
}

/* ====== HUD ====== */
void mostrarHiScore(){
    glColor3f(1.0,1.0,1.0);
    stringstream ss; ss << "HI-SCORE: " << hiScore;
    string s = ss.str();
    int tw = textWidth(s.c_str());
    glRasterPos2f((winWidth - tw) * 0.5f, winHeight - 20);
    for (char c : s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}
void mostrarPuntaje() {
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2f(10, winHeight - 20);
    stringstream ss; ss << "Puntaje: " << puntaje;
    for (char c : ss.str()) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}
void mostrarVelocimetro() {
    int velocidadKmh = 40 + (nivelDificultad * 40);
    if (velocidadKmh > 240) velocidadKmh = 240;
    glColor3f(0.0, 1.0, 0.0);
    stringstream ss; ss << "Velocidad: " << velocidadKmh << " km/h";
    string s = ss.str();
    int tw = textWidth(s.c_str());
    glRasterPos2f(winWidth - 10 - tw, winHeight - 20);
    for (char c : s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}
void mostrarBonusTexto() {
    if (bonusActivo && tiempoBonusRestante > 0) {
        glColor3f(1.0, 1.0, 0.0);
        glRasterPos2f(winWidth / 2 - 90, bonusY);
        const char* msg = "+50 puntos, por conduccion temeraria";
        for (int i = 0; msg[i]; i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, msg[i]);
    }
}

/* === Jugador === */
void dibujarAutoDesdeTxt(float x, float y, float escala, float alpha = 1.0f) {
    ifstream archivo("Carrorojo.txt");
    if (!archivo.is_open()) return;
    string linea; int fila = 0;
    const int alto = 27;

    while (getline(archivo, linea)) {
        for (int col = 0; col < (int)linea.size(); col++) {
            char c = linea[col];
            if (c == ' ' || c == '_') continue;

            if (c == 'r') glColor4f(1.0f, 0.0f, 0.0f, alpha);
            else if (c == 'y') glColor4f(1.0f, 1.0f, 0.0f, alpha);
            else if (c == 'n') glColor4f(0.2f, 0.2f, 0.2f, alpha);
            else if (c == 'p') glColor4f(1.0f, 0.5f, 0.5f, alpha);
            else continue;

            float px = x + col * escala;
            float py = y + (alto - fila) * escala;
            glBegin(GL_QUADS);
                glVertex2f(px, py);
                glVertex2f(px + escala, py);
                glVertex2f(px + escala, py + escala);
                glVertex2f(px, py + escala);
            glEnd();
        }
        fila++;
    }
    archivo.close();
}

/* Auto geométrico (enemigos sin TXT) */
void dibujarAuto(float x, float y, float w, float h, float r, float g, float b) {
    glColor3f(r * 0.8f, g * 0.8f, b * 0.8f);
    glBegin(GL_QUADS);
        glVertex2f(x + 5, y);
        glVertex2f(x + w - 5, y);
        glVertex2f(x + w - 10, y + 15);
        glVertex2f(x + 10, y + 15);
    glEnd();
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x + 10, y + 15);
        glVertex2f(x + w - 10, y + 15);
        glVertex2f(x + w - 10, y + h - 25);
        glVertex2f(x + 10, y + h - 25);
    glEnd();
    glColor3f(r * 0.9f, g * 0.9f, b * 0.9f);
    glBegin(GL_QUADS);
        glVertex2f(x + 5, y + h - 25);
        glVertex2f(x + w - 5, y + h - 25);
        glVertex2f(x + w - 10, y + h);
        glVertex2f(x + 10, y + h);
    glEnd();
    glColor3f(0.6f, 0.9f, 1.0f);
    glBegin(GL_QUADS);
        glVertex2f(x + 15, y + h - 50);
        glVertex2f(x + w - 15, y + h - 50);
        glVertex2f(x + w - 15, y + h - 30);
        glVertex2f(x + 15, y + h - 30);
    glEnd();
    glColor3f(0.05f, 0.05f, 0.05f);
    float radio = 6.5f;
    float puntos[4][2] = {{ x + 5, y + 5 },{ x + w - 5, y + 5 },{ x + 5, y + h - 10 },{ x + w - 5, y + h - 10 }};
    for (int i = 0; i < 4; i++) {
        glBegin(GL_POLYGON);
        for (int j = 0; j < 20; j++) {
            float theta = 2.0f * 3.1415926f * j / 20;
            float dx = radio * cosf(theta);
            float dy = radio * sinf(theta);
            glVertex2f(puntos[i][0] + dx, puntos[i][1] + dy);
        }
        glEnd();
    }
}

/* ====== Explosión ====== */
static void circle(float cx, float cy, float r, float aR, float aG, float aB, float aA){
    glColor4f(aR,aG,aB,aA);
    const int seg = 40;
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx,cy);
        for(int i=0;i<=seg;++i){
            float th = 2.0f*3.1415926f*i/seg;
            glVertex2f(cx + cosf(th)*r, cy + sinf(th)*r);
        }
    glEnd();
}
static void dibujarExplosion(float cx, float cy, float t){
    t = max(0.f, min(1.f, t));
    float rCore = 18.0f + 40.0f * t;
    float rGlow = rCore + 18.0f;
    float rSmoke = rGlow + 12.0f;

    circle(cx, cy, rCore, 1.0f, 0.95f, 0.40f, 0.95f);
    circle(cx, cy, rGlow, 1.0f, 0.45f, 0.05f, 0.75f*(1.0f*t*0.6f + 0.4f));

    int spikes = 12;
    glBegin(GL_TRIANGLES);
    for (int i=0;i<spikes;++i){
        float a  = (2.0f*3.1415926f*i)/spikes + t*3.0f;
        float a1 = a - 0.06f, a2 = a + 0.06f;
        float r1 = rGlow*0.7f, r2 = rGlow + 22.0f*(0.4f + 0.6f*t);
        glColor4f(1.0f, 0.55f, 0.10f, 0.85f*(1.0f-t*0.5f));
        glVertex2f(cx + cosf(a)*r2, cy + sinf(a)*r2);
        glColor4f(0.95f, 0.35f, 0.05f, 0.80f*(1.0f-t*0.5f));
        glVertex2f(cx + cosf(a1)*r1, cy + sinf(a1)*r1);
        glVertex2f(cx + cosf(a2)*r1, cy + sinf(a2)*r1);
    }
    glEnd();

    circle(cx, cy, rSmoke, 0.1f, 0.1f, 0.1f, 0.45f*(1.0f-t));

    glPointSize(3.0f);
    glBegin(GL_POINTS);
        for(int i=0;i<26;++i){
            float a = (2.0f*3.1415926f*i)/26.0f;
            float rr = rCore + 10.0f + 60.0f*t;
            float px = cx + cosf(a)*(rr + 6.0f*sinf(t*8.0f + i));
            float py = cy + sinf(a)*(rr + 6.0f*cosf(t*6.5f + i));
            glColor4f(1.0f, 0.9f, 0.3f, 0.85f*(1.0f-t));
            glVertex2f(px, py);
        }
    glEnd();
}

/* ====== MENÚ INICIO ====== */
void dibujarMenuInicio() {
    glColor3f(0,0,0);
    glBegin(GL_QUADS);
        glVertex2f(0,0); glVertex2f(winWidth,0); glVertex2f(winWidth,winHeight); glVertex2f(0,winHeight);
    glEnd();

    float logoW = 0.0f, logoH = 0.0f;
    if (logoRows > 0 && logoCols > 0) {
        logoW = logoCols * logoEscala;
        logoH = logoRows * logoEscala;
        float x = (winWidth - logoW) * 0.5f;
        dibujarTxtPreloaded(logoLines, x, logoY, logoEscala);
    }

    if (mostrarOpcionesMenu && !enInstrucciones && !enPuntuaciones) {
        const char* opciones[] = { "Iniciar", "Instrucciones", "Puntuaciones", "Salir" };
        int count = 4;
        float baseY = max(logoY - MENU_OPTIONS_OFFSET, 90.0f);
        for (int i=0;i<count;++i){
            const char* t = opciones[i];
            int tw = textWidth(t);
            float x = (winWidth - tw) * 0.5f;
            float y = baseY - i*28.0f;
            if (i == opcionMenuInicio) glColor3f(1.0f,1.0f,0.0f);
            else                       glColor3f(1.0f,1.0f,1.0f);
            drawText(x, y, t);
        }
        glColor3f(0.6f,0.6f,0.6f);
        const char* hint = "W/S: mover  |  Enter: seleccionar  |  Esc: salir  |  Enter (intro): saltar animacion";
        drawText((winWidth - textWidth(hint, GLUT_BITMAP_HELVETICA_12))*0.5f, 18, hint, GLUT_BITMAP_HELVETICA_12);
    }

    // ----- INSTRUCCIONES (posición fija) -----
    if (enInstrucciones) {
        float startY = INSTR_TITLE_Y;
        glColor3f(1,1,1);
        const char* title = "Instrucciones";
        drawText((winWidth - textWidth(title))*0.5f, startY, title);

        glColor3f(0.85f,0.85f,0.85f);
        const char* l1 = "Mover: W/A/S/D";
        const char* l2 = "Evita chocar; suma puntos al rebasar y por tiempo";
        const char* l3 = "Orbe rojo: x2 puntos | Orbe azul: invisibilidad";
        const char* l4 = "P: Pausa  |  Enter en menus para confirmar";
        drawText((winWidth - textWidth(l1))*0.5f, startY - (1*INSTR_LINE_STEP), l1);
        drawText((winWidth - textWidth(l2))*0.5f, startY - (2*INSTR_LINE_STEP), l2);
        drawText((winWidth - textWidth(l3))*0.5f, startY - (3*INSTR_LINE_STEP), l3);
        drawText((winWidth - textWidth(l4))*0.5f, startY - (4*INSTR_LINE_STEP), l4);

        glColor3f(0.9f,0.9f,0.9f);
        const char* back = "Esc o Enter: volver";
        drawText((winWidth - textWidth(back,GLUT_BITMAP_HELVETICA_12))*0.5f, 24, back, GLUT_BITMAP_HELVETICA_12);
    }

    // ----- PUNTUACIONES (posición fija) -----
    if (enPuntuaciones) {
        float startY = PUNTS_TITLE_Y; // YA NO depende del logo
        glColor3f(1,1,1);
        const char* title = "Puntuaciones";
        drawText((winWidth - textWidth(title))*0.5f, startY, title);

        vector<Entry> v; leerPuntuaciones(v);
        float y = startY - PUNTS_FIRST_OFFSET;
        for (size_t i=0;i<v.size();++i){
            string s = to_string((int)(i+1)) + ". " + v[i].name + " - " + to_string(v[i].score);
            int tw = textWidth(s.c_str());
            glColor3f(0.85f,0.85f,0.85f);
            drawText((winWidth - tw)*0.5f, y, s.c_str());
            y -= PUNTS_ROW_STEP;
        }
        if (v.empty()){
            const char* none = "No hay puntajes guardados.";
            glColor3f(0.7f,0.7f,0.7f);
            drawText((winWidth - textWidth(none))*0.5f, startY - PUNTS_FIRST_OFFSET, none);
        }
        glColor3f(0.9f,0.9f,0.9f);
        const char* back = "Esc o Enter: volver";
        drawText((winWidth - textWidth(back,GLUT_BITMAP_HELVETICA_12))*0.5f, 24, back, GLUT_BITMAP_HELVETICA_12);
    }
}

/* ====== Menú de pausa ====== */
static void drawSegBarColored(
    float x, float y, int segments, int active, bool isActive,
    float activeR, float activeG, float activeB,
    float segW = 10.0f, float segH = 14.0f, float gap = 2.0f
){
    active = max(0, min(segments, active));

    for (int i = 0; i < segments; ++i) {
        float sx = x + i * (segW + gap);

        // fondo del segmento
        glColor3f(0.15f, 0.15f, 0.15f);
        glBegin(GL_QUADS);
            glVertex2f(sx,          y);
            glVertex2f(sx + segW,   y);
            glVertex2f(sx + segW,   y + segH);
            glVertex2f(sx,          y + segH);
        glEnd();

        // relleno activo
        if (i < active) {
            if (isActive) glColor3f(activeR, activeG, activeB);
            else          glColor3f(0.85f, 0.85f, 0.85f);
            glBegin(GL_QUADS);
                glVertex2f(sx + 1,         y + 1);
                glVertex2f(sx + segW - 1,  y + 1);
                glVertex2f(sx + segW - 1,  y + segH - 1);
                glVertex2f(sx + 1,         y + segH - 1);
            glEnd();
        }

        // borde
        glColor3f(1, 1, 1);
        glBegin(GL_LINE_LOOP);
            glVertex2f(sx,          y);
            glVertex2f(sx + segW,   y);
            glVertex2f(sx + segW,   y + segH);
            glVertex2f(sx,          y + segH);
        glEnd();
    }
}

static int volToSeg(int vol, int segments=10){
    return (int)round((vol / 128.0f) * segments);
}
static int segToVol(int seg, int segments=10){
    seg = max(0, min(segments, seg));
    return (int)round((seg / (float)segments) * 128.0f);
}
static void drawPanel(float x1, float y1, float x2, float y2) {
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(x1, y1); glVertex2f(x2, y1); glVertex2f(x2, y2); glVertex2f(x1, y2);
    glEnd();
    glColor3f(1,1,1);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x1, y1); glVertex2f(x2, y1); glVertex2f(x2, y2); glVertex2f(x1, y2);
    glEnd();
}
static void dibujarMenuPausa() {
    float x1 = 80, y1 = 180, x2 = winWidth - 80, y2 = winHeight - 180;
    drawPanel(x1,y1,x2,y2);
    glColor3f(1,1,1); drawText(winWidth/2 - 30, y2 - 40, "PAUSA");
    const char* opciones[] = {"1. Reanudar","2. Reiniciar Nivel","3. Opciones","4. Salir al Menu"};
    for (int i=0;i<totalOpciones;++i){
        if (i == opcionSeleccionada) glColor3f(1.0f,1.0f,0.0f); else glColor3f(1,1,1);
        drawText(winWidth/2 - 110, winHeight/2 + 40 - i*25, opciones[i]);
    }
    glColor3f(0.7f,0.7f,0.7f);
    const char* hint = "W/S: mover  |  Enter: seleccionar  |  P: reanudar";
    drawText((winWidth - textWidth(hint,GLUT_BITMAP_HELVETICA_12))*0.5f, y1+14, hint, GLUT_BITMAP_HELVETICA_12);
}
static void dibujarMenuOpciones() {
    float x1 = 80, y1 = 180, x2 = winWidth - 80, y2 = winHeight - 180;
    drawPanel(x1,y1,x2,y2);
    glColor3f(1,1,1); drawText(winWidth/2 - 40, y2 - 40, "OPCIONES");
    const char* labels[] = {"Musica","Efectos","Volver"}; int items=3;
    for (int i=0;i<items;++i){
        bool filaActiva = (i==opcionSeleccionadaOpc);
        if (filaActiva) glColor3f(1.0f,1.0f,0.0f); else glColor3f(1,1,1);
        drawText(winWidth/2 - 160, winHeight/2 + 20 - i*30, labels[i]);
        if (i==0) {
            int segs=10; int act = volToSeg(volumenMusica,segs);
            drawSegBarColored(winWidth/2 - 50, winHeight/2 + 16 - i*30, segs, act, filaActiva, 0.20f,0.85f,0.20f);
        } else if (i==1) {
            int segs=10; int act = volToSeg(volumenEfectos,segs);
            drawSegBarColored(winWidth/2 - 50, winHeight/2 + 16 - i*30, segs, act, filaActiva, 0.20f,0.60f,1.00f);
        }
    }
    glColor3f(0.7f,0.7f,0.7f);
    const char* hint = "W/S: mover  |  A/D: ajustar  |  Enter/Esc: volver";
    drawText((winWidth - textWidth(hint,GLUT_BITMAP_HELVETICA_12))*0.5f, y1+14, hint, GLUT_BITMAP_HELVETICA_12);
}
static void dibujarMenuConfirmReset() {
    float x1 = 80, y1 = 220, x2 = winWidth - 80, y2 = winHeight - 220;
    drawPanel(x1,y1,x2,y2);
    const char* titulo = "Reiniciar nivel?";
    int tw = textWidth(titulo); glColor3f(1,1,1);
    drawText(x1 + ((x2 - x1) - tw) * 0.5f, y2 - 35, titulo);
    float oy = (y1 + y2)/2 - 8;
    if (opcionConfirmReset == 0) glColor3f(1.0f,1.0f,0.0f); else glColor3f(1,1,1);
    drawText(winWidth/2 - 50, oy, "Si");
    if (opcionConfirmReset == 1) glColor3f(1.0f,1.0f,0.0f); else glColor3f(1,1,1);
    drawText(winWidth/2 + 30, oy, "No");
    glColor3f(0.7f,0.7f,0.7f);
    const char* hint = "A/D o W/S: mover  |  Enter: confirmar  |  Esc: cancelar";
    drawText((winWidth - textWidth(hint,GLUT_BITMAP_HELVETICA_12))*0.5f, y1+14, hint, GLUT_BITMAP_HELVETICA_12);
}
static void dibujarMenuConfirmSalir() {
    float x1 = 80, y1 = 220, x2 = winWidth - 80, y2 = winHeight - 220;
    drawPanel(x1,y1,x2,y2);
    const char* titulo = "Salir al menu de inicio?";
    int tw = textWidth(titulo); glColor3f(1,1,1);
    drawText(x1 + ((x2 - x1) - tw) * 0.5f, y2 - 35, titulo);
    float oy = (y1 + y2)/2 - 8;
    if (opcionConfirmSalir == 0) glColor3f(1.0f,1.0f,0.0f); else glColor3f(1,1,1);
    drawText(winWidth/2 - 50, oy, "Si");
    if (opcionConfirmSalir == 1) glColor3f(1.0f,1.0f,0.0f); else glColor3f(1,1,1);
    drawText(winWidth/2 + 30, oy, "No");
    glColor3f(0.7f,0.7f,0.7f);
    const char* hint = "A/D o W/S: mover  |  Enter: confirmar  |  Esc: cancelar";
    drawText((winWidth - textWidth(hint,GLUT_BITMAP_HELVETICA_12))*0.5f, y1+14, hint, GLUT_BITMAP_HELVETICA_12);
}

/* ====== Puntos y niveles ====== */
int  siguienteCambioPuntaje = 60;
const int CAMBIO_PUNTAJE_MAX = 360;

static inline void addPoints(int base) {
    int mult = x2Activo ? 2 : 1;
    int prevScore = puntaje;
    puntaje += base * mult;
    int prevTier = prevScore / 60;
    int newTier  = puntaje   / 60;
    if (newTier > prevTier) {
        int inc = newTier - prevTier;
        if (nivelDificultad + inc > 10) inc = 10 - nivelDificultad;
        if (inc > 0) nivelDificultad += inc;
    }
    while (siguienteCambioPuntaje <= CAMBIO_PUNTAJE_MAX &&
           prevScore <  siguienteCambioPuntaje &&
           puntaje    >= siguienteCambioPuntaje)
    {
        if (sfxCambio) Mix_PlayChannel(-1, sfxCambio, 0);
        siguienteCambioPuntaje += 60;
    }
}

/* ====== Bola de poder ====== */
void dibujarPowerOrb(const PowerUp& p) {
    if (!p.activo) return;
    const int segmentos = 32;

    if (p.tipo == POWER_INVIS) {
        glBegin(GL_TRIANGLE_FAN);
            glColor3f(0.7f, 0.9f, 1.0f);
            glVertex2f(p.cx, p.cy);
            glColor3f(0.1f, 0.4f, 0.95f);
            for (int i = 0; i <= segmentos; ++i) {
                float th = 2.0f * 3.1415926f * i / segmentos;
                glVertex2f(p.cx + cosf(th) * p.radio, p.cy + sinf(th) * p.radio);
            }
        glEnd();
        glColor4f(0.4f, 0.7f, 1.0f, 0.6f);
    } else {
        glBegin(GL_TRIANGLE_FAN);
            glColor3f(1.0f, 0.6f, 0.6f);
            glVertex2f(p.cx, p.cy);
            glColor3f(0.9f, 0.1f, 0.1f);
            for (int i = 0; i <= segmentos; ++i) {
                float th = 2.0f * 3.1415926f * i / segmentos;
                glVertex2f(p.cx + cosf(th) * p.radio, p.cy + sinf(th) * p.radio);
            }
        glEnd();
        glColor4f(1.0f, 0.4f, 0.4f, 0.6f);
    }

    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segmentos; ++i) {
            float th = 2.0f * 3.1415926f * i / segmentos;
            glVertex2f(p.cx + cosf(th) * (p.radio + 2.5f),
                       p.cy + sinf(th) * (p.radio + 2.5f));
        }
    glEnd();
}

/* ====== Crear enemigo ====== */
AutoEnemigo crearAuto(int carril) {
    AutoEnemigo a; a.carril = carril;
    a.width  = 50.0f; a.height = 80.0f;
    a.usaTXT = false; a.txtPath = nullptr;
    a.txtEscala   = 1.8f; a.hitboxScale = 0.90f;
    if (carril == 0) { a.usaTXT = true; a.txtPath = "F1_Rojo.txt";      a.txtEscala = 2.15f; a.hitboxScale = 0.80f; }
    if (carril == 1) { a.usaTXT = true; a.txtPath = "Carro_Blanco.txt"; a.txtEscala = 2.25f; a.hitboxScale = 0.85f; }
    if (carril == 2) { a.usaTXT = true; a.txtPath = "F1_Rojoo.txt";     a.txtEscala = 2.20f; a.hitboxScale = 0.80f; }
    if (carril == 3) { a.usaTXT = true; a.txtPath = "F1_Azul.txt";      a.txtEscala = 2.20f; a.hitboxScale = 0.80f; }

    if (a.usaTXT && a.txtPath) {
        ifstream f(a.txtPath);
        if (f.is_open()) {
            string line; int maxCols = 0, rows = 0;
            while (getline(f, line)) { maxCols = max(maxCols, (int)line.size()); rows++; }
            f.close();
            if (rows > 0 && maxCols > 0) { a.width = maxCols * a.txtEscala; a.height = rows * a.txtEscala; }
            else a.usaTXT = false;
        } else a.usaTXT = false;
    }

    a.x = carriles[carril] + (anchoCarril - a.width) / 2.0f;
    a.velocidad = 2.5f + (rand() % 100) / 50.0f + nivelDificultad * 1.0f;
    a.colorR = (rand() % 256) / 255.0f; a.colorG = (rand() % 256) / 255.0f; a.colorB = (rand() % 256) / 255.0f;

    float nuevoY = winHeight + rand() % 300;
    if (!respetaSeparacionVertical(nuevoY, a.height)) nuevoY += 300;
    a.y = nuevoY;
    a.activo = true;
    return a;
}

/* ====== Carretera ====== */
void dibujarCarretera() {
    glColor3f(0.07f, 0.07f, 0.07f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(winWidth, 0);
        glVertex2f(winWidth, winHeight); glVertex2f(0, winHeight);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    for (int i = 1; i < numCarriles; ++i) {
        float x = carriles[i];
        glBegin(GL_LINES);
            glVertex2f(x, 0); glVertex2f(x, winHeight);
        glEnd();
    }

    glColor3f(0.9f, 0.8f, 0.2f);
    glLineWidth(2.0f);
    for (int i = 0; i < numCarriles; ++i) {
        float centerX = carriles[i] + anchoCarril / 2;
        for (int y = -40; y < winHeight; y += 40) {
            float offsetY = fmod(y + desplazamientoScroll, winHeight);
            glBegin(GL_LINES);
                glVertex2f(centerX, offsetY);
                glVertex2f(centerX, offsetY + 15);
            glEnd();
        }
    }
}

/* ====== Letterbox superior/inferior (HUD) ====== */
void dibujarBarrasLetterbox() {
    glColor4f(0.0f, 0.0f, 0.0f, BARRA_ALPHA);
    glBegin(GL_QUADS);
        glVertex2f(0, winHeight - BARRA_ALTO); glVertex2f(winWidth, winHeight - BARRA_ALTO);
        glVertex2f(winWidth, winHeight); glVertex2f(0, winHeight);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(winWidth, 0); glVertex2f(winWidth, BARRA_ALTO); glVertex2f(0, BARRA_ALTO);
    glEnd();
}

/* ====== Barras de poder HUD ====== */
void dibujarBarrasPoderInferior() {
    if (!x2Activo && !invisActivo) return;
    int barras = (x2Activo ? 1 : 0) + (invisActivo ? 1 : 0);
    float totalH = barras * POWER_BAR_H + (barras - 1) * POWER_BAR_GAP;
    float totalW = POWER_BAR_W + POWER_ICON_GAP + POWER_ICON_W;
    float baseY = (BARRA_ALTO - totalH) * 0.5f;
    float baseX = winWidth - POWER_BAR_MARGIN - totalW;
    int idx = 0;

    auto drawOne = [&](float frac, float r, float g, float b, const char* iconPath, HudIconStyle style) {
        frac = max(0.f, min(1.f, frac));
        float y = baseY + idx * (POWER_BAR_H + POWER_BAR_GAP);

        glColor3f(0.15f, 0.15f, 0.15f);
        glBegin(GL_QUADS);
            glVertex2f(baseX, y); glVertex2f(baseX + POWER_BAR_W, y);
            glVertex2f(baseX + POWER_BAR_W, y + POWER_BAR_H); glVertex2f(baseX, y + POWER_BAR_H);
        glEnd();

        float fillL = baseX + POWER_BAR_W * (1.0f - frac);
        float fillR = baseX + POWER_BAR_W;
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
            glVertex2f(fillL, y); glVertex2f(fillR, y);
            glVertex2f(fillR, y + POWER_BAR_H); glVertex2f(fillL, y + POWER_BAR_H);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f); glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(baseX, y); glVertex2f(baseX + POWER_BAR_W, y);
            glVertex2f(baseX + POWER_BAR_W, y + POWER_BAR_H); glVertex2f(baseX, y + POWER_BAR_H);
        glEnd();

        float ix = baseX + POWER_BAR_W + POWER_ICON_GAP;
        float iy = y + (POWER_BAR_H - POWER_ICON_H) * 0.5f;
        glColor3f(0.10f, 0.10f, 0.10f);
        glBegin(GL_QUADS);
            glVertex2f(ix, iy); glVertex2f(ix + POWER_ICON_W, iy);
            glVertex2f(ix + POWER_ICON_W, iy + POWER_ICON_H); glVertex2f(ix, iy + POWER_ICON_H);
        glEnd();
        glColor3f(1,1,1); glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(ix, iy); glVertex2f(ix + POWER_ICON_W, iy);
            glVertex2f(ix + POWER_ICON_W, iy + POWER_ICON_H); glVertex2f(ix, iy + POWER_ICON_H);
        glEnd();

        dibujarSpriteHUDTXT(iconPath, ix, iy, POWER_ICON_W, POWER_ICON_H, style);
        idx++;
    };

    if (x2Activo) {
        float frac = (float)x2FramesRestantes / (float)X2_DURACION_FRAMES;
        drawOne(frac, 0.9f, 0.2f, 0.2f, HUD_X2_TXT, ICON_TINT_RED);
    }
    if (invisActivo) {
        float frac = (float)invisFramesRestantes / (float)INVIS_DURACION_FRAMES;
        drawOne(frac, 0.3f, 0.7f, 1.0f, HUD_GHOST_TXT, ICON_GHOST_GRAY);
    }
}

/* ====== Reiniciar nivel ====== */
void reiniciarNivel() {
    for (int i = 0; i < numCarriles; i++) enemigos[i].activo = false;
    jugadorX = carriles[1] + (anchoCarril - jugadorWidth) / 2;
    jugadorY = 50;
    gameOver = false;
    desplazamientoScroll = 0.0f;
    puntaje = 0;
    nivelDificultad = 0;
    bonusActivo = false; puedeActivarBonus = true;

    x2Activo = false; x2FramesRestantes = 0;
    invisActivo = false; invisFramesRestantes = 0;

    powerOrb.activo = false; powerOrb.tipo = POWER_NONE;
    framesParaPowerUp = (int)(4.0f * FPS_APROX);
    siguienteCambioPuntaje = 60;

    juegoPausado = false; mostrarMenuPausa = false;
    menuOpcionesActivo = false; menuConfirmResetActivo = false;
    menuConfirmSalirActivo = false;

    congeladoPorChoque = false;
    explosionActiva = false; explosionFrames = 0;
    enemigoChocado = -1;
}

/* ====== START banner ====== */
void comenzarConStartBanner(){
    startBannerActivo = true;
    startBannerFrames = START_BANNER_FRAMES;
    Mix_HaltMusic(); // música del juego arrancará cuando termine el banner
}
static void dibujarStartBanner(){
    float x1 = 160, y1 = 260, x2 = winWidth - 160, y2 = winHeight - 260;
    drawPanel(x1,y1,x2,y2);
    glColor3f(1,1,1);
    const char* t = "START";
    drawText((winWidth - textWidth(t))*0.5f, (y1+y2)/2 - 8, t);
}

/* ====== FADE helpers ====== */
static void drawFadeOverlay(){
    if (fadePhase == FADE_IDLE) return;
    glColor4f(0.0f, 0.0f, 0.0f, fadeAlpha);
    glBegin(GL_QUADS);
        glVertex2f(0,0); glVertex2f(winWidth,0); glVertex2f(winWidth,winHeight); glVertex2f(0,winHeight);
    glEnd();
}
static void startFade(PendingAction act){
    if (fadePhase != FADE_IDLE) return;
    pendingAction = act;
    fadePhase = FADE_OUT;
    fadeFrames = FADE_TOTAL_FRAMES;
    fadeAlpha = 0.0f;

    if (act == ACT_TO_NAME_ENTRY) {
        Mix_HaltMusic();
    }
}
static void doPendingAction(){
    switch (pendingAction) {
        case ACT_TO_PLAY_FROM_MENU: {
            enMenuInicio = false;
            enInstrucciones = enPuntuaciones = false;
            mostrarOpcionesMenu = false;
            animLogoActiva = false;
            reiniciarNivel();
            comenzarConStartBanner();
            break;
        }
        case ACT_TO_NAME_ENTRY: {
            solicitandoNombre = true;
            gameOver = false;
            congeladoPorChoque = false;
            explosionActiva = false;
            break;
        }
        case ACT_RESTART_LEVEL: {
            reiniciarNivel();
            comenzarConStartBanner();
            break;
        }
        case ACT_TO_MAIN_MENU: {
            reiniciarNivel();
            enMenuInicio = true;
            enInstrucciones = enPuntuaciones = false;
            animLogoActiva = true;
            mostrarOpcionesMenu = false;
            if (logoRows > 0 && logoCols > 0) {
                logoY = -logoRows * logoEscala - 8.0f; // reinicia desde abajo
            }
            Mix_HaltMusic();
            if (musicaIntro) Mix_PlayMusic(musicaIntro, -1);
            break;
        }
        default: break;
    }
    pendingAction = ACT_NONE;
}

/* ====== Display ====== */
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // MENÚ INICIO
    if (enMenuInicio) {
        dibujarMenuInicio();
        if (startBannerActivo) dibujarStartBanner();
        drawFadeOverlay();
        glutSwapBuffers();
        return;
    }

    // PAUSA
    if (juegoPausado) {
        dibujarCarretera();
        for (int i = 0; i < numCarriles; i++) {
            if (!enemigos[i].activo) continue;
            if (enemigos[i].usaTXT && enemigos[i].txtPath)
                dibujarSpriteEnemigoTXT(enemigos[i].txtPath, enemigos[i].x, enemigos[i].y, enemigos[i].txtEscala);
            else
                dibujarAuto(enemigos[i].x, enemigos[i].y, enemigos[i].width, enemigos[i].height,
                            enemigos[i].colorR, enemigos[i].colorG, enemigos[i].colorB);
        }
        float alphaJugador = invisActivo ? INVIS_ALPHA : 1.0f;
        dibujarAutoDesdeTxt(jugadorX - 15.5f, jugadorY, JUGADOR_SPRITE_ESCALA, alphaJugador);

        if      (menuConfirmResetActivo) dibujarMenuConfirmReset();
        else if (menuConfirmSalirActivo) dibujarMenuConfirmSalir();
        else if (menuOpcionesActivo)     dibujarMenuOpciones();
        else                             dibujarMenuPausa();

        drawFadeOverlay();
        glutSwapBuffers();
        return;
    }

    // PANEL DE NOMBRE
    if (solicitandoNombre) {
        glColor3f(0,0,0);
        glBegin(GL_QUADS);
            glVertex2f(0,0); glVertex2f(winWidth,0); glVertex2f(winWidth,winHeight); glVertex2f(0,winHeight);
        glEnd();

        float x1 = 60, y1 = 220, x2 = winWidth - 60, y2 = winHeight - 220;
        drawPanel(x1,y1,x2,y2);
        glColor3f(1,1,1);
        const char* t = "Introduce tu nombre:";
        drawText((winWidth - textWidth(t))*0.5f, y2 - 40, t);
        string line = nombreBuffer.empty() ? string("_") : nombreBuffer;
        drawText((winWidth - textWidth(line.c_str()))*0.5f, (y1+y2)/2, line.c_str());
        glColor3f(0.8f,0.8f,0.8f);
        const char* h = "Enter: guardar  |  Backspace: borrar  |  Esc: cancelar";
        drawText((winWidth - textWidth(h,GLUT_BITMAP_HELVETICA_12))*0.5f, y1 + 20, h, GLUT_BITMAP_HELVETICA_12);

        drawFadeOverlay();
        glutSwapBuffers();
        return;
    }

    // GAME OVER con sprite (azul del logo)
    if (gameOver) {
        glColor3f(0,0,0);
        glBegin(GL_QUADS);
            glVertex2f(0,0); glVertex2f(winWidth,0); glVertex2f(winWidth,winHeight); glVertex2f(0,winHeight);
        glEnd();

        // Sprite grande arriba con fade in/out (recoloreado a azul del logo)
        if (!goLines.empty()) {
            float goW = goCols * goEscala;
            float x = (winWidth - goW) * 0.5f + GO_X_OFFSET;
            float y = winHeight - (BARRA_ALTO + 24) - goRows * goEscala;
            y = max(y, winHeight - 24.0f - goRows * goEscala);
            dibujarTxtPreloadedAlphaLogoColor(goLines, x, y, goEscala, goAlpha);
        }

        // Opciones del menú más abajo
        const char* opciones[] = { "Si (Reintentar)", "Salir (Menu Inicio)" };
        for (int i=0;i<2;++i){
            if (i == opcionGameOver) glColor3f(1.0f,1.0f,0.0f); else glColor3f(1,1,1);
            drawText(winWidth/2 - 90, winHeight/2 - 30 - i*30 + GO_OPTIONS_Y_EXTRA, opciones[i]);
        }
        glColor3f(0.7f,0.7f,0.7f);
        const char* hintGO = "W/S: mover  |  Enter: seleccionar";
        drawText((winWidth - textWidth(hintGO,GLUT_BITMAP_HELVETICA_12))*0.5f, 26, hintGO, GLUT_BITMAP_HELVETICA_12);

        drawFadeOverlay();
        glutSwapBuffers();
        return;
    }

    // ===== JUEGO =====
    dibujarCarretera();

    // Jugador (oculto durante explosión y freeze)
    if (!(explosionActiva || congeladoPorChoque)) {
        float alphaJugador = invisActivo ? INVIS_ALPHA : 1.0f;
        dibujarAutoDesdeTxt(jugadorX - 15.5f, jugadorY, JUGADOR_SPRITE_ESCALA, alphaJugador);
        if (invisActivo) {
            float jhb = jugadorHitboxScale;
            float jx = jugadorX + (jugadorWidth  * (1.0f - jhb)) * 0.5f;
            float jy = jugadorY + (jugadorHeight * (1.0f - jhb)) * 0.5f;
            float jw = jugadorWidth  * jhb;
            float jh = jugadorHeight * jhb;
            float cx = jx + jw * 0.5f, cy = jy + jh * 0.5f;
            float rx = jw * 0.65f, ry = jh * 0.65f;
            const int seg = 40;
            glColor4f(0.3f, 0.7f, 1.0f, 0.35f);
            glBegin(GL_LINE_LOOP);
                for (int i = 0; i < seg; ++i) {
                    float th = 2.0f * 3.1415926f * i / seg;
                    glVertex2f(cx + cosf(th) * rx, cy + sinf(th) * ry);
                }
            glEnd();
        }
    }

    // Enemigos (ocultar el impactado durante toda la secuencia)
    for (int i = 0; i < numCarriles; i++) {
        if (!enemigos[i].activo) continue;
        if ((explosionActiva || congeladoPorChoque) && i == enemigoChocado) continue;
        if (enemigos[i].usaTXT && enemigos[i].txtPath) {
            dibujarSpriteEnemigoTXT(enemigos[i].txtPath, enemigos[i].x, enemigos[i].y, enemigos[i].txtEscala);
        } else {
            dibujarAuto(enemigos[i].x, enemigos[i].y, enemigos[i].width, enemigos[i].height,
                        enemigos[i].colorR, enemigos[i].colorG, enemigos[i].colorB);
        }
    }

    // Bola de poder
    dibujarPowerOrb(powerOrb);

    // Letterbox + HUD
    dibujarBarrasLetterbox();
    if (!startBannerActivo) {
        dibujarBarrasPoderInferior();
        mostrarPuntaje(); mostrarHiScore(); mostrarVelocimetro(); mostrarBonusTexto();
    }

    // Explosión
    if (explosionActiva) {
        float t = 1.0f - (explosionFrames / (float)EXPLOSION_TOTAL_FRAMES);
        dibujarExplosion(explosionX, explosionY, t);
    }

    if (startBannerActivo) {
        dibujarStartBanner();
    }

    drawFadeOverlay();
    glutSwapBuffers();
}

/* ====== Update ====== */
void update(int) {
    if (fadePhase == FADE_OUT) {
        float t = 1.0f - (fadeFrames / (float)FADE_TOTAL_FRAMES);
        fadeAlpha = t;
        if (--fadeFrames <= 0) {
            fadeAlpha = 1.0f;
            doPendingAction();
            fadePhase = FADE_IN;
            fadeFrames = FADE_TOTAL_FRAMES;
        }
    } else if (fadePhase == FADE_IN) {
        float t = (fadeFrames / (float)FADE_TOTAL_FRAMES);
        fadeAlpha = t;
        if (--fadeFrames <= 0) {
            fadeAlpha = 0.0f;
            fadePhase = FADE_IDLE;
        }
    }

    // Animación del sprite de Game Over (fade in/out)
    if (gameOver) {
        goAlpha += goDir * 0.025f;
        if (goAlpha >= 1.0f) { goAlpha = 1.0f; goDir = -1; }
        if (goAlpha <= 0.25f){ goAlpha = 0.25f; goDir = +1; }
    }

    if (enMenuInicio || juegoPausado || solicitandoNombre || gameOver || startBannerActivo || congeladoPorChoque) {
        if (enMenuInicio && animLogoActiva && !enInstrucciones && !enPuntuaciones) {
            float logoH = logoRows * logoEscala;
            logoY += LOGO_VEL;
            float targetTop = winHeight * LOGO_ANCLA_FRACCION;
            if ((logoY + logoH) >= targetTop) mostrarOpcionesMenu = true;
            if (logoY >= targetTop - logoH) { logoY = targetTop - logoH; animLogoActiva = false; }
        }
        if (startBannerActivo) {
            if (startBannerFrames > 0) --startBannerFrames;
            if (startBannerFrames == 0) {
                startBannerActivo = false;
                if (musicaFondo) { Mix_PlayMusic(musicaFondo, -1); Mix_VolumeMusic(volumenMusica); }
            }
        }
        if (congeladoPorChoque && explosionActiva) {
            if (explosionFrames > 0) --explosionFrames;
            if (explosionFrames <= 0 && fadePhase == FADE_IDLE) {
                explosionActiva = false;
                startFade(ACT_TO_NAME_ENTRY);
            }
        }
        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }

    // Lógica normal de juego
    if (!gameOver) {
        float velocidad = (puntaje >= 180) ? 6.5f : 5.0f;
        if (teclasPresionadas['a']) jugadorX -= velocidad;
        if (teclasPresionadas['d']) jugadorX += velocidad;
        if (teclasPresionadas['w']) jugadorY += velocidad;
        if (teclasPresionadas['s']) jugadorY -= velocidad;

        if (jugadorX < 30) jugadorX = 30;
        if (jugadorX > winWidth - 80) jugadorX = winWidth - 80;

        const float spriteH = JUGADOR_SPRITE_ALTO * JUGADOR_SPRITE_ESCALA;
        const float minY    = BARRA_ALTO;
        const float maxY    = (winHeight - BARRA_ALTO) - spriteH;
        if (jugadorY < minY) jugadorY = minY;
        if (jugadorY > maxY) jugadorY = maxY;

        float jhb = jugadorHitboxScale;
        float jx = jugadorX + (jugadorWidth  * (1.0f - jhb)) * 0.5f;
        float jy = jugadorY + (jugadorHeight * (1.0f - jhb)) * 0.5f;
        float jw = jugadorWidth  * jhb;
        float jh = jugadorHeight * jhb;

        for (int i = 0; i < numCarriles; i++) {
            if (enemigos[i].activo) {
                enemigos[i].y -= enemigos[i].velocidad;

                float ehb = enemigos[i].hitboxScale;
                float ex = enemigos[i].x + (enemigos[i].width  * (1.0f - ehb)) * 0.5f;
                float ey = enemigos[i].y + (enemigos[i].height * (1.0f - ehb)) * 0.5f;
                float ew = enemigos[i].width  * ehb;
                float eh = enemigos[i].height * ehb;

                if (!invisActivo && fadePhase == FADE_IDLE &&
                    hayColision(jx, jy, jw, jh, ex, ey, ew, eh)) {
                    // Congelar, sonar choque, explosion y ocultar ambos
                    congeladoPorChoque = true;

                    Mix_HaltChannel(-1);
                    if (sfxChoque) Mix_PlayChannel(-1, sfxChoque, 0);
                    Mix_HaltMusic();

                    explosionActiva = true;
                    explosionFrames = EXPLOSION_TOTAL_FRAMES;
                    // Centro aprox. entre ambos autos
                    explosionX = (jx + jw*0.5f + ex + ew*0.5f) * 0.5f;
                    explosionY = (jy + jh*0.5f + ey + eh*0.5f) * 0.5f;

                    enemigoChocado = i;
                    enemigos[i].activo = false; // ocultar enemigo impactado
                    break;
                }

                if (enemigos[i].y + enemigos[i].height < 0) {
                    enemigos[i] = crearAuto(i);
                    addPoints(5);
                }
            } else {
                enemigos[i] = crearAuto(i);
            }
        }

        // Bonus rebase
        bool estaEntreDosAutos = false;
        for (int i = 0; i < numCarriles - 1; i++) {
            AutoEnemigo& a1 = enemigos[i];
            AutoEnemigo& a2 = enemigos[i + 1];
            if (!a1.activo || !a2.activo) continue;

            float centro1 = a1.x + a1.width / 2;
            float centro2 = a2.x + a2.width / 2;
            float centroJugador = jugadorX + jugadorWidth / 2;

            if (abs(a1.y - a2.y) < 40 &&
                centroJugador > centro1 && centroJugador < centro2 &&
                jugadorY + jugadorHeight > std::min(a1.y, a2.y) &&
                jugadorY < std::max(a1.y + a1.height, a2.y + a2.height)) {

                estaEntreDosAutos = true;
                if (puedeActivarBonus) {
                    addPoints(50);
                    bonusActivo = true;
                    puedeActivarBonus = false;
                    tiempoBonusRestante = 60;
                    bonusY = jugadorY + 80;
                }
                break;
            }
        }
        if (!estaEntreDosAutos) puedeActivarBonus = true;

        if (bonusActivo) {
            tiempoBonusRestante--;
            bonusY += 0.5f;
            if (tiempoBonusRestante <= 0) bonusActivo = false;
        }

        // Power-ups
        if (!powerOrb.activo) {
            framesParaPowerUp--;
            if (framesParaPowerUp <= 0) {
                PowerType tipo = POWER_X2;
                if (puntaje >= INVIS_MIN_SCORE) {
                    int prob = (int)(INVIS_SPAWN_PROB * 100.0f);
                    if ((rand() % 100) < prob) tipo = POWER_INVIS;
                }
                int lane = rand() % numCarriles;
                float cx = carriles[lane] + anchoCarril / 2.0f;
                powerOrb.cx = cx; powerOrb.cy = winHeight + 30.0f;
                powerOrb.radio = 12.0f;
                powerOrb.velocidad = 3.5f + nivelDificultad * 0.8f;
                powerOrb.tipo = tipo;
                powerOrb.activo = true;

                int minFrames = (int)(X2_SPAWN_MIN_SEG * FPS_APROX);
                int maxFrames = (int)(X2_SPAWN_MAX_SEG * FPS_APROX);
                if (maxFrames < minFrames) maxFrames = minFrames;
                framesParaPowerUp = minFrames + (rand() % (maxFrames - minFrames + 1));
            }
        } else {
            powerOrb.cy -= powerOrb.velocidad;

            if (powerOrb.cy + powerOrb.radio < 0) {
                powerOrb.activo = false;
            } else {
                if (colisionCirculoRect(powerOrb.cx, powerOrb.cy, powerOrb.radio, jx, jy, jw, jh)) {
                    if (sfxPowerUp) Mix_PlayChannel(-1, sfxPowerUp, 0);
                    if (powerOrb.tipo == POWER_X2) {
                        x2Activo = true; x2FramesRestantes = X2_DURACION_FRAMES;
                    } else if (powerOrb.tipo == POWER_INVIS) {
                        invisActivo = true; invisFramesRestantes = INVIS_DURACION_FRAMES;
                    }
                    powerOrb.activo = false;
                }
            }
        }

        if (x2Activo) { x2FramesRestantes--; if (x2FramesRestantes <= 0) x2Activo = false; }
        if (invisActivo){ invisFramesRestantes--; if (invisFramesRestantes <= 0) invisActivo = false; }

        float velocidadScroll = velocidadScrollBase + nivelDificultad * 1.0f;
        desplazamientoScroll -= velocidadScroll;
        if (desplazamientoScroll < 0.0f) desplazamientoScroll += 40.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

/* ====== Input ====== */
void teclaPresionada(unsigned char key, int x, int y) {
    teclasPresionadas[key] = true;

    if (fadePhase != FADE_IDLE) return;

    // ===== Entrada de nombre =====
    if (solicitandoNombre) {
        if (key == 13 || key == '\r') { // Enter -> guardar y pasar a Game Over
            registrarPuntajeConNombre(nombreBuffer, puntaje);
            solicitandoNombre = false;
            gameOver = true;
            opcionGameOver = 0;
            Mix_HaltMusic();
            if (musicaGameOver) { Mix_PlayMusic(musicaGameOver, 0); Mix_VolumeMusic(volumenMusica); } // una sola vez
            goAlpha = 0.0f; goDir = +1;
            glutPostRedisplay();
            return;
        } else if (key == 27) { // Esc -> cancelar, usa nombre por defecto
            registrarPuntajeConNombre("PLAYER", puntaje);
            solicitandoNombre = false;
            gameOver = true;
            opcionGameOver = 0;
            Mix_HaltMusic();
            if (musicaGameOver) { Mix_PlayMusic(musicaGameOver, 0); Mix_VolumeMusic(volumenMusica); } // una sola vez
            goAlpha = 0.0f; goDir = +1;
            glutPostRedisplay();
            return;
        } else if (key == 8) { // Backspace
            if (!nombreBuffer.empty()) nombreBuffer.pop_back();
            glutPostRedisplay();
            return;
        } else {
            if (nombreBuffer.size() < 16 && (key >= 32 && key <= 126)) {
                nombreBuffer.push_back((char)key);
                glutPostRedisplay();
            }
            return;
        }
    }

    // ===== Menú de inicio y pantallas =====
    if (enMenuInicio) {
        if (!mostrarOpcionesMenu && (key == 13 || key == '\r')) {
            float logoH = logoRows * logoEscala;
            float targetTop = winHeight * LOGO_ANCLA_FRACCION;
            logoY = targetTop - logoH;
            animLogoActiva = false;
            mostrarOpcionesMenu = true;
            if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
            return;
        }

        if (enInstrucciones || enPuntuaciones) {
            if (key == 27 || key == 13 || key == '\r') {
                enInstrucciones = false;
                enPuntuaciones  = false;
                if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
            }
            return;
        }

        if (key == 27) { if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0); exit(0); }

        if (!mostrarOpcionesMenu) return;

        if (key == 'w')      { opcionMenuInicio = (opcionMenuInicio + 4 - 1) % 4; if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0); }
        else if (key == 's') { opcionMenuInicio = (opcionMenuInicio + 1) % 4;     if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0); }
        else if (key == 13 || key == '\r') {
            if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
            if      (opcionMenuInicio == 0) { startFade(ACT_TO_PLAY_FROM_MENU); }
            else if (opcionMenuInicio == 1) { enInstrucciones = true; }
            else if (opcionMenuInicio == 2) { enPuntuaciones = true; }
            else if (opcionMenuInicio == 3) { exit(0); }
        }
        return;
    }

    // P pausa si no hay Game Over
    if (key == 'p' && !gameOver) {
        juegoPausado = !juegoPausado;
        mostrarMenuPausa = juegoPausado;
        if (juegoPausado) { menuOpcionesActivo = false; menuConfirmResetActivo = false; menuConfirmSalirActivo = false; }
        glutPostRedisplay();
        return;
    }

    // ===== Menú de Game Over =====
    if (gameOver) {
        if (key == 'w' || key == 'a')      { opcionGameOver = (opcionGameOver + 2 - 1) % 2; if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0); }
        else if (key == 's' || key == 'd') { opcionGameOver = (opcionGameOver + 1) % 2;     if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0); }
        else if (key == 13 || key == '\r') {
            if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
            if (opcionGameOver == 0) {
                startFade(ACT_RESTART_LEVEL);
            } else {
                startFade(ACT_TO_MAIN_MENU);
            }
        }
        glutPostRedisplay();
        return;
    }

    // ===== Si está pausado, manejar submenús =====
    if (juegoPausado) {
        if (menuConfirmResetActivo) {
            if (key == 'a' || key == 'd' || key == 'w' || key == 's') {
                opcionConfirmReset = 1 - opcionConfirmReset;
                if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0);
            } else if (key == 27) {
                menuConfirmResetActivo = false; if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
            } else if (key == 13 || key == '\r') {
                if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
                if (opcionConfirmReset == 0) { startFade(ACT_RESTART_LEVEL); }
                else menuConfirmResetActivo = false;
            }
            glutPostRedisplay();
            return;
        }
        if (menuConfirmSalirActivo) {
            if (key == 'a' || key == 'd' || key == 'w' || key == 's') {
                opcionConfirmSalir = 1 - opcionConfirmSalir;
                if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0);
            } else if (key == 27) {
                menuConfirmSalirActivo = false; if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
            } else if (key == 13 || key == '\r') {
                if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
                if (opcionConfirmSalir == 0) startFade(ACT_TO_MAIN_MENU);
                else menuConfirmSalirActivo = false;
            }
            glutPostRedisplay();
            return;
        }

        if (menuOpcionesActivo) {
            if (key == 'w')       { opcionSeleccionadaOpc = (opcionSeleccionadaOpc + 3 - 1) % 3; if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0); }
            else if (key == 's')  { opcionSeleccionadaOpc = (opcionSeleccionadaOpc + 1) % 3;     if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0); }
            else if (key == 27)   { menuOpcionesActivo = false; if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0); }
            else if (key == 'a' || key == 'd') {
                int delta = (key == 'a') ? -1 : 1;
                if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0);
                if (opcionSeleccionadaOpc == 0) {
                    int seg = volToSeg(volumenMusica, 10) + delta;
                    seg = max(0, min(10, seg));
                    volumenMusica = segToVol(seg, 10);
                    Mix_VolumeMusic(volumenMusica);
                } else if (opcionSeleccionadaOpc == 1) {
                    int seg = volToSeg(volumenEfectos, 10) + delta;
                    seg = max(0, min(10, seg));
                    volumenEfectos = segToVol(seg, 10);
                    Mix_Volume(-1, volumenEfectos);
                    if (sfxCambio)   Mix_VolumeChunk(sfxCambio, volumenEfectos);
                    if (sfxPowerUp)  Mix_VolumeChunk(sfxPowerUp, volumenEfectos);
                    if (sfxChoque)   Mix_VolumeChunk(sfxChoque, volumenEfectos);
                    if (sfxMenuMove) Mix_VolumeChunk(sfxMenuMove, volumenEfectos);
                    if (sfxMenuSel)  Mix_VolumeChunk(sfxMenuSel,  volumenEfectos);
                }
            } else if (key == 13 || key == '\r') {
                if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
                if (opcionSeleccionadaOpc == 2) menuOpcionesActivo = false;
            }
            glutPostRedisplay();
            return;
        }

        if (key == 'w')       { opcionSeleccionada = (opcionSeleccionada - 1 + totalOpciones) % totalOpciones; if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0); }
        else if (key == 's')  { opcionSeleccionada = (opcionSeleccionada + 1) % totalOpciones;                   if (sfxMenuMove) Mix_PlayChannel(-1, sfxMenuMove, 0); }
        else if (key == 13 || key == '\r') {
            if (sfxMenuSel) Mix_PlayChannel(-1, sfxMenuSel, 0);
            if (opcionSeleccionada == 0) {
                juegoPausado = false;
            } else if (opcionSeleccionada == 1) {
                menuConfirmResetActivo = true; opcionConfirmReset = 0;
            } else if (opcionSeleccionada == 2) {
                menuOpcionesActivo = true; opcionSeleccionadaOpc = 0;
            } else if (opcionSeleccionada == 3) {
                menuConfirmSalirActivo = true; opcionConfirmSalir = 1;
            }
        }
        glutPostRedisplay();
        return;
    }
}

void teclaLiberada(unsigned char key, int x, int y) {
    teclasPresionadas[key] = false;
}

/* ====== Colisiones ====== */
bool hayColision(float x1, float y1, float w1, float h1,
                 float x2, float y2, float w2, float h2) {
    return !(x1 + w1 < x2 || x1 > x2 + w2 ||
             y1 + h1 < y2 || y1 > y2 + h2);
}
bool colisionCirculoRect(float cx, float cy, float r,
                         float rx, float ry, float rw, float rh) {
    float closestX = std::max(rx, std::min(cx, rx + rw));
    float closestY = std::max(ry, std::min(cy, ry + rh));
    float dx = cx - closestX;
    float dy = cy - closestY;
    return (dx*dx + dy*dy) <= r*r;
}
bool respetaSeparacionVertical(float nuevoY, float altura) {
    const float separacionMin = 150.0f;
    for (int i = 0; i < numCarriles; i++) {
        if (enemigos[i].activo) {
            float distY = fabs(enemigos[i].y - nuevoY);
            if (distY < separacionMin) return false;
        }
    }
    return true;
}

/* ====== Mantener relación de aspecto (fullscreen/resize) ====== */
void reshape(int w, int h) {
    float target = (float)winWidth / (float)winHeight;
    float aspect = (float)w / (float)h;

    int vpX = 0, vpY = 0, vpW = w, vpH = h;
    if (aspect > target) {
        vpW = (int)(h * target);
        vpX = (w - vpW) / 2;
    } else if (aspect < target) {
        vpH = (int)(w / target);
        vpY = (h - vpH) / 2;
    }
    glViewport(vpX, vpY, vpW, vpH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* ====== Init ====== */
void iniciar() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    srand((unsigned)time(0));
    inicializarCarriles();

    jugadorX = carriles[1] + (anchoCarril - jugadorWidth) / 2;
    jugadorY = 50;

    for (int i = 0; i < numCarriles; i++) enemigos[i].activo = false;

    if (cargarTxt(LOGO_TXT_PATH, logoLines, logoCols, logoRows)) {
        float maxW = winWidth * 0.88f;
        float maxH = winHeight * 0.36f;
        logoEscala = min(maxW / max(1,logoCols), maxH / max(1,logoRows));
        logoY = -logoRows * logoEscala - 8.0f;
        animLogoActiva = true;
        mostrarOpcionesMenu = false;
    } else {
        logoCols = logoRows = 0;
        animLogoActiva = false;
        mostrarOpcionesMenu = true;
    }

    if (cargarTxt(GO_TXT_PATH, goLines, goCols, goRows)) {
        float maxW = winWidth * 0.90f;
        float maxH = winHeight * 0.40f;
        goEscala = min(maxW / max(1, goCols), maxH / max(1, goRows));
        goAlpha = 0.0f; goDir = +1;
    }

    hiScore = cargarHiScore();
}

/* ====== main ====== */
int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cout << "Error al iniciar SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "Error al iniciar SDL_mixer: " << Mix_GetError() << std::endl;
        return -1;
    }

    musicaIntro = Mix_LoadMUS("Musica_Intro.mp3");
    if (!musicaIntro) std::cout << "No se pudo cargar Musica_Intro.mp3: " << Mix_GetError() << std::endl;
    else              Mix_PlayMusic(musicaIntro, -1);

    musicaFondo = Mix_LoadMUS("F-ZERO.mp3");
    if (!musicaFondo) {
        std::cout << "No se pudo cargar F-ZERO.mp3: " << Mix_GetError() << std::endl;
    }

    musicaGameOver = Mix_LoadMUS("Cancion_Game_Over.mp3");
    if (!musicaGameOver) {
        std::cout << "No se pudo cargar Cancion_Game_Over.mp3: " << Mix_GetError() << std::endl;
    }

    sfxCambio   = Mix_LoadWAV("Sonido_Cambio_Auto.mp3");
    if (!sfxCambio)   std::cout << "No se pudo cargar Sonido_Cambio_Auto.mp3: " << Mix_GetError() << std::endl;
    else              Mix_VolumeChunk(sfxCambio, volumenEfectos);

    sfxPowerUp  = Mix_LoadWAV("Power_Up.mp3");
    if (!sfxPowerUp)  std::cout << "No se pudo cargar Power_Up.mp3: " << Mix_GetError() << std::endl;
    else              Mix_VolumeChunk(sfxPowerUp, volumenEfectos);

    sfxChoque   = Mix_LoadWAV("Choque_de_auto.mp3");
    if (!sfxChoque)   std::cout << "No se pudo cargar Choque_de_auto.mp3: " << Mix_GetError() << std::endl;
    else              Mix_VolumeChunk(sfxChoque, volumenEfectos);

    sfxMenuMove = Mix_LoadWAV("Pasar_entre_opciones.mp3");
    if (!sfxMenuMove) std::cout << "No se pudo cargar Pasar_entre_opciones.mp3: " << Mix_GetError() << std::endl;
    else              Mix_VolumeChunk(sfxMenuMove, volumenEfectos);

    sfxMenuSel  = Mix_LoadWAV("Seleccion_opcion.mp3");
    if (!sfxMenuSel)  std::cout << "No se pudo cargar Seleccion_opcion.mp3: " << Mix_GetError() << std::endl;
    else              Mix_VolumeChunk(sfxMenuSel, volumenEfectos);

    volumenMusica  = Mix_VolumeMusic(-1);
    Mix_Volume(-1, volumenEfectos);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Endless Runner");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    iniciar();
    glutDisplayFunc(display);
    glutKeyboardFunc(teclaPresionada);
    glutKeyboardUpFunc(teclaLiberada);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, update, 0);
    glutMainLoop();

    if (sfxMenuSel)  Mix_FreeChunk(sfxMenuSel);
    if (sfxMenuMove) Mix_FreeChunk(sfxMenuMove);
    if (sfxChoque)   Mix_FreeChunk(sfxChoque);
    if (sfxPowerUp)  Mix_FreeChunk(sfxPowerUp);
    if (sfxCambio)   Mix_FreeChunk(sfxCambio);
    if (musicaGameOver) Mix_FreeMusic(musicaGameOver);
    if (musicaFondo) Mix_FreeMusic(musicaFondo);
    if (musicaIntro) Mix_FreeMusic(musicaIntro);
    Mix_CloseAudio();
    SDL_Quit();
    return 0;
}

