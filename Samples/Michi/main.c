/*
 * Libraries
 * glfw (https://www.glfw.org/)
 * stb_truetype (https://github.com/nothings/stb/blob/master/stb_truetype.h)
*/

// Author: Ashish Bhattarai (Zero5620)

/*
 * Michi
 * 'Michi' (道ーみち) means path. This program let's you control the actor by using commands that you enter in
 * graphics console. It is also capable of displaying various internal information about the values and working
 * of the system. For docomentation on various commands, check: https://github.com/IT-Club-Pulchowk/C-Programming-Guide/blob/main/Samples/Michi/readme.md
*/

/*
* File Navigation
* 
* [Helper Macros]
* [Utility Structs & Functions]
* [Context]
* [Font]
* [Mesh]
* [Rendering]
* [Parser]
* [Michi]
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <float.h>
#include <errno.h>

#include "glfw/include/GLFW/glfw3.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#if defined(_MSC_VER)
#pragma comment(lib, "Opengl32.lib")
#endif

//
// Helper Macros
//

#define MATH_PI 3.1415926535f

#define ARRAY_COUNT(A) (sizeof(A)/sizeof((A)[0]))

#define MINIMUM(A, B)			(((A) < (B)) ? (A) : (B))
#define MAXIMUM(A, B)			(((A) > (B)) ? (A) : (B))
#define CLAMP(A, B, V)      MINIMUM(B, MAXIMUM(A, V))
#define CLAMP01(V)				CLAMP(0.0F, 1.0F, V)
#define TO_RADIANS(DEG)			((DEG) * (MATH_PI / 180.0F))
#define TO_DEGREES(RAD)			((RAD) * (180.0F / MATH_PI))

//
// Utility Structs & Functions
//

typedef enum {
	false, true
} bool;

float lerp(float a, float b, float t) { return (1.0f - t) * a + t * b; }

typedef struct { float x, y; } V2;
typedef struct { float x, y, z; } V3;
typedef struct { float x, y, z, w; } V4;

V2 v2(float x, float y) { return (V2) { x, y }; }
V2 v2add(V2 a, V2 b) { return (V2) { a.x + b.x, a.y + b.y }; }
V2 v2sub(V2 a, V2 b) { return (V2) { a.x - b.x, a.y - b.y }; }
V2 v2mul(V2 a, float b) { return (V2) { a.x *b, a.y *b }; }
float v2dot(V2 a, V2 b) { return a.x * b.x + a.y * b.y; }
bool v2null(V2 a) { return fabsf(v2dot(a, a)) <= FLT_EPSILON; }
V2 v2lerp(V2 a, V2 b, float t) { return v2add(v2mul(a, 1.0f - t), v2mul(b, t)); }

V3 v3(float x, float y, float z) { return (V3) { x, y, z }; }
V3 v3add(V3 a, V3 b) { return (V3) { a.x + b.x, a.y + b.y, a.z + b.z }; }
V3 v3sub(V3 a, V3 b) { return (V3) { a.x - b.x, a.y - b.y, a.z - b.z }; }
V3 v3mul(V3 a, float b) { return (V3) { a.x *b, a.y *b, a.z *b }; }
float v3dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
bool v3null(V3 a) { return fabsf(v3dot(a, a)) <= FLT_EPSILON; }
V3 v3lerp(V3 a, V3 b, float t) { return v3add(v3mul(a, 1.0f - t), v3mul(b, t)); }

V4 v4(float x, float y, float z, float w) { return (V4) { x, y, z, w }; }
V4 v4add(V4 a, V4 b) { return (V4) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
V4 v4sub(V4 a, V4 b) { return (V4) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
V4 v4mul(V4 a, float b) { return (V4) { a.x *b, a.y *b, a.z *b, a.w *b }; }
float v4dot(V4 a, V4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
bool v4null(V4 a) { return fabsf(v4dot(a, a)) <= FLT_EPSILON; }
V4 v4lerp(V4 a, V4 b, float t) { return v4add(v4mul(a, 1.0f - t), v4mul(b, t)); }

bool point_inside_rect(V2 p, V2 ra, V2 rb) { return (p.x > ra.x && p.x < rb.x) && (p.y > ra.y && p.y < rb.y); }

int snprint_vector(char *buffer, int length, char *label, V4 v, uint32_t dim) {
	switch (dim) {
		case 1: return snprintf(buffer, (size_t)length, "%s: %.4f", label, v.x);
		case 2: return snprintf(buffer, (size_t)length, "%s: v2 %.4f %.4f", label, v.x, v.y);
		case 3: return snprintf(buffer, (size_t)length, "%s: v3 %.4f %.4f %.4f", label, v.x, v.y, v.z);
		case 4: return snprintf(buffer, (size_t)length, "%s: v4 %.4f %.4f %.4f %.4f", label, v.x, v.y, v.z, v.w);
	}

	return snprintf(buffer, (size_t)length, "%s: null", label);
}

void michi_fatal_error(const char *message) {
	fprintf(stderr, "Fatal error: %s\n", message);
	exit(0);
}

void *michi_malloc(size_t size) {
	void *ptr = malloc(size);
	if (ptr)  return ptr;
	michi_fatal_error("Out of memory - malloc() failed");
	return NULL;
}

void *michi_realloc(void *old_ptr, size_t new_size) {
	void *ptr = realloc(old_ptr, new_size);
	if (ptr)  return ptr;
	michi_fatal_error("Out of memory - remalloc() failed");
	return NULL;
}

void michi_free(void *ptr) {
	free(ptr);
}

char *read_entire_file(const char *file) {
	FILE *f = fopen(file, "rb");
	if (f == NULL) return NULL;

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *string = michi_malloc(fsize + 1);
	fread(string, 1, fsize, f);
	fclose(f);

	string[fsize] = 0;
	return string;
}

typedef struct {
	size_t length;
	char *data;
} String;

#define STRING(S) (String){ sizeof(S) - 1, S }
#define MAKE_STRING(S) { sizeof(S) - 1, S }

bool string_match(String a, String b) {
	if (a.length != b.length) return false;
	return memcmp(a.data, b.data, a.length) == 0;
}

uint32_t find_least_significant_set_bit(uint32_t value) {
	for (uint32_t test = 0; test < 32; ++test) {
		if (value & (1 << test)) {
			return test;
			break;
		}
	}
	return 0;
}

unsigned char *load_bmp(const char *file, int *w, int *h) {
	FILE *fp = fopen(file, "rb");
	if (!fp) return NULL;

	uint16_t file_type; uint32_t file_size; uint16_t reserved1; uint16_t reserved2; uint32_t bitmap_offset;
	if (fread(&file_type, sizeof(file_type), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&file_size, sizeof(file_size), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&reserved1, sizeof(reserved1), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&reserved2, sizeof(reserved2), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&bitmap_offset, sizeof(bitmap_offset), 1, fp) != 1) { fclose(fp); return NULL; }

	uint32_t size; int32_t width; int32_t height; uint16_t planes; uint16_t bits_per_pixel; uint32_t compression;
	if (fread(&size, sizeof(size), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&width, sizeof(width), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&height, sizeof(height), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&planes, sizeof(planes), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&bits_per_pixel, sizeof(bits_per_pixel), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&compression, sizeof(compression), 1, fp) != 1) { fclose(fp); return NULL; }

	uint32_t bitmap_size; int32_t h_res; int32_t v_res; uint32_t color_used; uint32_t colors_important;
	if (fread(&bitmap_size, sizeof(bitmap_size), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&h_res, sizeof(h_res), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&v_res, sizeof(v_res), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&color_used, sizeof(color_used), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&colors_important, sizeof(colors_important), 1, fp) != 1) { fclose(fp); return NULL; }

	uint32_t red_mask; uint32_t green_mask; uint32_t blue_mask;
	if (fread(&red_mask, sizeof(red_mask), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&green_mask, sizeof(green_mask), 1, fp) != 1) { fclose(fp); return NULL; }
	if (fread(&blue_mask, sizeof(blue_mask), 1, fp) != 1) { fclose(fp); return NULL; }

	uint32_t alpha_mask = ~(red_mask | green_mask | blue_mask);
	if (red_mask == 0 || green_mask == 0 || blue_mask == 0 || alpha_mask == 0) {
		fprintf(stderr, "Failed to load BMP(%s). Color format must be RGBA", file);
		fclose(fp);
		return NULL;
	}

	if (compression != 3) {
		fprintf(stderr, "Failed to load BMP(%s). Compression is not supported", file);
		fclose(fp);
		return NULL;
	}

	if (bits_per_pixel != 32) {
		fprintf(stderr, "Failed to load BMP(%s). Bits per pixel must be 32", file);
		fclose(fp);
		return NULL;
	}

	size_t pixels_size = sizeof(uint32_t) * width * height;
	unsigned char *pixels = michi_malloc(pixels_size);
	if (fread(pixels, pixels_size, 1, fp) != 1) {
		fprintf(stderr, "Invalid BMP file");
		michi_free(pixels);
		fclose(fp);
		return NULL;
	}

	fclose(fp);

	uint32_t red_shift = find_least_significant_set_bit(red_mask);
	uint32_t green_shift = find_least_significant_set_bit(green_mask);
	uint32_t blue_shift = find_least_significant_set_bit(blue_mask);
	uint32_t alpha_shift = find_least_significant_set_bit(alpha_mask);

	uint32_t *dest = (uint32_t *)pixels;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t c = *dest;
			*dest = ((((c >> alpha_shift) & 0xff) << 24) | (((c >> blue_shift) & 0xff) << 16) | 
					 (((c >> green_shift) & 0xff) << 8) | (((c >> red_shift) & 0xff) << 0));
			dest += 1;
		}
	}

	*w = width;
	*h = height;
	return pixels;
}

//
// Context
//

typedef enum {
	CURSOR_KIND_ARROW,
	CURSOR_KIND_IBEAM,

	_CURSOR_KIND_COUNT
} Cursor_Kind;

typedef struct {
	int framebuffer_w, framebuffer_h;
	int window_w, window_h;
	GLFWwindow *window;
	GLFWcursor *cursors[_CURSOR_KIND_COUNT];
} Context;

static Context context;

void glfw_error(int error, const char *description) {
	fprintf(stderr, "Error(%d): %s\n", error, description);
}

bool context_create() {
	if (!glfwInit()) {
		fprintf(stderr, "glfwInit() failed!\n");
		return false;
	}

	glfwSetErrorCallback(glfw_error);

	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	context.window = glfwCreateWindow(800, 600, "Michi", NULL, NULL);
	if (!context.window) {
		glfwTerminate();
		return false;
	}

	GLFWimage icon;
	icon.pixels = load_bmp("Logo.bmp", &icon.width, &icon.height);
	if (icon.pixels) {
		glfwSetWindowIcon(context.window, 1, &icon);
	} else {
		fprintf(stderr, "Failed to load icon (Logo.bmp)");
	}

	glfwMakeContextCurrent(context.window);
	glfwShowWindow(context.window);

	context.cursors[CURSOR_KIND_ARROW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	context.cursors[CURSOR_KIND_IBEAM] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);

	return true;
}

void context_destory() {
	for (int i = 0; i < _CURSOR_KIND_COUNT; ++i)
		glfwDestroyCursor(context.cursors[i]);
	glfwDestroyWindow(context.window);
	glfwTerminate();
}

//
// Font
//

typedef struct {
	GLuint id;
	int width, height;
} Texture;

// ASCII 32..126 is 95 glyphs
#define FONT_PACKED_MIN_CODEPOINT 32
#define FONT_PACKED_MAX_CODEPOINT 126
#define FONT_PACKED_CODEPOINT_COUNT (FONT_PACKED_MAX_CODEPOINT - FONT_PACKED_MIN_CODEPOINT + 1)

typedef struct {
	Texture texture;
	float size;
	stbtt_packedchar cdata[FONT_PACKED_CODEPOINT_COUNT];
} Font;

void stbttEx_GetPackedQuad(const stbtt_packedchar *chardata, int pw, int ph, int char_index, float *xpos, float *ypos, stbtt_aligned_quad *q) {
	float ipw = 1.0f / pw, iph = 1.0f / ph;
	const stbtt_packedchar *b = chardata + char_index;

	float x = (float)STBTT_ifloor((*xpos + b->xoff) + 0.5f);
	float y = (float)STBTT_ifloor((*ypos - b->yoff2) + 0.5f);
	q->x0 = x;
	q->y0 = y;
	q->x1 = x + b->xoff2 - b->xoff;
	q->y1 = y + b->yoff2 - b->yoff;

	q->s0 = b->x0 * ipw;
	q->t0 = b->y0 * iph;
	q->s1 = b->x1 * ipw;
	q->t1 = b->y1 * iph;

	*xpos += b->xadvance;
}

size_t stbttEx_FindCursorOffset(Font *font, V2 pos, float c, const char *text, size_t len) {
	stbtt_aligned_quad q;
	const char *first = text;
	const char *last = text + len;

	while (text != last) {
		if (*text >= 32 && *text < 126) {
			float prev_x = pos.x;
			stbttEx_GetPackedQuad(font->cdata, font->texture.width, font->texture.height, *text - 32, &pos.x, &pos.y, &q);
			if (c >= prev_x && c <= pos.x) {
				if (c - prev_x < pos.x - c) {
					return text - first;
				} else {
					return text - first + 1;
				}
			}
		}
		++text;
	}

	return len;
}

bool font_load(const char *file, float font_size, int bitmap_w, int bitmap_h, Font *font) {
	char *data = read_entire_file(file);
	if (!data) return false;

	stbtt_fontinfo info;

	int offset = stbtt_GetFontOffsetForIndex(data, 0);
	if (!stbtt_InitFont(&info, data, offset)) {
		michi_free(data);
		return false;
	}

	unsigned char *pixels = michi_malloc(bitmap_w * bitmap_h);

	stbtt_pack_context context;

	stbtt_PackBegin(&context, pixels, bitmap_w, bitmap_h, 0, 1, NULL);
	stbtt_PackSetOversampling(&context, 1, 1);
	stbtt_PackFontRange(&context, data, 0, font_size, FONT_PACKED_MIN_CODEPOINT, FONT_PACKED_CODEPOINT_COUNT, font->cdata);
	stbtt_PackEnd(&context);

	font->texture.width = bitmap_w;
	font->texture.height = bitmap_h;
	font->size = font_size;

	glGenTextures(1, &font->texture.id);
	glBindTexture(GL_TEXTURE_2D, font->texture.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_w, bitmap_h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	michi_free(pixels);
	michi_free(data);
	return true;
}

//
// Rendering
//

#define MAX_CIRCLE_SEGMENTS 48

static float im_unit_circle_cos[MAX_CIRCLE_SEGMENTS];
static float im_unit_circle_sin[MAX_CIRCLE_SEGMENTS];

void render_init() {
	for (int i = 0; i < MAX_CIRCLE_SEGMENTS; ++i) {
		float theta = ((float)i / (float)MAX_CIRCLE_SEGMENTS) * MATH_PI * 2;
		im_unit_circle_cos[i] = cosf(theta);
		im_unit_circle_sin[i] = sinf(theta);
	}

	im_unit_circle_cos[MAX_CIRCLE_SEGMENTS - 1] = 1;
	im_unit_circle_sin[MAX_CIRCLE_SEGMENTS - 1] = 0;
}

void render_rect(V2 pos, V2 dim, V4 color) {
	glColor4f(color.x, color.y, color.z, color.w);
	glVertex2f(pos.x, pos.y);
	glVertex2f(pos.x, pos.y + dim.y);
	glVertex2f(pos.x + dim.x, pos.y + dim.y);
	glVertex2f(pos.x + dim.x, pos.y);
}

void render_ellipse(V2 pos, float radius_a, float radius_b, V4 color, float factor) {
	float r = color.x;
	float g = color.y;
	float b = color.z;
	float a = color.w;
	float af = factor * a;

	float cx = pos.x;
	float cy = pos.y;

	int segments = MAX_CIRCLE_SEGMENTS;

	float px = im_unit_circle_cos[0] * radius_a;
	float py = im_unit_circle_sin[0] * radius_b;

	float npx, npy;
	for (int index = 1; index <= segments; ++index) {
		int lookup = (int)(((float)index / (float)segments) * (MAX_CIRCLE_SEGMENTS - 1) + 0.5f);

		npx = im_unit_circle_cos[lookup] * radius_a;
		npy = im_unit_circle_sin[lookup] * radius_b;

		glColor4f(r, g, b,  a); glVertex2f(cx, cy);
		glColor4f(r, g, b, af); glVertex2f(cx + px, cy + py);
		glColor4f(r, g, b, af); glVertex2f(cx + npx, cy + npy);

		px = npx;
		py = npy;
	}
}

float render_font(Font *font, V2 pos, V4 color, const char *text, size_t len) {
	stbtt_aligned_quad q;
	glColor4f(color.x, color.y, color.z, color.w);
	const char *last = text + len;
	while (text != last) {
		if (*text >= 32 && *text < 126) {
			stbttEx_GetPackedQuad(font->cdata, font->texture.width, font->texture.height, *text - 32, &pos.x, &pos.y, &q);
			glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y0);
			glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y0);
			glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y1);
			glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y1);
		}
		++text;
	}
	return pos.x;
}

float render_font_stub(Font *font, V2 pos, const char *text, size_t len) {
	stbtt_aligned_quad q;
	const char *last = text + len;
	while (text != last) {
		if (*text >= 32 && *text < 126) {
			stbttEx_GetPackedQuad(font->cdata, font->texture.width, font->texture.height, *text - 32, &pos.x, &pos.y, &q);
		}
		++text;
	}
	return pos.x;
}

//
// Parser
//

typedef enum {
	TOKEN_KIND_ERROR,
	TOKEN_KIND_EOF,

	TOKEN_KIND_NUMBER_LITERAL,

	TOKEN_KIND_PLUS,
	TOKEN_KIND_MINUS,
	TOKEN_KIND_BRACKET_OPEN,
	TOKEN_KIND_BRACKET_CLOSE,
	TOKEN_KIND_MUL,
	TOKEN_KIND_DIV,
	TOKEN_KIND_PERIOD,
	TOKEN_KIND_COMMA,
	TOKEN_KIND_COLON,

	TOKEN_KIND_IDENTIFIER,

	_TOKEN_KIND_COUNT
} Token_Kind;

typedef struct {
	Token_Kind			kind;
	String				string;
	float				number;
} Token;

typedef struct {
	char *first;
	char *token_start;
	char *current;

	String error;

	Token token;

	char scratch[512];

	bool tokenizing;
} Lexer;

void lexer_init(Lexer *l, char *first) {
	l->first = first;
	l->token_start = first;
	l->current = first;

	l->tokenizing = (*first != 0);
}

void _lexer_consume_character(Lexer *l) {
	if (l->tokenizing) {
		l->current += 1;
		if (*l->current == 0)
			l->tokenizing = false;
	}
}

void _lexer_consume_characters(Lexer *l, size_t count) {
	for (size_t i = 0; i < count; ++i)
		_lexer_consume_character(l);
}

void _lexer_set_token_start(Lexer *l) {
	l->token_start = l->current;
}

void _lexer_make_token(Lexer *l, Token_Kind kind) {
	l->token.kind = kind;
	l->token.string.data = l->token_start;
	l->token.string.length = l->current - l->token_start;
	l->token_start = l->current;
}

void _lexer_make_error_token(Lexer *l, String error) {
	_lexer_make_token(l, TOKEN_KIND_ERROR);
	l->error = error;
}

void lexer_advance_token(Lexer *l) {
	while (l->tokenizing) {
		char a = *l->current;
		char b = *(l->current + 1);

		if (isspace(a)) {
			_lexer_consume_character(l);

			while (l->tokenizing && isspace(*l->current)) {
				_lexer_consume_character(l);
			}

			_lexer_set_token_start(l);
			continue;
		}

		switch (a) {
			case '+': _lexer_consume_character(l); _lexer_make_token(l, TOKEN_KIND_PLUS); return;
			case '-': _lexer_consume_character(l); _lexer_make_token(l, TOKEN_KIND_MINUS); return;
			case '*': _lexer_consume_character(l); _lexer_make_token(l, TOKEN_KIND_MUL); return;
			case '/': _lexer_consume_character(l); _lexer_make_token(l, TOKEN_KIND_DIV); return;
			case '(': _lexer_consume_character(l); _lexer_make_token(l, TOKEN_KIND_BRACKET_OPEN); return;
			case ')': _lexer_consume_character(l); _lexer_make_token(l, TOKEN_KIND_BRACKET_CLOSE); return;
			case ',': _lexer_consume_character(l); _lexer_make_token(l, TOKEN_KIND_COMMA); return;
			case ':': _lexer_consume_character(l); _lexer_make_token(l, TOKEN_KIND_COLON); return;

			case '.': {
				if (isdigit(b)) {
					char *endptr = NULL;
					float value = strtof(l->current, &endptr);
					if (l->current == endptr) {
						_lexer_consume_character(l);
						_lexer_make_token(l, TOKEN_KIND_PERIOD);
					} else {
						_lexer_consume_characters(l, endptr - l->current);
						if (errno == ERANGE) {
							_lexer_make_error_token(l, STRING("Number literal out of range"));
						} else {
							_lexer_make_token(l, TOKEN_KIND_NUMBER_LITERAL);
							l->token.number = value;
						}
					}
				} else {
					_lexer_consume_character(l);
					_lexer_make_token(l, TOKEN_KIND_PERIOD);
				}
				return;
			} break;

			default: {
				if (isdigit(a)) {
					char *endptr = NULL;
					float value = strtof(l->current, &endptr);
					if (l->current != endptr) {
						_lexer_consume_characters(l, endptr - l->current);
						if (errno == ERANGE) {
							_lexer_make_error_token(l, STRING("Number literal out of range"));
						} else {
							_lexer_make_token(l, TOKEN_KIND_NUMBER_LITERAL);
							l->token.number = value;
						}
						return;
					}
				}

				if (!isalnum(a) && a != '_') {
					_lexer_consume_character(l);
					_lexer_make_error_token(l, STRING("Invalid character"));
					return;
				}

				while (l->tokenizing) {
					_lexer_consume_character(l);
					if (!isalnum(*l->current) && a != '_') break;
				}

				_lexer_make_token(l, TOKEN_KIND_IDENTIFIER);
				return;
			} break;
		}
	}

	_lexer_make_token(l, TOKEN_KIND_EOF);
}

typedef enum {
	EXPR_KIND_NONE,
	EXPR_KIND_NUMBER_LITERAL,
	EXPR_KIND_IDENTIFIER,
	EXPR_KIND_UNARY_OPERATOR,
	EXPR_KIND_BINARY_OPERATOR,

	EXPR_KIND_VAR,
	EXPR_KIND_CONST,
	EXPR_KIND_ACTION,
	EXPR_KIND_STATEMENT,

	_EXPR_KIND_COUNT,
} Expr_Kind;

typedef enum {
	OP_KIND_NULL = -1,
	OP_KIND_PLUS = TOKEN_KIND_PLUS,
	OP_KIND_MINUS = TOKEN_KIND_MINUS,
	OP_KIND_DIV = TOKEN_KIND_DIV,
	OP_KIND_MUL = TOKEN_KIND_MUL,
	OP_KIND_PERIOD = TOKEN_KIND_PERIOD,
	OP_KIND_COMMA = TOKEN_KIND_COMMA,
	OP_KIND_COLON = TOKEN_KIND_COLON,
	OP_KIND_BRACKET = TOKEN_KIND_BRACKET_CLOSE,
} Op_Kind;

static const String op_kind_string(Op_Kind op) {
	switch (op) {
		case OP_KIND_PLUS :return STRING(" + ");
		case OP_KIND_MINUS :return STRING(" - ");
		case OP_KIND_DIV :return STRING(" / ");
		case OP_KIND_MUL :return STRING(" * ");
		case OP_KIND_PERIOD :return STRING(" . ");
		case OP_KIND_COMMA :return STRING(" , ");
		case OP_KIND_COLON :return STRING(" : ");
		case OP_KIND_BRACKET :return STRING(" () ");
	}
	return STRING(" null ");
};

typedef enum {
	MICHI_ACTION_MOVE,
	MICHI_ACTION_ROTATE,
	MICHI_ACTION_ENLARGE,
	MICHI_ACTION_CHANGE,
	MICHI_ACTION_FOLLOW,
	MICHI_ACTION_DRAW,
	MICHI_ACTION_DISP,
	MICHI_ACTION_EXIT,

	_MICHI_ACTION_COUNT
} Michi_Action;

static const String michi_action_strings[_MICHI_ACTION_COUNT] = {
	MAKE_STRING("move"), MAKE_STRING("rotate"),
	MAKE_STRING("enlarge"), MAKE_STRING("change"),
	MAKE_STRING("follow"), MAKE_STRING("draw"), 
	MAKE_STRING("disp"), MAKE_STRING("exit")
};

typedef enum {
	MICHI_VAR_OUTPUT,
	MICHI_VAR_ACTOR,
	MICHI_VAR_SPEED,
	MICHI_VAR_POSITION,
	MICHI_VAR_ROTATION,
	MICHI_VAR_SCALE,
	MICHI_VAR_COLOR,
	MICHI_VAR_X,
	MICHI_VAR_Y,
	MICHI_VAR_Z,
	MICHI_VAR_W,

	_MICHI_VAR_COUNT
} Michi_Var;

static const String michi_var_strings[_MICHI_VAR_COUNT] = {
	MAKE_STRING("output"), MAKE_STRING("actor"), MAKE_STRING("speed"),
	MAKE_STRING("position"), MAKE_STRING("rotation"),
	MAKE_STRING("scale"), MAKE_STRING("color"),
	MAKE_STRING("x"), MAKE_STRING("y"), MAKE_STRING("z"), MAKE_STRING("w")
};

typedef enum {
	MICHI_CONST_ON,
	MICHI_CONST_OFF,
	MICHI_CONST_HELP,
	MICHI_CONST_EXPR,

	_MICHI_CONST_COUNT
} Michi_Const;

static const String michi_const_strings[_MICHI_CONST_COUNT] = {
	MAKE_STRING("on"), MAKE_STRING("off"), MAKE_STRING("help"), MAKE_STRING("expr")
};

struct Expr;
struct Expr {
	Expr_Kind kind;

	String string;

	union {
		struct {
			V4 vector;
			uint32_t vector_dim;
		} number;

		struct {
			Op_Kind kind;
			struct Expr *child;
		} unary_op;

		struct {
			Op_Kind kind;
			struct Expr *left;
			struct Expr *right;
		} binary_op;

		struct {
			Michi_Var kind;
			V4 vector;
			uint32_t vector_dim;
			float *ptr;
			float *copy_ptr;
		} var;

		struct {
			Michi_Const kind;
			V4 vector;
			uint32_t vector_dim;
		} constant;

		struct {
			Michi_Action kind;
		} action;

		struct {
			struct Expr *left;
			struct Expr *right;
		} statement;
	};
};
typedef struct Expr Expr;

typedef struct {
	Token *tokens;
	size_t count;
	size_t allocated;
} Token_Array;

size_t _array_get_grow_capacity(size_t c, size_t n) {
	if (c) {
		size_t geom = c + c / 2;
		size_t new_capacity = c + n;
		if (c < new_capacity) return (new_capacity);
		return geom;
	}
	return 8;
}

void token_array_add(Token_Array *a, Token tok) {
	if (a->count == a->allocated) {
		a->allocated = _array_get_grow_capacity(a->allocated, 1);
		a->tokens = michi_realloc(a->tokens, sizeof(Token) * a->allocated);
	}
	a->tokens[a->count] = tok;
	a->count++;
}

void token_array_reset(Token_Array *a) {
	a->count = 0;
}

#define EXPR_PER_BUCKET 1000
typedef struct {
	Expr exprs[EXPR_PER_BUCKET];
	size_t used;
} Expr_Bucket;

typedef struct {
	Expr_Bucket **buckets;
	size_t count;
	size_t allocated;
} Expr_Allocator;

Expr *expr_allocator_push(Expr_Allocator *allocator) {
	Expr_Bucket *buk = NULL;
	if (allocator->count && allocator->buckets[allocator->count - 1]->used != EXPR_PER_BUCKET) {
		buk = allocator->buckets[allocator->count - 1];
	} else if (allocator->count < allocator->allocated) {
		buk = allocator->buckets[allocator->count];
		allocator->count += 1;
	} else {
		allocator->allocated = _array_get_grow_capacity(allocator->allocated, 1);
		allocator->buckets = michi_realloc(allocator->buckets, sizeof(Expr_Bucket *) * allocator->allocated);
		for (size_t index = allocator->count; index < allocator->allocated; ++index) {
			allocator->buckets[index] = michi_malloc(sizeof(Expr_Bucket));
			allocator->buckets[index]->used = 0;
		}
		buk = allocator->buckets[allocator->count];
		allocator->count += 1;
	}

	Expr *expr = &buk->exprs[buk->used];
	buk->used += 1;
	return expr;
}

void expr_allocator_reset(Expr_Allocator *allocator) {
	size_t count = allocator->count;
	for (size_t i = 0; i < count; ++i) {
		allocator->buckets[i]->used = 0;
	}
	allocator->count = 0;
}

typedef struct {
	String content;
	String message;
} Parse_Error;

typedef struct {
	Parse_Error *error;
	size_t count;
	size_t allocated;
} Error_Stream;

void error_stream_add(Error_Stream *stream, String content, String message) {
	if (stream->count == stream->allocated) {
		stream->allocated = _array_get_grow_capacity(stream->allocated, 1);
		stream->error = michi_realloc(stream->error, sizeof(*stream->error) * stream->allocated);
	}
	stream->error[stream->count] = (Parse_Error){ content, message };
	stream->count += 1;
}

void error_stream_reset(Error_Stream *stream) {
	stream->count = 0;
}

typedef struct {
	Token_Array tokens;
	Expr_Allocator allocator;
	Error_Stream error_stream;
	Lexer lexer;
	size_t cursor;
	Expr null_expr;
} Parser;

void parser_create(Parser *parser) {
	memset(parser, 0, sizeof(*parser));
	parser->null_expr.kind = EXPR_KIND_NONE;
}

void parser_consume_token(Parser *parser) {
	if (parser->cursor != parser->tokens.count) {
		parser->cursor += 1;
	}
}

Token parser_peek_token(Parser *parser) {
	if (parser->cursor != parser->tokens.count) {
		return parser->tokens.tokens[parser->cursor];
	}
	return parser->tokens.tokens[parser->tokens.count - 1];
}

Expr *parser_null_expr(Parser *parser) {
	return &parser->null_expr;
}

void parser_report_error(Parser *parser, String content, String message) {
	error_stream_add(&parser->error_stream, content, message);
}

int token_op_precedence(Token_Kind op_kind) {
	switch (op_kind) {
		case TOKEN_KIND_COLON:
			return 10;

		case TOKEN_KIND_COMMA:
			return 15;

		case TOKEN_KIND_PLUS:
		case TOKEN_KIND_MINUS:
			return 80;

		case TOKEN_KIND_MUL:
		case TOKEN_KIND_DIV:
			return 90;

		case TOKEN_KIND_PERIOD:
		case TOKEN_KIND_BRACKET_OPEN:
			return 100;
	}

	return -1;
}

typedef enum {
	ASSOCIATIVITY_LR,
	ASSOCIATIVITY_RL
} Associativity;

Associativity token_op_associativity(Token_Kind op_kind) {
	switch (op_kind) {
		case TOKEN_KIND_COLON:
			return ASSOCIATIVITY_RL;

		case TOKEN_KIND_PLUS:
		case TOKEN_KIND_MINUS:
		case TOKEN_KIND_MUL:
		case TOKEN_KIND_DIV:
		case TOKEN_KIND_COMMA:
		case TOKEN_KIND_PERIOD:
		case TOKEN_KIND_BRACKET_OPEN:
			return ASSOCIATIVITY_LR;
	}

	return ASSOCIATIVITY_RL;
}

Expr *expr_number_literal(Parser *parser, String content, V4 value, uint32_t dim) {
	Expr *expr = expr_allocator_push(&parser->allocator);
	expr->kind = EXPR_KIND_NUMBER_LITERAL;
	expr->string = content;
	expr->number.vector = value;
	expr->number.vector_dim = dim;
	return expr;
}

Expr *expr_unary_operator(Parser *parser, String content, Op_Kind kind, Expr *child) {
	Expr *expr = expr_allocator_push(&parser->allocator);
	expr->kind = EXPR_KIND_UNARY_OPERATOR;
	expr->string = content;
	expr->unary_op.kind = kind;
	expr->unary_op.child = child;
	return expr;
}

Expr *expr_binary_operator(Parser *parser, String content, Op_Kind kind, Expr *left, Expr *right) {
	Expr *expr = expr_allocator_push(&parser->allocator);
	expr->kind = EXPR_KIND_BINARY_OPERATOR;
	expr->string = content;
	expr->binary_op.kind = kind;
	expr->binary_op.left = left;
	expr->binary_op.right = right;
	return expr;
}

Expr *expr_identifier(Parser *parser, String content) {
	Expr *expr = expr_allocator_push(&parser->allocator);
	expr->kind = EXPR_KIND_IDENTIFIER;
	expr->string = content;
	return expr;
}

Expr *expr_var(Parser *parser, String content, Michi_Var kind, V4 value, uint32_t dim, float *ptr, float *copy_ptr) {
	Expr *expr = expr_allocator_push(&parser->allocator);
	expr->kind = EXPR_KIND_VAR;
	expr->string = content;
	expr->var.kind = kind;
	expr->var.vector = value;
	expr->var.vector_dim = dim;
	expr->var.ptr = ptr;
	expr->var.copy_ptr = copy_ptr;
	return expr;
}

Expr *expr_const(Parser *parser, String content, Michi_Const kind, V4 value, uint32_t dim) {
	Expr *expr = expr_allocator_push(&parser->allocator);
	expr->kind = EXPR_KIND_CONST;
	expr->string = content;
	expr->constant.kind = kind;
	expr->constant.vector = value;
	expr->constant.vector_dim = dim;
	return expr;
}

Expr *expr_action(Parser *parser, String content, Michi_Action kind) {
	Expr *expr = expr_allocator_push(&parser->allocator);
	expr->kind = EXPR_KIND_ACTION;
	expr->string = content;
	expr->action.kind = kind;
	return expr;
}

Expr *expr_statement(Parser *parser, String content, Expr *left, Expr *right) {
	Expr *expr = expr_allocator_push(&parser->allocator);
	expr->kind = EXPR_KIND_STATEMENT;
	expr->string = content;
	expr->statement.left = left;
	expr->statement.right = right;
	return expr;
}

bool expr_resolves_to_literal(Expr *expr) {
	return (expr->kind == EXPR_KIND_NUMBER_LITERAL) ||
		(expr->kind == EXPR_KIND_VAR && expr->var.vector_dim != 0) ||
		(expr->kind == EXPR_KIND_CONST && expr->constant.vector_dim != 0);
}

V4 expr_resolve(Expr *expr, uint32_t *dim) {
	switch (expr->kind) {
		case EXPR_KIND_NUMBER_LITERAL: 
			*dim = expr->number.vector_dim;
			return expr->number.vector;
		case EXPR_KIND_VAR:
			*dim = expr->var.vector_dim;
			return expr->var.vector;
		case EXPR_KIND_CONST:
			*dim = expr->constant.vector_dim;
			return expr->constant.vector;
	}

	*dim = 0;
	return v4(0, 0, 0, 0);
}

Expr *parse_expression(Parser *parser, int prec, Token_Kind expect);
Expr *parse_subexpression(Parser *parser);

Expr *parse_subexpression(Parser *parser) {
	Token token = parser_peek_token(parser);

	Expr *node = NULL;

	switch (token.kind) {
		case TOKEN_KIND_PLUS:
		case TOKEN_KIND_MINUS: {
			parser_consume_token(parser);
			Expr *child = parse_subexpression(parser);
			node = expr_unary_operator(parser, token.string, token.kind, child);
		} break;

		case TOKEN_KIND_NUMBER_LITERAL: {
			parser_consume_token(parser);
			node = expr_number_literal(parser, token.string, v4(token.number, 0, 0, 0), 1);
		} break;

		case TOKEN_KIND_IDENTIFIER: {
			parser_consume_token(parser);
			node = expr_identifier(parser, token.string);
		} break;

		case TOKEN_KIND_BRACKET_OPEN: {
			parser_consume_token(parser);
			Expr *child = parse_expression(parser, -1, TOKEN_KIND_BRACKET_CLOSE);
			token = parser_peek_token(parser);
			if (token.kind == TOKEN_KIND_BRACKET_CLOSE) {
				parser_consume_token(parser);
			} else {
				parser_report_error(parser, token.string, STRING("Expected \")\""));
			}
			node = expr_unary_operator(parser, token.string, OP_KIND_BRACKET, child);
		} break;

		case TOKEN_KIND_BRACKET_CLOSE: {
			parser_consume_token(parser);
			parser_report_error(parser, token.string, STRING("Bracket mismatch!"));
			return parser_null_expr(parser);
		} break;
	}

	if (node) return node;

	parser_report_error(parser, token.string, STRING("Expected expression"));
	return parser_null_expr(parser);
}

Expr *parse_binary_operator(Parser *parser, Expr *left) {
	Op_Kind op = OP_KIND_NULL;

	Token token = parser_peek_token(parser);

	switch (token.kind) {
		case TOKEN_KIND_PLUS: op = OP_KIND_PLUS; break;
		case TOKEN_KIND_MINUS: op = OP_KIND_MINUS; break;
		case TOKEN_KIND_MUL: op = OP_KIND_MUL; break;
		case TOKEN_KIND_DIV: op = OP_KIND_DIV; break;
		case TOKEN_KIND_PERIOD: op = OP_KIND_PERIOD; break;
		case TOKEN_KIND_COMMA: op = OP_KIND_COMMA; break;
		case TOKEN_KIND_COLON: op = OP_KIND_COLON; break;

		case TOKEN_KIND_BRACKET_CLOSE: {
			parser_consume_token(parser);
			Expr *right = parser_null_expr(parser);
			parser_report_error(parser, token.string, STRING("Bracket mismatch"));
			return expr_binary_operator(parser, token.string, OP_KIND_NULL, left, right);
		} break;

		default: {
			parser_report_error(parser, token.string, STRING("Expected operator"));
			return expr_binary_operator(parser, token.string, OP_KIND_NULL, left, NULL);
		} break;
	}

	parser_consume_token(parser);
	return expr_binary_operator(parser, token.string, op, left, NULL);
}

Expr *parse_expression(Parser *parser, int prec, Token_Kind expect) {
	Token token = parser_peek_token(parser);
	if (token.kind != TOKEN_KIND_EOF) {
		Expr *a_node = parse_subexpression(parser);
		
		token = parser_peek_token(parser);
		while (token.kind != TOKEN_KIND_EOF) {
			if (token.kind == expect) break;

			int op_prec = token_op_precedence(token.kind);
			if (op_prec < prec) break;
			if (op_prec == prec && token_op_associativity(token.kind) == ASSOCIATIVITY_LR) break;

			Expr *op_node = parse_binary_operator(parser, a_node);

			if (op_node->binary_op.right == NULL) {
				op_node->binary_op.right = parse_expression(parser, op_prec, expect);
			}

			a_node = op_node;

			token = parser_peek_token(parser);
		}

		return a_node;
	}

	parser_report_error(parser, token.string, STRING("Expected expression"));
	return parser_null_expr(parser);
}

Expr *parse(Parser *parser, char *text) {
	lexer_init(&parser->lexer, text);
	lexer_advance_token(&parser->lexer);

	token_array_reset(&parser->tokens);
	expr_allocator_reset(&parser->allocator);
	error_stream_reset(&parser->error_stream);

	Lexer *lexer = &parser->lexer;
	while (lexer->token.kind != TOKEN_KIND_EOF) {
		if (lexer->token.kind == TOKEN_KIND_ERROR) {
			parser_report_error(parser, lexer->token.string, lexer->error);
			return parser_null_expr(parser);
		}
		token_array_add(&parser->tokens, lexer->token);
		lexer_advance_token(lexer);
	}
	token_array_add(&parser->tokens, lexer->token);

	parser->cursor = 0;

	if (parser->tokens.tokens[0].kind == TOKEN_KIND_EOF) {
		return parser_null_expr(parser);
	}

	return parse_expression(parser, -1, TOKEN_KIND_EOF);
}

//
// Michi
//

typedef struct {
	V2 p;
	float ra;
	float rb;
	V4 c;
} Stroke;

typedef struct {
	Stroke *ptr;
	size_t count;
	size_t allocated;
} Stroke_Buffer;

void stroke_buffer_add(Stroke_Buffer *buffer, V2 p, float ra, float rb, V4 c) {
	if (buffer->count == buffer->allocated) {
		buffer->allocated = _array_get_grow_capacity(buffer->allocated, 1);
		buffer->ptr = michi_realloc(buffer->ptr, sizeof(*buffer->ptr) * buffer->allocated);
	}
	Stroke *strk = buffer->ptr + buffer->count;
	strk->p = p;
	strk->ra = ra;
	strk->rb = rb;
	strk->c = c;
	buffer->count += 1;
}

void stroke_buffer_clear(Stroke_Buffer *buffer) {
	buffer->count = 0;
}

typedef enum {
	PANEL_COLOR_BACKGROUND,
	PANEL_COLOR_INPUT_INDICATOR,

	PANEL_COLOR_CURSOR0,
	PANEL_COLOR_CURSOR1,
	PANEL_COLOR_CURSOR_NO_TYPE,

	PANEL_COLOR_TEXT_INPUT_PLACEHOLDER,

	PANEL_COLOR_CODE_GENERAL,
	PANEL_COLOR_CODE_ERROR,
	PANEL_COLOR_COMPILE_ERROR,
	PANEL_COLOR_CODE_NUMBER_LITERAL,
	PANEL_COLOR_CODE_IDENTIFIER,
	PANEL_COLOR_CODE_OPERATORS,

	PANEL_COLOR_INFO,
	
	_PANEL_COLOR_COUNT,
} Panel_Color;

typedef struct {
	Font font;
	float height;
	float indicator_size;
	float cursor_blink_time;
	float cursor_blink_rate;
	double cursor_dposition;
	double cursor_dsize;
	V2 cursor_size[2];
	V2 error_offset;
	V2 info_offset;
	V4 colors[_PANEL_COLOR_COUNT];
} Panel_Style;

#define PANEL_TEXT_INPUT_BUFFER_SIZE 256

typedef struct {
	char buffer[PANEL_TEXT_INPUT_BUFFER_SIZE + 1]; // extra 1 for null-terminator
	size_t count;
	size_t cursor;
} Panel_Text_Input;

typedef enum {
	PANEL_STATE_IDEL,
	PANEL_STATE_TYPING,
} Panel_State;

typedef enum {
	PANEL_DISP_HELP,
	PANEL_DISP_EXPR,
	PANEL_DISP_POSITION,
	PANEL_DISP_ROTATION,
	PANEL_DISP_SCALE,
	PANEL_DISP_COLOR,
	PANEL_DISP_SPEED,
	PANEL_DISP_OUTPUT,

	_PANEL_DISP_COUNT
} Panel_Disp;

struct Michi;
typedef struct {
	Panel_Style style;
	Panel_Text_Input text_input;
	Parser parser;

	Panel_State state;
	float text_position_x_offset;
	float cursor_t;
	float cursor_position;
	float cursor_position_target;
	V2 cursor_size;
	size_t error_cursor_index;
	bool hovering;

	bool disp[_PANEL_DISP_COUNT];
	char scratch[1024];

	struct Michi *michi;
} Panel;

typedef struct {
	float position;
	float rotation;
	float scale;
	float color;
} Actor_Speed;

typedef struct {
	V2 position;
	float rotation;
	V2 scale;
	V4 color;

	float move_distance;
	float rotation_target;
	V2 scale_target;
	V4 color_target;

	Actor_Speed speed;
} Actor;

struct Michi {
	float size;
	V2 position;
	Actor actor;
	bool follow;
	bool draw;
	Panel panel;
	Parser parser;

	Stroke_Buffer strokes;

	V4 output;
	uint32_t output_dim;
};
typedef struct Michi Michi;

Expr *expr_evaluate_expression(Parser *parser, Expr *expr, Michi *michi);
Expr *expr_evaluate_binary_operator(Parser *parser, Expr *expr, Michi *michi);
bool expr_type_check_and_execute(Expr *expr, Parser *parser, Michi *michi);

typedef bool(*Panel_Styler)(Panel_Style *style);

bool panel_default_styler(Panel_Style *style) {
	const char *font_file = "Stanberry.ttf";
	const float font_size = 16;
	const int font_bitmap_w = 512;
	const int font_bitmap_h = 512;

	if (!font_load(font_file, font_size, font_bitmap_w, font_bitmap_h, &style->font)) {
		fprintf(stderr, "Failed to load font: %s\n", font_file);
		return false;
	}

	style->height = font_size + 20;
	style->indicator_size = 20;
	style->cursor_blink_rate = 1.5f;
	style->cursor_blink_time = 1.5f;
	style->cursor_dposition = 0.000000000000000001;
	style->cursor_dsize = 0.00000001;
	style->cursor_size[0] = v2(2.0f, 0.7f * style->height);
	style->cursor_size[1] = v2(0.5f * font_size, 0.7f * style->height);
	style->error_offset = v2(10, 10);
	style->info_offset = v2(10, -10);

	style->colors[PANEL_COLOR_BACKGROUND] = v4(0.04f, 0.04f, 0.04f, 1.0f);
	style->colors[PANEL_COLOR_INPUT_INDICATOR] = v4(0.2f, 0.6f, 0.6f, 1.0f);
	style->colors[PANEL_COLOR_CURSOR0] = v4(0.2f, 0.8f, 0.2f, 1.f);
	style->colors[PANEL_COLOR_CURSOR1] = v4(0.2f, 0.6f, 0.6f, 1.0f);
	style->colors[PANEL_COLOR_CURSOR_NO_TYPE] = v4(4.0f, 0.0f, 0.0f, 1.0f);
	style->colors[PANEL_COLOR_TEXT_INPUT_PLACEHOLDER] = v4(0.5, 0.5f, 0.5f, 1.0f);

	style->colors[PANEL_COLOR_CODE_GENERAL] = v4(.8f, .8f, .9f, 1.f);
	style->colors[PANEL_COLOR_CODE_ERROR] = v4(1.f, 1.f, .3f, 1.f);
	style->colors[PANEL_COLOR_COMPILE_ERROR] = v4(1.f, .3f, .3f, 1.f);
	style->colors[PANEL_COLOR_CODE_NUMBER_LITERAL] = v4(.3f, .3f, .8f, 1.f);
	style->colors[PANEL_COLOR_CODE_IDENTIFIER] = v4(.3f, .9f, .3f, 1.f);
	style->colors[PANEL_COLOR_CODE_OPERATORS] = v4(.8f, .8f, .3f, 1.f);

	style->colors[PANEL_COLOR_INFO] = v4(1, 1, 1, 1);

	return true;
}

String panel_text(Panel *panel) {
	String string;
	string.length = panel->text_input.count;
	string.data = panel->text_input.buffer;
	return string;
}

void panel_input_character(Panel *panel, char c) {
	size_t index = panel->text_input.cursor;
	size_t count = panel->text_input.count;
	if (index <= count && count + 1 <= PANEL_TEXT_INPUT_BUFFER_SIZE) {
		memmove(panel->text_input.buffer + index + 1, panel->text_input.buffer + index, count - index);
		panel->text_input.buffer[index] = c;
		panel->text_input.count += 1;
		panel->text_input.cursor += 1;
	}
	panel->cursor_t = 0;
}

void panel_delete_character(Panel *panel, bool backspace) {
	size_t index = panel->text_input.cursor - (backspace != 0);
	size_t count = panel->text_input.count;
	if (index < count && count) {
		memmove(panel->text_input.buffer + index, panel->text_input.buffer + index + 1, (count - index - 1));
		panel->text_input.count -= 1;
		panel->text_input.cursor -= (backspace != 0);
	}
	panel->cursor_t = 0;
}

void panel_set_cursor(Panel *panel, size_t cursor) {
	if (cursor <= panel->text_input.count) {
		panel->text_input.cursor = cursor;
		panel->cursor_t = 0;
	}
}

V2 panel_text_render_position(Panel *panel) {
	float mid_y = (panel->style.height - panel->style.font.size) * 0.5f;
	return v2(panel->style.indicator_size - panel->text_position_x_offset, mid_y);
}

void panel_set_cursor_at_position(Panel *panel, float xpos) {
	V2 pos = panel_text_render_position(panel);
	String text = panel_text(panel);
	size_t cursor = stbttEx_FindCursorOffset(&panel->style.font, pos, xpos, text.data, text.length);
	panel_set_cursor(panel, cursor);
}

void panel_start_typing(Panel *panel) {
	panel->state = PANEL_STATE_TYPING;
	panel->cursor_t = 0;
	glfwFocusWindow(context.window);
}

void panel_stop_typing(Panel *panel) {
	panel->state = PANEL_STATE_IDEL;
}

void panel_on_cursor_pos_changed(GLFWwindow *window, double x, double y) {
	Panel *panel = glfwGetWindowUserPointer(window);

	y = (double)context.window_h - y;
	V2 cursor = v2((float)x, (float)y);

	V2 panel_rect_min = v2(panel->style.indicator_size, 0);
	V2 panel_rect_max = v2((float)context.framebuffer_w, panel->style.height);

	bool hovered = panel->hovering;
	panel->hovering = point_inside_rect(cursor, panel_rect_min, panel_rect_max);

	if (panel->hovering) {
		if (!hovered)
			glfwSetCursor(window, context.cursors[CURSOR_KIND_IBEAM]);
	} else {
		if (hovered)
			glfwSetCursor(window, context.cursors[CURSOR_KIND_ARROW]);
	}
}

void panel_on_mouse_input(GLFWwindow *window, int button, int action, int mods) {
	Panel *panel = glfwGetWindowUserPointer(window);

	switch (panel->state) {
		case PANEL_STATE_IDEL: {
			if (panel->hovering && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
				panel_start_typing(panel);

				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				panel_set_cursor_at_position(panel, (float)xpos);
			}
		} break;

		case PANEL_STATE_TYPING: {
			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
				if (panel->hovering) {
					double xpos, ypos;
					glfwGetCursorPos(window, &xpos, &ypos);
					panel_set_cursor_at_position(panel, (float)xpos);
				} else {
					panel_stop_typing(panel);
				}
			}
		} break;
	}
}

bool panel_set_cursor_on_error(Panel *panel, Parser *parser) {
	if (parser->error_stream.count) {
		if (panel->error_cursor_index >= parser->error_stream.count)
			panel->error_cursor_index = 0;
		size_t cursor = parser->error_stream.error[panel->error_cursor_index].content.data - panel->text_input.buffer;
		panel->error_cursor_index += 1;
		panel_set_cursor(panel, cursor);
		return true;
	}
	return false;
}

void panel_on_key_input(GLFWwindow *window, int key, int scancode, int action, int mods) {
	Panel *panel = glfwGetWindowUserPointer(window);

	switch (panel->state) {
		case PANEL_STATE_TYPING: {
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
				panel_stop_typing(panel);
			} else if (action == GLFW_PRESS || action == GLFW_REPEAT) {
				switch (key) {
					case GLFW_KEY_BACKSPACE: panel_delete_character(panel, true); break;
					case GLFW_KEY_DELETE: panel_delete_character(panel, false); break;
					case GLFW_KEY_RIGHT: panel_set_cursor(panel, panel->text_input.cursor + 1); break;
					case GLFW_KEY_LEFT: panel_set_cursor(panel, panel->text_input.cursor ? panel->text_input.cursor - 1 : 0); break;
					case GLFW_KEY_HOME: panel_set_cursor(panel, 0); break;
					case GLFW_KEY_END: panel_set_cursor(panel, panel->text_input.count); break;

					case GLFW_KEY_TAB: {
						if (!panel_set_cursor_on_error(panel, &panel->parser)) {
							panel_set_cursor_on_error(panel, &panel->michi->parser);
						}
					} break;

					case GLFW_KEY_ENTER: {
						Parser *parser = &panel->michi->parser;

						panel->text_input.buffer[panel->text_input.count] = 0;
						Expr *expr = parse(parser, panel->text_input.buffer);

						if (!panel_set_cursor_on_error(panel, parser)) {
							expr = expr_evaluate_expression(parser, expr, panel->michi);
							if (!panel_set_cursor_on_error(panel, parser)) {
								if (expr_type_check_and_execute(expr, parser, panel->michi)) {
									panel->text_input.count = 0;
									panel_set_cursor(panel, 0);
								} else {
									panel_set_cursor_on_error(panel, parser);
								}
							}
						}
					} break;
				}
			}
		} break;

		case PANEL_STATE_IDEL: {
			if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
				panel_start_typing(panel);
			}
		} break;
	}
}

void panel_on_text_input(GLFWwindow *window, unsigned int codepoint) {
	Panel *panel = glfwGetWindowUserPointer(window);

	switch (panel->state) {
		case PANEL_STATE_TYPING: {
			if (codepoint >= FONT_PACKED_MIN_CODEPOINT && codepoint < FONT_PACKED_MAX_CODEPOINT)
				panel_input_character(panel, (char)codepoint);
		} break;
	}
}

bool panel_create(Panel_Styler styler, Michi *michi, Panel *panel) {
	if (styler == NULL)
		styler = panel_default_styler;

	if (!styler(&panel->style)) {
		return false;
	}

	parser_create(&panel->parser);

	memset(&panel->text_input, 0, sizeof(panel->text_input));

	panel->state = PANEL_STATE_IDEL;
	panel->text_position_x_offset = false;
	panel->cursor_t = 0;
	panel->cursor_position = panel->style.indicator_size;
	panel->cursor_position_target = panel->style.indicator_size;
	panel->cursor_size = panel->style.cursor_size[1];
	panel->error_cursor_index = 0;
	panel->hovering = false;

	for (int i = 0; i < _PANEL_DISP_COUNT; ++i) {
		panel->disp[i] = false;
	}

	panel->disp[PANEL_DISP_OUTPUT] = true;

	panel->michi = michi;

	return true;
}

void panel_update(Panel *panel, float dt) {
	panel->cursor_t += dt * panel->style.cursor_blink_rate;
	if (panel->cursor_t > panel->style.cursor_blink_time) panel->cursor_t = 0;

	panel->cursor_position = lerp(panel->cursor_position, panel->cursor_position_target, (float)(1.0 - pow(panel->style.cursor_dposition, (double)dt)));

	V2 cursor_target_size = ((panel->text_input.count != panel->text_input.cursor) ? panel->style.cursor_size[0] : panel->style.cursor_size[1]);
	panel->cursor_size = v2lerp(panel->cursor_size, cursor_target_size, (float)(1.0f - pow(panel->style.cursor_dsize, dt)));
}

float panel_render_expr(Expr *expr, Panel *panel, V2 pos, V4 color, Font *font) {
	switch (expr->kind) {
		case EXPR_KIND_NONE: {
			String text = STRING("Expr None");
			render_font(font, pos, color, text.data, text.length);
			return pos.y - 20;
		} break;

		case EXPR_KIND_NUMBER_LITERAL: {
			int len = snprint_vector(panel->scratch, sizeof(panel->scratch), "Expr Number", expr->number.vector, expr->number.vector_dim);
			render_font(font, pos, color, panel->scratch, len);
			return pos.y - 20;
		} break;

		case EXPR_KIND_IDENTIFIER: {
			String text = STRING("Expr Identifier: ");
			float x = render_font(font, pos, color, text.data, text.length);
			render_font(font, v2(x, pos.y), color, expr->string.data, expr->string.length);
			return pos.y - 20;
		} break;

		case EXPR_KIND_UNARY_OPERATOR: {
			String text = STRING("Expr Unary: ");
			float x = render_font(font, pos, color, text.data, text.length);
			String op = op_kind_string(expr->unary_op.kind);
			render_font(font, v2(x, pos.y), color, op.data, op.length);
			pos.y = panel_render_expr(expr->unary_op.child, panel, v2add(pos, v2(20, -20)), color, font);
			return pos.y;
		} break;

		case EXPR_KIND_BINARY_OPERATOR: {
			String text = STRING("Expr Binary: ");
			float x = render_font(font, pos, color, text.data, text.length);
			String op = op_kind_string(expr->binary_op.kind);
			render_font(font, v2(x, pos.y), color, op.data, op.length);
			pos.y = panel_render_expr(expr->binary_op.left, panel, v2add(pos, v2(20, -20)), color, font);
			pos.y = panel_render_expr(expr->binary_op.right, panel, v2add(pos, v2(20, 0)), color, font);
			return pos.y;
		} break;
	}

	return pos.y;
}

V2 panel_render_error(Panel *panel, Parser *parser, V2 pos, V4 color) {
	if (parser->error_stream.count) {
		Font *font = &panel->style.font;

		int len = 0;
		float x_add = 0;

		size_t count = parser->error_stream.count;
		for (size_t index = 0; index < count; ++index) {
			Parse_Error *error = &parser->error_stream.error[index];
			len = snprintf(panel->scratch, sizeof(panel->scratch), "%d:", (int)(error->content.data - panel->text_input.buffer));
			x_add = render_font(font, pos, color, panel->scratch, len);
			render_font(font, v2add(pos, v2(x_add, 0)), color, error->message.data, error->message.length);
			pos.y += font->size;
		}
	}

	return pos;
}

void panel_render(Panel *panel) {
	float off_x = panel->style.indicator_size - 1;

	glLoadIdentity();
	glOrtho(0, context.framebuffer_w, 0, context.framebuffer_h, -1, 1);

	glBegin(GL_QUADS);
	render_rect(v2(0, 0), v2((float)context.framebuffer_w, panel->style.height), panel->style.colors[PANEL_COLOR_BACKGROUND]);
	if (off_x > 0) {
		render_rect(v2(0, 0), v2(off_x - 1, panel->style.height), panel->style.colors[PANEL_COLOR_INPUT_INDICATOR]);
	}
	glEnd();

	String text = panel_text(panel);
	size_t cursor = panel->text_input.cursor;

	float cursor_w = panel->cursor_size.x;
	float cursor_h = panel->cursor_size.y;

	V2 text_pos = panel_text_render_position(panel);
	float cursor_render_x = render_font_stub(&panel->style.font, text_pos, text.data, cursor);

	if (cursor_render_x < panel->style.indicator_size) {
		panel->text_position_x_offset -= (panel->style.indicator_size - cursor_render_x);
		text_pos = panel_text_render_position(panel);
		cursor_render_x = render_font_stub(&panel->style.font, text_pos, text.data, cursor);
	} else if (cursor_render_x > (context.framebuffer_w - cursor_w - panel->style.font.size)) {
		panel->text_position_x_offset += (cursor_render_x - context.framebuffer_w + cursor_w + panel->style.font.size);
		text_pos = panel_text_render_position(panel);
		cursor_render_x = render_font_stub(&panel->style.font, text_pos, text.data, cursor);
	}

	glEnable(GL_SCISSOR_TEST);
	glScissor((GLint)panel->style.indicator_size, 0, context.framebuffer_w, (GLsizei)panel->style.height);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, panel->style.font.texture.id);
	glBegin(GL_QUADS);

	panel->text_input.buffer[panel->text_input.count] = 0;
	Expr *expr = parse(&panel->parser, panel->text_input.buffer);

	if (panel->state == PANEL_STATE_TYPING || text.length != 0) {
		Panel_Style *style = &panel->style;

		V4 text_color;
		char *text_start = panel->text_input.buffer;

		Token_Array *tokens = &panel->parser.tokens;
		size_t token_counts = tokens->count;
		for (size_t index = 0; index < token_counts; ++index) {
			Token *token = tokens->tokens + index;
			switch (token->kind) {
				case TOKEN_KIND_ERROR: text_color = style->colors[PANEL_COLOR_CODE_ERROR]; break;
				case TOKEN_KIND_NUMBER_LITERAL: text_color = style->colors[PANEL_COLOR_CODE_NUMBER_LITERAL]; break;
				case TOKEN_KIND_IDENTIFIER: text_color = style->colors[PANEL_COLOR_CODE_IDENTIFIER]; break;

				case TOKEN_KIND_PLUS:
				case TOKEN_KIND_MINUS:
				case TOKEN_KIND_MUL:
				case TOKEN_KIND_DIV:
				case TOKEN_KIND_PERIOD:
					text_color = style->colors[PANEL_COLOR_CODE_OPERATORS];
					break;

				default: text_color = style->colors[PANEL_COLOR_CODE_GENERAL]; break;
			}

			text_pos.x = render_font(&panel->style.font, text_pos, style->colors[PANEL_COLOR_CODE_GENERAL], text_start, token->string.data - text_start);
			text_pos.x = render_font(&panel->style.font, text_pos, text_color, token->string.data, token->string.length);
			text_start = token->string.data + token->string.length;
		}

		char *text_end = panel->text_input.buffer + panel->text_input.count;
		text_pos.x = render_font(&panel->style.font, text_pos, style->colors[PANEL_COLOR_CODE_GENERAL], text_start, text_end - text_start);
	} else {
		const char msg[] = "Enter Code...";
		render_font(&panel->style.font, v2(panel->style.indicator_size, text_pos.y), panel->style.colors[PANEL_COLOR_TEXT_INPUT_PLACEHOLDER], msg, sizeof(msg) - 1);
	}

	glEnd();
	glDisable(GL_TEXTURE_2D);

	if (panel->state == PANEL_STATE_TYPING) {
		float t = panel->cursor_t;
		if (t > 1) t = 1;
		V4 cursor_color; 
		
		if (text.length != PANEL_TEXT_INPUT_BUFFER_SIZE)
			cursor_color = v4lerp(panel->style.colors[PANEL_COLOR_CURSOR0], panel->style.colors[PANEL_COLOR_CURSOR1], t);
		else
			cursor_color = panel->style.colors[PANEL_COLOR_CURSOR_NO_TYPE];

		float mid_y = (panel->style.height - cursor_h) * 0.5f;
		panel->cursor_position_target = cursor_render_x;

		glBegin(GL_QUADS);
		render_rect(v2(panel->cursor_position, mid_y), v2(cursor_w, cursor_h), cursor_color);
		glEnd();
	}

	glDisable(GL_SCISSOR_TEST);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, panel->style.font.texture.id);
	glBegin(GL_QUADS);

	if (panel->text_input.count) {
		V2 pos = v2add(panel->style.error_offset, v2(0, panel->style.height));
		pos = panel_render_error(panel, &panel->parser, pos, panel->style.colors[PANEL_COLOR_CODE_ERROR]);
		panel_render_error(panel, &panel->michi->parser, pos, panel->style.colors[PANEL_COLOR_COMPILE_ERROR]);
	}

	V2 info_pos = v2(panel->style.info_offset.x, panel->style.info_offset.y + (float)context.framebuffer_h - panel->style.font.size);
	Font *font = &panel->style.font;
	V4 info_color = panel->style.colors[PANEL_COLOR_INFO];

	if (panel->disp[PANEL_DISP_HELP]) {
		// Actions
		{
			String title = STRING("Action: ");
			String comma = STRING(", ");
			V2 p = info_pos;

			p.x = render_font(font, p, info_color, title.data, title.length);

			for (int i = 0; i < _MICHI_ACTION_COUNT; ++i) {
				String string = michi_action_strings[i];
				p.x = render_font(font, p, info_color, string.data, string.length);
				if (i != _MICHI_ACTION_COUNT - 1)
					p.x = render_font(font, p, info_color, comma.data, comma.length);
			}
			info_pos.y -= font->size;
		}

		// Variables
		{
			String title = STRING("Variables: ");
			String comma = STRING(", ");
			V2 p = info_pos;

			p.x = render_font(font, p, info_color, title.data, title.length);

			for (int i = 0; i < _MICHI_VAR_COUNT; ++i) {
				String string = michi_var_strings[i];
				p.x = render_font(font, p, info_color, string.data, string.length);
				if (i != _MICHI_VAR_COUNT - 1)
					p.x = render_font(font, p, info_color, comma.data, comma.length);
			}
			info_pos.y -= font->size;
		}

		// Constants
		{
			String title = STRING("Constants: ");
			String comma = STRING(", ");
			V2 p = info_pos;

			p.x = render_font(font, p, info_color, title.data, title.length);

			for (int i = 0; i < _MICHI_CONST_COUNT; ++i) {
				String string = michi_const_strings[i];
				p.x = render_font(font, p, info_color, string.data, string.length);
				if (i != _MICHI_CONST_COUNT - 1)
					p.x = render_font(font, p, info_color, comma.data, comma.length);
			}
			info_pos.y -= font->size;
		}
	}

	Michi *michi = panel->michi;
	if (panel->disp[PANEL_DISP_POSITION]) {
		int len = snprintf(panel->scratch, sizeof(panel->scratch), "Position: %.4f, %.4f", 
						   michi->actor.position.x, michi->actor.position.y);
		render_font(font, info_pos, info_color, panel->scratch, len);
		info_pos.y -= font->size;
	}

	if (panel->disp[PANEL_DISP_ROTATION]) {
		int len = snprintf(panel->scratch, sizeof(panel->scratch), "Rotation: %.4f degs",
						   michi->actor.rotation);
		render_font(font, info_pos, info_color, panel->scratch, len);
		info_pos.y -= font->size;
	}

	if (panel->disp[PANEL_DISP_SCALE]) {
		int len = snprintf(panel->scratch, sizeof(panel->scratch), "Scale: %.4f, %.4f",
						   michi->actor.scale.x, michi->actor.scale.y);
		render_font(font, info_pos, info_color, panel->scratch, len);
		info_pos.y -= font->size;
	}

	if (panel->disp[PANEL_DISP_COLOR]) {
		int len = snprintf(panel->scratch, sizeof(panel->scratch), "Color: %.4f, %.4f, %.4f, %.4f",
						   michi->actor.color.x, michi->actor.color.y, michi->actor.color.z, michi->actor.color.w);
		render_font(font, info_pos, info_color, panel->scratch, len);
		info_pos.y -= font->size;
	}

	if (panel->disp[PANEL_DISP_SPEED]) {
		int len = snprintf(panel->scratch, sizeof(panel->scratch), "Speed: Position(%.4f), Rotation(%.4f), Scale(%.4f), Color(%.4f)",
						   michi->actor.speed.position, michi->actor.speed.rotation, 
						   michi->actor.speed.scale, michi->actor.speed.color);
		render_font(font, info_pos, info_color, panel->scratch, len);
		info_pos.y -= font->size;
	}

	if (panel->disp[PANEL_DISP_OUTPUT]) {
		int len = snprint_vector(panel->scratch, sizeof(panel->scratch), "Output", michi->output, michi->output_dim);
		render_font(font, info_pos, info_color, panel->scratch, len);
		info_pos.y -= font->size;
		len = snprintf(panel->scratch, sizeof(panel->scratch), "Stroke Count: %zu", panel->michi->strokes.count);
		render_font(font, info_pos, info_color, panel->scratch, len);
		info_pos.y -= font->size;
		len = snprintf(panel->scratch, sizeof(panel->scratch), "Follow: %s, Draw: %s", 
					   panel->michi->follow ? "on" : "off", panel->michi->draw ? "on" : "off");
		render_font(font, info_pos, info_color, panel->scratch, len);
		info_pos.y -= font->size;
	}

	if (panel->disp[PANEL_DISP_EXPR]) {
		String title = STRING("Expr: ");
		render_font(font, info_pos, info_color, title.data, title.length);
		info_pos.y -= font->size;
		panel_render_expr(expr, panel, v2add(info_pos, v2(font->size, 0)), info_color, font);
	}

	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void actor_render(Actor *actor) {
	glPushMatrix();
	glTranslatef(actor->position.x, actor->position.y, 0);
	glRotatef(TO_DEGREES(actor->rotation), 0, 0, -1);
	glScalef(actor->scale.x, actor->scale.y, 1);

	glPushMatrix();
	glScalef(1.2f, 1.2f, 1);
	glColor4f(1.0f - actor->color.x, 1.0f - actor->color.y, 1.0f - actor->color.z, actor->color.w);
	glBegin(GL_TRIANGLES);
	glVertex3f(-1, -1, 0);
	glVertex3f(0, 1, 0);
	glVertex3f(1, -1, 0);
	glEnd();
	glPopMatrix();

	glColor4f(actor->color.x, actor->color.y, actor->color.z, actor->color.w);
	glBegin(GL_TRIANGLES);
	glVertex3f(-1, -1, 0);
	glVertex3f(0, 1, 0);
	glVertex3f(1, -1, 0);
	glEnd();

	glPopMatrix();
}

bool michi_create(float size, Panel_Styler styler, Michi *michi) {
	if (!panel_create(styler, michi, &michi->panel)) {
		fprintf(stderr, "Panel failed to create!\n");
		return false;
	}

	parser_create(&michi->parser);

	michi->position = v2(0, 0);
	michi->size = size;

	michi->actor.position = v2(0, 0);
	michi->actor.rotation = 0;
	michi->actor.scale = v2(4, 4);
	michi->actor.color = v4(0, 1, 1, 1);

	michi->actor.move_distance = 0;
	michi->actor.rotation_target = michi->actor.rotation;
	michi->actor.scale_target = michi->actor.scale;
	michi->actor.color_target = michi->actor.color;

	michi->actor.speed.position = 0.25f;
	michi->actor.speed.rotation = 0.25f;
	michi->actor.speed.scale = 0.25;
	michi->actor.speed.color = 0.25;

	michi->strokes.count = michi->strokes.allocated = 0;
	michi->strokes.ptr = NULL;

	michi->output = v4(0, 0, 0, 0);
	michi->output_dim = 4;

	michi->follow = false;
	michi->draw = true;

	return true;
}

Expr *expr_evaluate_binary_operator(Parser *parser, Expr *expr, Michi *michi) {
	if (expr->kind != EXPR_KIND_BINARY_OPERATOR || expr->binary_op.kind == OP_KIND_NULL) {
		parser_report_error(parser, expr->string, STRING("Expected binary operator"));
		return parser_null_expr(parser);
	}

	Expr *left = expr_evaluate_expression(parser, expr->binary_op.left, michi);
	Expr *right = expr_evaluate_expression(parser, expr->binary_op.right, michi);

	if (left->kind == EXPR_KIND_NONE || right->kind == EXPR_KIND_NONE)
		return parser_null_expr(parser);

	Expr *result = parser_null_expr(parser);

	switch (expr->binary_op.kind) {
		case OP_KIND_PLUS: {
			if (!expr_resolves_to_literal(left)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}
			if (!expr_resolves_to_literal(right)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}

			uint32_t ld, rd;
			V4 lv = expr_resolve(left, &ld);
			V4 rv = expr_resolve(right, &rd);

			if (ld == rd) {
				result = expr_number_literal(parser, expr->string, v4add(lv, rv), ld);
			} else {
				parser_report_error(parser, left->string, STRING("Addition can not be performed on vectors with different dimension"));
			}
		} break;

		case OP_KIND_MINUS: {
			if (!expr_resolves_to_literal(left)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}
			if (!expr_resolves_to_literal(right)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}

			uint32_t ld, rd;
			V4 lv = expr_resolve(left, &ld);
			V4 rv = expr_resolve(right, &rd);

			if (ld == rd) {
				result = expr_number_literal(parser, expr->string, v4sub(lv, rv), ld);
			} else {
				parser_report_error(parser, left->string, STRING("Subtraction can not be performed on vectors with different dimension"));
			}
		} break;

		case OP_KIND_DIV: {
			if (!expr_resolves_to_literal(left)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}
			if (!expr_resolves_to_literal(right)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}

			uint32_t ld, rd;
			V4 lv = expr_resolve(left, &ld);
			V4 rv = expr_resolve(right, &rd);

			if (rd == 1) {
				result = expr_number_literal(parser, expr->string, v4mul(lv, 1.0f / rv.x), ld);
			} else {
				parser_report_error(parser, left->string, STRING("Division can not be performed by vector"));
			}
		} break;

		case OP_KIND_MUL: {
			if (!expr_resolves_to_literal(left)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}
			if (!expr_resolves_to_literal(right)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}

			uint32_t ld, rd;
			V4 lv = expr_resolve(left, &ld);
			V4 rv = expr_resolve(right, &rd);

			if (rd == ld) {
				result = expr_number_literal(parser, expr->string, v4(v4dot(lv, rv), 0, 0, 0), ld);
			} else if (rd == 1) {
				result = expr_number_literal(parser, expr->string, v4mul(lv, rv.x), ld);
			} else if (ld == 1) {
				result = expr_number_literal(parser, expr->string, v4mul(rv, lv.x), rd);
			} else {
				parser_report_error(parser, left->string, STRING("Invalid vectors for multiplication"));
			}
		} break;

		case OP_KIND_PERIOD: {
			if (left->kind != EXPR_KIND_VAR) {
				parser_report_error(parser, left->string, STRING("Expected variable"));
				break;
			}
			if (right->kind != EXPR_KIND_VAR) {
				parser_report_error(parser, right->string, STRING("Expected variable"));
				break;
			}

			switch (left->var.kind) {
				case MICHI_VAR_OUTPUT: {
					V4 out = michi->output;
					float *ptr = (float *)&michi->output;
					switch (right->var.kind) {
						case MICHI_VAR_X: 
							result = expr_var(parser, expr->string, MICHI_VAR_X, v4(out.x, 0, 0, 0), 1, ptr + 0, NULL);
							break;
						case MICHI_VAR_Y:
							result = expr_var(parser, expr->string, MICHI_VAR_Y, v4(out.y, 0, 0, 0), 1, ptr + 1, NULL);
							break;
						case MICHI_VAR_Z:
							result = expr_var(parser, expr->string, MICHI_VAR_Z, v4(out.z, 0, 0, 0), 1, ptr + 2, NULL);
							break;
						case MICHI_VAR_W:
							result = expr_var(parser, expr->string, MICHI_VAR_W, v4(out.w, 0, 0, 0), 1, ptr + 3, NULL);
							break;
						default:
							parser_report_error(parser, expr->string, STRING("Invalid member access"));
							break;
					}
				} break;

				case MICHI_VAR_ACTOR: {
					switch (right->var.kind) {
						case MICHI_VAR_POSITION: {
							V2 out = michi->actor.position;
							float *ptr = (float *)&michi->actor.position;
							result = expr_var(parser, expr->string, MICHI_VAR_POSITION, v4(out.x, out.y, 0, 0), 2, ptr, NULL);
						} break;
						case MICHI_VAR_ROTATION: {
							float out = michi->actor.rotation;
							float *ptr = &michi->actor.rotation;
							float *copy_ptr = &michi->actor.rotation_target;
							result = expr_var(parser, expr->string, MICHI_VAR_ROTATION, v4(out, 0, 0, 0), 1, ptr, copy_ptr);
						} break;
						case MICHI_VAR_SCALE: {
							V2 out = michi->actor.scale;
							float *ptr = (float *)&michi->actor.scale;
							float *copy_ptr = (float *)&michi->actor.scale_target;
							result = expr_var(parser, expr->string, MICHI_VAR_SCALE, v4(out.x, out.y, 0, 0), 2, ptr, copy_ptr);
						} break;
						case MICHI_VAR_COLOR: {
							V4 out = michi->actor.color;
							float *ptr = (float *)&michi->actor.color;
							float *copy_ptr = (float *)&michi->actor.color_target;
							result = expr_var(parser, expr->string, MICHI_VAR_COLOR, v4(out.x, out.y, out.z, out.w), 4, ptr, copy_ptr);
						} break;
					}
				} break;

				case MICHI_VAR_SPEED: {
					switch (right->var.kind) {
						case MICHI_VAR_POSITION: {
							float out = michi->actor.speed.position;
							float *ptr = &michi->actor.speed.position;
							result = expr_var(parser, expr->string, MICHI_VAR_X, v4(out, 0, 0, 0), 1, ptr, NULL);
						} break;
						case MICHI_VAR_ROTATION: {
							float out = michi->actor.speed.rotation;
							float *ptr = &michi->actor.speed.rotation;
							result = expr_var(parser, expr->string, MICHI_VAR_X, v4(out, 0, 0, 0), 1, ptr, NULL);
						} break;
						case MICHI_VAR_SCALE: {
							float out = michi->actor.speed.scale;
							float *ptr = &michi->actor.speed.scale;
							result = expr_var(parser, expr->string, MICHI_VAR_X, v4(out, 0, 0, 0), 1, ptr, NULL);
						} break;
						case MICHI_VAR_COLOR: {
							float out = michi->actor.speed.color;
							float *ptr = &michi->actor.speed.color;
							result = expr_var(parser, expr->string, MICHI_VAR_X, v4(out, 0, 0, 0), 1, ptr, NULL);
						} break;
						default:
							parser_report_error(parser, expr->string, STRING("Invalid member access"));
							break;
					}
				} break;

				case MICHI_VAR_POSITION:
				case MICHI_VAR_SCALE:
				case MICHI_VAR_COLOR: {
					float *ptr = left->var.ptr;
					float *copy_ptr = left->var.copy_ptr;
					if (ptr == NULL) {
						parser_report_error(parser, expr->string, STRING("Invalid identifier"));
						break;
					}

					V4 out = left->var.vector;
					uint32_t outd = left->var.vector_dim;

					switch (right->var.kind) {
						case MICHI_VAR_X:
							result = expr_var(parser, expr->string, MICHI_VAR_X, v4(out.x, 0, 0, 0), 1, ptr + 0, copy_ptr ? copy_ptr + 0 : NULL);
							break;
						case MICHI_VAR_Y:
							if (outd >= 2)
								result = expr_var(parser, expr->string, MICHI_VAR_Y, v4(out.y, 0, 0, 0), 1, ptr + 1, copy_ptr ? copy_ptr + 1 : NULL);
							else
								parser_report_error(parser, expr->string, STRING("Invalid member access"));
							break;
						case MICHI_VAR_Z:
							if (outd >= 3)
								result = expr_var(parser, expr->string, MICHI_VAR_Z, v4(out.z, 0, 0, 0), 1, ptr + 2, copy_ptr ? copy_ptr + 2 : NULL);
							else
								parser_report_error(parser, expr->string, STRING("Invalid member access"));
							break;
						case MICHI_VAR_W:
							if (outd == 4)
								result = expr_var(parser, expr->string, MICHI_VAR_W, v4(out.w, 0, 0, 0), 1, ptr + 3, copy_ptr ? copy_ptr + 3 : NULL);
							else
								parser_report_error(parser, expr->string, STRING("Invalid member access"));
							break;
						default:
							parser_report_error(parser, expr->string, STRING("Invalid member access"));
							break;
					}
				} break;

				case MICHI_VAR_ROTATION:
				case MICHI_VAR_X:
				case MICHI_VAR_Y:
				case MICHI_VAR_Z:
				case MICHI_VAR_W: {
					parser_report_error(parser, expr->string, STRING("Invalid member access"));
				} break;
			}
		} break;

		case OP_KIND_COMMA: {
			if (!expr_resolves_to_literal(left)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}
			if (!expr_resolves_to_literal(right)) {
				parser_report_error(parser, left->string, STRING("Expected variable or literal"));
				break;
			}

			uint32_t ld, rd;
			V4 lv = expr_resolve(left, &ld);
			V4 rv = expr_resolve(right, &rd);

			if (ld + rd <= 4) {
				float vec[4] = { 0,0,0,0 };
				for (uint32_t i = 0; i < ld; ++i) vec[i] = *((float *)&lv + i);
				for (uint32_t i = 0; i < rd; ++i) vec[i + ld] = *((float *)&rv + i);
				result = expr_number_literal(parser, expr->string, v4(vec[0], vec[1], vec[2], vec[3]), ld + rd);
			} else {
				parser_report_error(parser, expr->string, STRING("Vectors with dimension greater than 4 is not supported"));
			}
		} break;

		case OP_KIND_COLON: {
			if (left->kind != EXPR_KIND_ACTION && left->kind != EXPR_KIND_VAR) {
				parser_report_error(parser, left->string, STRING("Expected action or variable"));
				break;
			}
			if (!expr_resolves_to_literal(right) && right->kind != EXPR_KIND_VAR && right->kind != EXPR_KIND_CONST) {
				parser_report_error(parser, right->string, STRING("Expected action or variable or constant"));
				break;
			}

			result = expr_statement(parser, expr->string, left, right);
		} break;
	}

	return result;
}

Expr *expr_evaluate_expression(Parser *parser, Expr *expr, Michi *michi) {
	switch (expr->kind) {
		case EXPR_KIND_VAR:
		case EXPR_KIND_CONST:
		case EXPR_KIND_ACTION:
		case EXPR_KIND_STATEMENT:
		case EXPR_KIND_NUMBER_LITERAL: 
			return expr;

		case EXPR_KIND_UNARY_OPERATOR: {
			Expr *child = expr_evaluate_expression(parser, expr->unary_op.child, michi);

			if (expr_resolves_to_literal(child)) {
				uint32_t dim;
				V4 out = expr_resolve(child, &dim);
				switch (expr->unary_op.kind) {
					case OP_KIND_MINUS:
						return expr_number_literal(parser, expr->string, v4(-out.x, -out.y, -out.z, -out.w), dim);
					case OP_KIND_PLUS:
					case OP_KIND_BRACKET:
						return expr_number_literal(parser, expr->string, out, dim);
				}
				return parser_null_expr(parser);
			} else if (child->kind == EXPR_KIND_NONE) {
				return parser_null_expr(parser);
			} else {
				parser_report_error(parser, child->string, STRING("Expected expression"));
				return parser_null_expr(parser);
			}
		} break;

		case EXPR_KIND_BINARY_OPERATOR:
			return expr_evaluate_binary_operator(parser, expr, michi);

		case EXPR_KIND_IDENTIFIER: {
			// Action
			for (int i = 0; i < _MICHI_ACTION_COUNT; ++i) {
				if (string_match(expr->string, michi_action_strings[i])) {
					return expr_action(parser, expr->string, (Michi_Action)i);
				}
			}

			// Variable
			for (int i = 0; i < _MICHI_VAR_COUNT; ++i) {
				if (string_match(expr->string, michi_var_strings[i])) {
					uint32_t dim = 0;
					if (i == MICHI_VAR_OUTPUT)dim = michi->output_dim;
					return expr_var(parser, expr->string, (Michi_Var)i, v4(0, 0, 0, 0), dim, NULL, NULL);
				}
			}

			// Constants
			for (int i = 0; i < _MICHI_CONST_COUNT; ++i) {
				if (string_match(expr->string, michi_const_strings[i])) {
					return expr_const(parser, expr->string, (Michi_Const)i, v4(0, 0, 0, 0), 0);
				}
			}

			parser_report_error(parser, expr->string, STRING("Invalid identifier"));
			return parser_null_expr(parser);
		} break;
	}

	parser_report_error(parser, expr->string, STRING("Expected expression"));
	return parser_null_expr(parser);
}

bool expr_type_check_and_execute(Expr *expr, Parser *parser, Michi *michi) {
	switch (expr->kind) {
		case EXPR_KIND_NONE: {
			return false;
		} break;

		case EXPR_KIND_NUMBER_LITERAL: {
			michi->output = expr->number.vector;
			michi->output_dim = expr->number.vector_dim;
			return true;
		} break;

		case EXPR_KIND_VAR: {
			if (expr->var.vector_dim == 0) {
				parser_report_error(parser, expr->string, STRING("Invalid variable"));
				return false;
			}

			michi->output = expr->var.vector;
			michi->output_dim = expr->var.vector_dim;
			return true;
		} break;

		case EXPR_KIND_ACTION: {
			switch (expr->action.kind) {
				case MICHI_ACTION_EXIT:
					glfwSetWindowShouldClose(context.window, 1);
					return true;

				case MICHI_ACTION_MOVE:
				case MICHI_ACTION_ROTATE:
					parser_report_error(parser, expr->string, STRING("Expected vector1 argument"));
					return false;
				case MICHI_ACTION_ENLARGE:
					parser_report_error(parser, expr->string, STRING("Expected vector1 or vector 2 argument"));
					return false;
				case MICHI_ACTION_CHANGE:
					parser_report_error(parser, expr->string, STRING("Expected vector1, vector2, vector3 or vector4 argument"));
					return false;
				case MICHI_ACTION_FOLLOW:
				case MICHI_ACTION_DISP:
					parser_report_error(parser, expr->string, STRING("Expected 'on' or 'off' argument"));
					return false;
			}
		} break;

		case EXPR_KIND_STATEMENT: {
			Expr *left = expr->statement.left;
			Expr *right = expr->statement.right;

			switch (left->kind) {
				case EXPR_KIND_ACTION: {
					if (left->action.kind == MICHI_ACTION_EXIT) {
						parser_report_error(parser, left->string, STRING("Action takes no arguments"));
						return false;
					}

					if (left->action.kind == MICHI_ACTION_FOLLOW || left->action.kind == MICHI_ACTION_DRAW) {
						if (right->kind == EXPR_KIND_CONST) {
							Michi_Const const_kind = right->constant.kind;
							if (const_kind == MICHI_CONST_ON) {
								if (left->action.kind == MICHI_ACTION_FOLLOW)
									michi->follow = true;
								else
									michi->draw = true;
								return true;
							} else if (const_kind == MICHI_CONST_OFF) {
								if (left->action.kind == MICHI_ACTION_FOLLOW)
									michi->follow = false;
								else
									michi->draw = false;
								return true;
							}
						}
						parser_report_error(parser, right->string, STRING("Expected 'on' or 'off' argument"));
						return false;
					} else if (left->action.kind == MICHI_ACTION_DISP) {
						if (right->kind == EXPR_KIND_CONST) {
							Michi_Const const_kind = right->constant.kind;
							if (const_kind == MICHI_CONST_HELP) {
								michi->panel.disp[PANEL_DISP_HELP] = !michi->panel.disp[PANEL_DISP_HELP];
								return true;
							} else if (const_kind == MICHI_CONST_EXPR) {
								michi->panel.disp[PANEL_DISP_EXPR] = !michi->panel.disp[PANEL_DISP_EXPR];
								return true;
							}
						} else if (right->kind == EXPR_KIND_VAR) {
							switch (right->var.kind) {
								case MICHI_VAR_POSITION:
									michi->panel.disp[PANEL_DISP_POSITION] = !michi->panel.disp[PANEL_DISP_POSITION];
									return true;
								case MICHI_VAR_ROTATION:
									michi->panel.disp[PANEL_DISP_ROTATION] = !michi->panel.disp[PANEL_DISP_ROTATION];
									return true;
								case MICHI_VAR_SCALE:
									michi->panel.disp[PANEL_DISP_SCALE] = !michi->panel.disp[PANEL_DISP_SCALE];
									return true;
								case MICHI_VAR_COLOR:
									michi->panel.disp[PANEL_DISP_COLOR] = !michi->panel.disp[PANEL_DISP_COLOR];
									return true;
								case MICHI_VAR_SPEED:
									michi->panel.disp[PANEL_DISP_SPEED] = !michi->panel.disp[PANEL_DISP_SPEED];
									return true;
								case MICHI_VAR_OUTPUT:
									michi->panel.disp[PANEL_DISP_OUTPUT] = !michi->panel.disp[PANEL_DISP_OUTPUT];
									return true;
							}
						}
						parser_report_error(parser, right->string, STRING("Invalid option"));
						return false;
					}

					if (!expr_resolves_to_literal(right)) {
						parser_report_error(parser, right->string, STRING("Expected r-value resolving to vector"));
						return false;
					}

					uint32_t dim;
					V4 in = expr_resolve(right, &dim);

					switch (left->action.kind) {
						case MICHI_ACTION_MOVE: {
							if (dim == 1) {
								michi->actor.move_distance = in.x;
								return true;
							}
							parser_report_error(parser, expr->string, STRING("Expected vector1 argument"));
							return false;
						} break;
						case MICHI_ACTION_ROTATE: {
							if (dim == 1) {
								michi->actor.rotation_target += TO_RADIANS(in.x);
								return true;
							}
							parser_report_error(parser, expr->string, STRING("Expected vector1 argument"));
							return false;
						} break;
						case MICHI_ACTION_ENLARGE: {
							if (dim <= 2) {
								memcpy(&michi->actor.scale_target, &in, sizeof(float) * dim);
								return true;
							}
							parser_report_error(parser, expr->string, STRING("Expected vector1 or vector2 argument"));
							return false;
						} break;
						case MICHI_ACTION_CHANGE: {
							memcpy(&michi->actor.color_target, &in, sizeof(float) * dim);
							return true;
						} break;
					}
				} break;

				case EXPR_KIND_VAR: {
					if (left->var.kind == MICHI_VAR_OUTPUT) {
						if (expr_resolves_to_literal(right)) {
							uint32_t dim;
							V4 in = expr_resolve(right, &dim);
							michi->output = in;
							michi->output_dim = dim;
							return true;
						} else {
							parser_report_error(parser, right->string, STRING("Expected r-value resolving to vector"));
							return false;
						}
					}

					if (left->var.vector_dim == 0) {
						parser_report_error(parser, left->string, STRING("Invalid variable"));
						return false;
					}

					if (expr_resolves_to_literal(right)) {
						uint32_t dim;
						V4 in = expr_resolve(right, &dim);
						if (dim == left->var.vector_dim) {
							memcpy(left->var.ptr, &in, sizeof(float) * dim);
							if (left->var.copy_ptr) {
								memcpy(left->var.copy_ptr, &in, sizeof(float) * dim);
							} else if (left->var.ptr == (float *)&michi->actor.position) {
								michi->actor.move_distance = 0;
							}
							return true;
						}
						parser_report_error(parser, expr->string, STRING("Incompatible types"));
						return false;
					} else {
						parser_report_error(parser, right->string, STRING("Expected r-value resolving to vector"));
						return false;
					}
				} break;

				default: {
					parser_report_error(parser, left->string, STRING("Expected action or variable"));
					return false;
				} break;
			}
		} break;
	}

	parser_report_error(parser, expr->string, STRING("Expected literal, variable or statement"));
	return false;
}

void michi_update(Michi *michi, float dt) {
	Actor *a = &michi->actor;

	float angle = michi->actor.rotation;
	float c = cosf(-angle);
	float s = sinf(-angle);
	V2 p = v2(0, 1);
	V2 dir;
	dir.x = p.x * c - p.y * s;
	dir.y = p.x * s + p.y * c;

	float prev_move_distance = a->move_distance;
	a->move_distance = lerp(a->move_distance, 0.0f, 1.0f - powf(1.0f - a->speed.position, dt));
	a->position = v2add(a->position, v2mul(dir, prev_move_distance - a->move_distance));
	a->rotation = lerp(a->rotation, a->rotation_target, 1.0f - powf(1.0f - a->speed.rotation, dt));
	a->scale = v2lerp(a->scale, a->scale_target, 1.0f - powf(1.0f - a->speed.scale, dt));
	a->color = v4lerp(a->color, a->color_target, 1.0f - powf(1.0f - a->speed.color, dt));

	if (michi->follow) {
		michi->position = v2lerp(michi->position, a->position, 1.0f - powf(1.0f - .99f, dt));
	}

	if (michi->draw) {
		if (a->move_distance > 1.0f) {
			stroke_buffer_add(&michi->strokes, a->position, a->scale.x, a->scale.y, a->color);
		}
	}

	panel_update(&michi->panel, dt);
}

void michi_render(Michi *michi) {
	glLoadIdentity();

	float aspect_ratio = (float)context.framebuffer_w / (float)context.framebuffer_h;
	float half_height = michi->size;
	float half_width = half_height * aspect_ratio;

	glOrtho(-half_width, half_width, -half_height, half_height, -1, 1);

	glTranslatef(-michi->position.x, -michi->position.y, 0);

	{
		glBegin(GL_TRIANGLES);

		size_t count = michi->strokes.count;
		Stroke *strk = michi->strokes.ptr;
		for (size_t index = 0; index < count; ++index, ++strk) {
			render_ellipse(strk->p, strk->ra, strk->rb, strk->c, 0);
		}

		glEnd();
	}

	actor_render(&michi->actor);

	panel_render(&michi->panel);
}

int main(int argc, char *argv[]) {
	if (!context_create()) {
		return -1;
	}

	Michi *michi = michi_malloc(sizeof(Michi));
	if (michi == NULL) {
		fprintf(stderr, "Out of memory!\n");
		return -1;
	}

	if (!michi_create(100, NULL, michi)) {
		fprintf(stderr, "Failed to create Michi\n");
		return -1;
	}

	render_init();

	glfwSetWindowUserPointer(context.window, &michi->panel);
	glfwSetCursorPosCallback(context.window, panel_on_cursor_pos_changed);
	glfwSetCharCallback(context.window, panel_on_text_input);
	glfwSetKeyCallback(context.window, panel_on_key_input);
	glfwSetMouseButtonCallback(context.window, panel_on_mouse_input);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSwapInterval(1);

	uint64_t frequency = glfwGetTimerFrequency();
	uint64_t counter = glfwGetTimerValue();
	float dt = 1.0f / 60.0f;

	while (!glfwWindowShouldClose(context.window)) {
		glfwPollEvents();

		glfwGetFramebufferSize(context.window, &context.framebuffer_w, &context.framebuffer_h);
		glfwGetWindowSize(context.window, &context.window_w, &context.window_h);

		michi_update(michi, dt);

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glViewport(0, 0, context.framebuffer_w, context.framebuffer_h);

		michi_render(michi);

		glfwSwapBuffers(context.window);

		uint64_t new_counter = glfwGetTimerValue();
		uint64_t counts = new_counter - counter;
		counter = new_counter;
		dt = ((1000000.0f * (float)counts) / (float)frequency) / 1000000.0f;
	}

	context_destory();

	return 0;
}
