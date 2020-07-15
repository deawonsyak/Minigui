/*
 * File: dw_style.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 2001-2003 Sebastian Geerken <S.Geerken@ping.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mgdconfig.h"
#include <minigui/common.h>
#include <minigui/gdi.h>

#include <glib.h>

#include "dw_widget.h"
#include "dw_viewport.h"
#include "dw_style.h"
#include "image.h"
#include "prefs.h"
#include "dicache.h"

#define DEBUG_LEVEL 10
#include "debug.h"

#define EQUIV(a, b) (((a) && (b)) || (!(a) && !(b)))

#define Dw_style_font_ref(font)   ((font)->ref_count++)
#define Dw_style_font_unref(font) if (--((font)->ref_count) == 0) \
                                     Dw_style_font_remove (font)

#define Dw_style_color_ref(color)   ((color)->ref_count++)
#define Dw_style_color_unref(color) if (--((color)->ref_count) == 0) \
                                       Dw_style_color_remove (color)

#define Dw_style_bgimage_ref(bg_image)   do { (bg_image)->ref_count++; \
                                         } while (0);
#define Dw_style_bgimage_unref(bg_image)   if (--((bg_image)->ref_count) == 0) \
                                       Dw_style_bgimage_remove (bg_image)

#define Dw_style_shaded_color_ref(color)   ((color)->ref_count++)

#define Dw_style_shaded_color_unref(color) if (--((color)->ref_count) == 0) \
                                              Dw_style_shaded_color_remove \
                                                 (color) \

static GHashTable *fonts_table;
static GHashTable *colors_table;
static GHashTable *shaded_colors_table;

static gint styles_num = 0;
extern char* set_charset;



/* Used by a_Dw_style_numtostr(). */
static const char
   *roman_I0[] ={"I","II","III","IV","V","VI","VII","VIII","IX"},
   *roman_I1[] ={"X","XX","XXX","XL","L","LX","LXX","LXXX","XC"},
   *roman_I2[] ={"C","CC","CCC","CD","D","DC","DCC","DCCC","CM"},
   *roman_I3[] ={"M","MM","MMM","MMMM"};


static gint Dw_style_font_equal (gconstpointer v1, gconstpointer v2);
static guint Dw_style_font_hash (gconstpointer key);
static void Dw_style_font_remove (DwStyleFont *font);

static void Dw_style_color_remove (DwStyleColor *color);
static void Dw_style_bgimage_remove (DwStyleBgImage* background_image);
static void Dw_style_shaded_color_remove (DwStyleShadedColor *color);


/* ----------------------------------------------------------------------
 *
 *    Initialization / Cleaning up
 *
 * ----------------------------------------------------------------------
 */

/*
 * Initialize the DwStyle submodule.
 */
void a_Dw_style_init (void)
{
    fonts_table = g_hash_table_new (Dw_style_font_hash, Dw_style_font_equal);
    colors_table = g_hash_table_new (g_direct_hash, g_direct_equal);
    shaded_colors_table = g_hash_table_new (g_direct_hash, g_direct_equal);
}


/*
 * Called by a_Dw_style_freeall.
 */
static void Dw_style_count_fonts (gpointer key,
                                  gpointer value,
                                  gpointer user_data)
{
    DwStyleFont *font = (DwStyleFont*) key;
    gint *count = (int*)user_data;

    count[0]++;
    count[1] += font->ref_count;
}

/*
 * Called by a_Dw_style_freeall.
 */
static void Dw_style_count_colors (gpointer key,
                                   gpointer value,
                                   gpointer user_data)
{
    DwStyleColor *color = (DwStyleColor*) value;
    gint *count = (int*)user_data;

    count[0]++;
    count[1] += color->ref_count;
}

/*
 * Called by a_Dw_style_freeall.
 */
static void Dw_style_count_shaded_colors (gpointer key,
                                          gpointer value,
                                          gpointer user_data)
{
    DwStyleShadedColor *color = (DwStyleShadedColor*) value;
    gint *count = (int*)user_data;

    count[0]++;
    count[1] += color->ref_count;
}


/*
 * Free variables used by DwStyle, and do a check whether memory
 * management works properly.
 */
void a_Dw_style_freeall (void)
{
    gint count[2];

    if (styles_num)
        g_warning ("%d styles left", styles_num);

    count[0] = count[1] = 0;
    g_hash_table_foreach (fonts_table, Dw_style_count_fonts, count);
    if (count[0] || count[1])
        g_warning ("%d fonts (%d references) left", count[0], count[1]);

    count[0] = count[1] = 0;
    g_hash_table_foreach (colors_table, Dw_style_count_colors, count);
    if (count[0] || count[1])
        g_warning ("%d colors (%d references) left", count[0], count[1]);

    count[0] = count[1] = 0;
    g_hash_table_foreach (shaded_colors_table, Dw_style_count_shaded_colors,
                         count);
    if (count[0] || count[1])
        g_warning ("%d shaded colors (%d references) left", count[0], count[1]);

    g_hash_table_destroy (fonts_table);
    g_hash_table_destroy (colors_table);
}


/* ----------------------------------------------------------------------
 *
 *    Styles
 *
 * ----------------------------------------------------------------------
 */


/*
 * Set all style fields except font and color to reasonable defaults.
 */
void a_Dw_style_init_values (DwStyle *style_attrs, HWND wnd)
{
    style_attrs->x_link = -1;
    style_attrs->x_tooltip = NULL; /* fixed */
    style_attrs->text_decoration = 0;
    style_attrs->text_align = DW_STYLE_TEXT_ALIGN_LEFT;
    style_attrs->list_style_type = DW_STYLE_LIST_STYLE_TYPE_DISC;
    style_attrs->valign = DW_STYLE_VALIGN_MIDDLE;
    style_attrs->background_color = NULL;
    style_attrs->background_image = NULL;
    style_attrs->width = DW_STYLE_LENGTH_AUTO;
    style_attrs->height = DW_STYLE_LENGTH_AUTO;

    a_Dw_style_box_set_val (&style_attrs->margin, 0);
    a_Dw_style_box_set_val (&style_attrs->border_width, 0);
    a_Dw_style_box_set_val (&style_attrs->padding, 0);
    a_Dw_style_box_set_border_color (style_attrs, NULL);
    a_Dw_style_box_set_border_style (style_attrs, DW_STYLE_BORDER_NONE);
    style_attrs->border_spacing = 0;

    style_attrs->display = DW_STYLE_DISPLAY_INLINE;
    style_attrs->white_space = DW_STYLE_WHITE_SPACE_NORMAL;
}


/*
 * Reset those style attributes to their standard values, which are
 * not inherited, according to CSS.
 */
void a_Dw_style_reset_values (DwStyle *style_attrs)
{
    style_attrs->x_link = -1;
    style_attrs->x_tooltip = NULL; /* fixed */

    style_attrs->text_align = DW_STYLE_TEXT_ALIGN_LEFT; /* ??? */
    style_attrs->valign = DW_STYLE_VALIGN_MIDDLE;
    style_attrs->text_align_char = '.';
    style_attrs->background_color = NULL;
    style_attrs->background_image = NULL;
    style_attrs->width = DW_STYLE_LENGTH_AUTO;
    style_attrs->height = DW_STYLE_LENGTH_AUTO;

    a_Dw_style_box_set_val (&style_attrs->margin, 0);
    a_Dw_style_box_set_val (&style_attrs->border_width, 0);
    a_Dw_style_box_set_val (&style_attrs->padding, 0);
    a_Dw_style_box_set_border_color (style_attrs, NULL);
    a_Dw_style_box_set_border_style (style_attrs, DW_STYLE_BORDER_NONE);
    style_attrs->border_spacing = 0;

    style_attrs->display = DW_STYLE_DISPLAY_INLINE;
    style_attrs->white_space = DW_STYLE_WHITE_SPACE_NORMAL;
}

/*
 * Return a new DwStyle, with increased reference pointer.
 */
DwStyle* a_Dw_style_new (DwStyle *style_attrs, HWND wnd)
{
    DwStyle *style;

    style = g_new (DwStyle, 1);
    *style = *style_attrs;
    style->ref_count = 1;

    if (style->font)
        Dw_style_font_ref (style->font);
    if (style->color)
        Dw_style_color_ref (style->color);
    if (style->background_color)
        Dw_style_color_ref (style->background_color);
    if (style->background_image)
        Dw_style_bgimage_ref (style->background_image);
    if (style->border_color.top)
        Dw_style_shaded_color_ref (style->border_color.top);
    if (style->border_color.right)
        Dw_style_shaded_color_ref (style->border_color.right);
    if (style->border_color.bottom)
        Dw_style_shaded_color_ref (style->border_color.bottom);
    if (style->border_color.left)
        Dw_style_shaded_color_ref (style->border_color.left);
    if (style->x_tooltip)
        a_Dw_tooltip_ref (style->x_tooltip);
    styles_num++;
    return style;
}

/*
 * Remove a style (called when ref_count == 0).
 */
void Dw_style_remove (DwStyle *style)
{
    if(style->font)
        Dw_style_font_unref (style->font);
    if(style->color)
        Dw_style_color_unref (style->color);
    if (style->background_color)
        Dw_style_color_unref (style->background_color);
    if (style->background_image)
        Dw_style_bgimage_unref (style->background_image);
    if (style->border_color.top)
        Dw_style_shaded_color_unref (style->border_color.top);
    if (style->border_color.right)
        Dw_style_shaded_color_unref (style->border_color.right);
    if (style->border_color.bottom)
        Dw_style_shaded_color_unref (style->border_color.bottom);
    if (style->border_color.left)
        Dw_style_shaded_color_unref (style->border_color.left);
    /* fixed */
    if (style->x_tooltip)
        a_Dw_tooltip_unref (style->x_tooltip);
    /* fixed */

    g_free (style);
    styles_num--;
}

/* ----------------------------------------------------------------------
 *
 *    Fonts
 *
 * ----------------------------------------------------------------------
 */

/*
 * Create the GdkFont. font->name contains one name. If try_all is
 * TRUE, try also standard fonts, if anything else fails.
 */
static void Dw_style_font_realize (DwStyleFont *font, gboolean try_all)
{
    char fontname[256], style_char_1 = 'r', style_char_2 = 'r';

   switch (font->style) {
   case DW_STYLE_FONT_STYLE_NORMAL:
      style_char_1 = style_char_2 = 'r';
      break;
   case DW_STYLE_FONT_STYLE_ITALIC:
      style_char_1 = 'i';
      style_char_2 = 'o';
      break;
   case DW_STYLE_FONT_STYLE_OBLIQUE:
      style_char_1 = 'o';
      style_char_2 = 'i';
      break;
   }

   
    sprintf (fontname, "*-%s-%c%cncnn-*-%d-%s",
                font->name,
#ifdef ENABLE_TVOUTPUT
            (font->weight >= 500) ? 'd' : 'k',
#else
            (font->weight >= 500) ? 'b' : 'r',
#endif
            style_char_1, font->size, font->charset);
	printf("func:%s line:%d fontname:%s\n", __func__, __LINE__, fontname);
    font->font = CreateLogFontByName (fontname);

    if (font->font == NULL && font->style != DW_STYLE_FONT_STYLE_NORMAL) {
        sprintf (fontname, "*-%s-%c%cncnn-*-%d-%s",
                font->name,
#ifdef ENABLE_TVOUTPUT
               (font->weight >= 500) ? 'd' : 'k',
#else
               (font->weight >= 500) ? 'b' : 'r',
#endif
               style_char_2, font->size, font->charset);
		printf("func:%s line:%d fontname:%s\n", __func__, __LINE__, fontname);
        font->font = CreateLogFontByName (fontname);
    }
	printf("func:%s line:%d font->font:%x\n", __func__, __LINE__, font->font);
   if (font->font) {
      SIZE ext;

      SelectFont (HDC_SCREEN, font->font);
      GetTextExtent (HDC_SCREEN, " ", 1, &ext);
      font->space_width = ext.cx;
      font->x_height = ext.cy;
   }
}

/*
 * Used for the font_table hash table.
 */
static gint Dw_style_font_equal (gconstpointer v1, gconstpointer v2)
{
   const DwStyleFont *font1 = (DwStyleFont*) v1, *font2 = (DwStyleFont*) v2;

   return (font1->size == font2->size &&
           font1->weight == font2->weight &&
           font1->style == font2->style &&
           strcmp (font1->name, font2->name) == 0 &&
           strcmp (font1->charset, font2->charset) == 0);
}


/*
 * Used for the font_table hash table.
 */
static guint Dw_style_font_hash (gconstpointer key)
{
    guint h;
    const DwStyleFont *font = (DwStyleFont*) key;
    char name_charset [128];

    strncpy (name_charset, font->name, 63);
    strncat (name_charset, font->charset, 63);
    h = g_str_hash (name_charset);
    h = (h << 5) - h + font->size;
    h = (h << 5) - h + font->weight;
    h = (h << 5) - h + font->style;
    return h;
}

/*
 * Returns a new or already existing font. This function is only used
 * internally, and called by a_Dw_style_font_new and
 * a_Dw_style_font_new_from_list.
 *
 * Note that the reference counter is not increased/set to zero, it
 * will be increased by a_Dw_style_new. If 'try_all' is TRUE, try also
 * standard fonts, if anything else fails.
 */
static DwStyleFont* Dw_style_font_new_internal (DwStyleFont *font_attrs,
                                                gboolean try_all)
{
    DwStyleFont *font;

    g_return_val_if_fail (font_attrs->name != NULL, NULL);

    if ((font = g_hash_table_lookup (fonts_table, font_attrs))) {
        return font;
    }
    else {
        font = g_new (DwStyleFont, 1);
        font->size = font_attrs->size;
        font->weight = font_attrs->weight;
        font->style = font_attrs->style;
        font->name = g_strdup (font_attrs->name);
        font->charset = g_strdup (font_attrs->charset);
        font->ref_count = 0;

        Dw_style_font_realize (font, try_all);
        if (font->font) {
            g_hash_table_insert (fonts_table, font, font);
            return font;
        }
        else {
            g_free (font->name);
            g_free (font->charset);
            g_free (font);
            return NULL;
        }
    }
}

/*
 * Return a new or already existing font, with one name in
 * font_attrs->name. See also Dw_style_font_new_internal.
 */
DwStyleFont* a_Dw_style_font_new (DwStyleFont *font_attrs)
{
    DwStyleFont *font;

    font = Dw_style_font_new_internal (font_attrs, TRUE);
    if (font == NULL)
        g_error ("Could not find any font.");

    return font;
}

/*
 * Return a new or already existing font, with font_attrs->name
 * containing a comma separated list of names. See also
 * Dw_style_font_new_internal.
 */
DwStyleFont* a_Dw_style_font_new_from_list (DwStyleFont *font_attrs,
                                            gchar *default_family,
                                            gchar *default_charset)
{
    DwStyleFont *font = NULL, font_attrs2;
    char *comma, *list, *current;

    font_attrs2 = *font_attrs;
    current = list = g_strdup (font_attrs->name);

    while (current && (font == NULL)) {
        comma = strchr (current, ',');
        if (comma) *comma = 0;

        font_attrs2.name = current;
        font = Dw_style_font_new_internal (&font_attrs2, FALSE);
        if (font)
            break;

        if (comma) {
            current = comma + 1;
            while (isspace (*current)) current++;
        }
        else
            current = NULL;
    }

    g_free (list);

    if (font == NULL) {
        font_attrs2.name = default_family;
        font_attrs2.charset = default_charset;
        font = Dw_style_font_new_internal (&font_attrs2, TRUE);
    }

    if (font == NULL)
        g_error ("Could not find any font.");

    return font;
}

void a_Dw_style_change_font (DwStyle *style, DwStyleFont *new_font)
{
    if (new_font == NULL)
        return;

    Dw_style_font_ref (new_font);
    if (style->font) {
        Dw_style_font_unref (style->font);
    }

    style->font = new_font;
}

/*
 * Remove a font (called when ref_count == 0).
 */
static void Dw_style_font_remove (DwStyleFont *font)
{
    g_hash_table_remove (fonts_table, font);
    g_free (font->name);
    g_free (font->charset);
    DestroyLogFont (font->font);
    g_free (font);
}


/* ----------------------------------------------------------------------
 *
 *    Colors
 *
 * ----------------------------------------------------------------------
 */

/*
 * Copied from gtkstyle.c.
 * Convert RGB into HLS.
 */
static void Dw_style_rgb_to_hls (gdouble *r,
                                 gdouble *g,
                                 gdouble *b)
{
    gdouble min;
    gdouble max;
    gdouble red;
    gdouble green;
    gdouble blue;
    gdouble h, l, s;
    gdouble delta;

    red = *r;
    green = *g;
    blue = *b;

    if (red > green) {
        if (red > blue)
            max = red;
        else
            max = blue;

        if (green < blue)
            min = green;
        else
            min = blue;
    }
    else {
        if (green > blue)
            max = green;
        else
            max = blue;

        if (red < blue)
            min = red;
        else
            min = blue;
    }

    l = (max + min) / 2;
    s = 0;
    h = 0;

    if (max != min) {
        if (l <= 0.5)
            s = (max - min) / (max + min);
        else
            s = (max - min) / (2 - max - min);

        delta = max -min;
        if (red == max)
            h = (green - blue) / delta;
        else if (green == max)
            h = 2 + (blue - red) / delta;
        else if (blue == max)
            h = 4 + (red - green) / delta;

        h *= 60;
        if (h < 0.0)
            h += 360;
    }

    *r = h;
    *g = l;
    *b = s;
}

/*
 * Copied from gtkstyle.c.
 * Convert HLS into RGB.
 */
static void Dw_style_hls_to_rgb (gdouble *h,
                                 gdouble *l,
                                 gdouble *s)
{
    gdouble hue;
    gdouble lightness;
    gdouble saturation;
    gdouble m1, m2;
    gdouble r, g, b;

    lightness = *l;
    saturation = *s;

    if (lightness <= 0.5)
        m2 = lightness * (1 + saturation);
    else
        m2 = lightness + saturation - lightness * saturation;
    m1 = 2 * lightness - m2;

    if (saturation == 0) {
        *h = lightness;
        *l = lightness;
        *s = lightness;
    }
    else {
        hue = *h + 120;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            r = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            r = m2;
        else if (hue < 240)
            r = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            r = m1;

        hue = *h;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            g = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            g = m2;
        else if (hue < 240)
            g = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            g = m1;

        hue = *h - 120;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            b = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            b = m2;
        else if (hue < 240)
            b = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            b = m1;

        *h = r;
        *l = g;
        *s = b;
    }
}

/*
 * d is a factor the color is multiplied with before, this is needed
 * for shaded colors.
 */
static void Dw_style_color_create (gint color_val,
                                   RGB *color,
                                   gint d)
{
    gint red, green, blue;
    gdouble hue, lightness, saturation;

    red = (color_val >> 16) & 255;
    green = (color_val >> 8) & 255;
    blue = color_val & 255;

    if (d) {
        hue = (gdouble)red / 255;
        lightness = (gdouble)green / 255;
        saturation = (gdouble)blue / 255;

        Dw_style_rgb_to_hls (&hue, &lightness, &saturation);

        if (lightness > 0.8) {
            if (d > 0)
                lightness -= 0.2;
            else
                lightness -= 0.4;
        }
        else if (lightness < 0.2) {
            if (d > 0)
                lightness += 0.4;
            else
                lightness += 0.2;
        }
        else
            lightness += d * 0.2;

        Dw_style_hls_to_rgb (&hue, &lightness, &saturation);
        DEBUG_MSG (1, "(%1.3g, %1.3g, %1.3g)\n", hue, lightness, saturation);

        red = hue * 255;
        green = lightness * 255;
        blue = saturation * 255;
    }

    blue |= blue << 8;
    red |= red << 8;
    green |= green << 8;

    color->r = red;
    color->g = green;
    color->b = blue;
}

/*
   Return a new or already existing color. color_val has the form
   0xrrggbb.
 */
DwStyleColor* a_Dw_style_color_new (gint color_val, HWND wnd)
{
    HDC hdc = HDC_SCREEN;
    DwStyleColor *color;

    color = g_hash_table_lookup (colors_table, GINT_TO_POINTER (color_val));
    if (color == NULL) {
        color = g_new (DwStyleColor, 1);
        color->ref_count = 0;
        color->color_val = color_val;

        Dw_style_color_create (color_val, &color->color, 0);
        color->pixel = RGB2Pixel (hdc, 
                        color->color.r, 
                        color->color.g, 
                        color->color.b);
        Dw_style_color_create (color_val ^ 0xffffff, &color->inverse_color, 0);
        color->inverse_pixel = RGB2Pixel (hdc, 
                        color->inverse_color.r, 
                        color->inverse_color.g, 
                        color->inverse_color.b);
        g_hash_table_insert (colors_table, GINT_TO_POINTER (color_val), color);
    }

    return color;
}

/*
 * Remove a color (called when ref_count == 0).
 */
static void Dw_style_color_remove (DwStyleColor *color)
{
    g_hash_table_remove (colors_table,  GINT_TO_POINTER (color->color_val));

    g_free (color);
}

/*

  */
DwStyleBgImage* a_Dw_style_bgimage_new (DwImage* image)
{
    DwStyleBgImage* background_image;

    background_image = g_new0 (DwStyleBgImage, 1);
    background_image->ref_count = 0;
    background_image->draw_bgimg_idle = 0;
    background_image->image = image;
    background_image->state = IMAGE_NOREADY;
    background_image->bmp = NULL;

    return background_image;
}

static void Dw_style_bgimage_remove (DwStyleBgImage* background_image)
{
    DwImage * image;

    if (background_image->draw_bgimg_idle != 0)
        g_idle_remove_by_data (background_image);

    if (background_image->bmp) {
        UnloadBitmap (background_image->bmp);
        g_free (background_image->bmp);
    }

    if (background_image->image) {
        image = background_image->image;
        if (image->usemap_url)
            a_Url_free (image->usemap_url);
        if (image->url)
            a_Dicache_unref (image->url, image->version);
        if (image->alt_text)
            g_free (image->alt_text);
        if (image->scaled_buffer)
            g_free (image->scaled_buffer);
#ifdef ENABLE_ANIMATION
        if (image->timer)
            g_source_remove (image->timer);
        if (image->cmap)
            g_free (image->cmap);
        if (image->frames)
            destroy_gif_frames (image);
#endif
            g_free (background_image->image);
        }

    g_free (background_image);
}

/*
 * Return a new or already existing shaded color. color_val has the
 * form 0xrrggbb.
 */
DwStyleShadedColor* a_Dw_style_shaded_color_new (gint color_val, HWND wnd)
{
    HDC hdc = HDC_SCREEN;
    DwStyleShadedColor *color;

    color =
        g_hash_table_lookup (shaded_colors_table, GINT_TO_POINTER (color_val));

    if (color == NULL) {
        color = g_new (DwStyleShadedColor, 1);
        color->ref_count = 0;
        color->color_val = color_val;

        Dw_style_color_create (color_val, &color->color, 0);
        color->pixel = RGB2Pixel (hdc, 
                        color->color.r, 
                        color->color.g, 
                        color->color.b);
        Dw_style_color_create (color_val ^ 0xffffff, &color->inverse_color, 0);
        color->inverse_pixel = RGB2Pixel (hdc, 
                        color->inverse_color.r, 
                        color->inverse_color.g, 
                        color->inverse_color.b);
        Dw_style_color_create (color_val, &color->color_dark, -1);
        color->pixel_dark = RGB2Pixel (hdc, 
                        color->color_dark.r, 
                        color->color_dark.g, 
                        color->color_dark.b);
        Dw_style_color_create (color_val, &color->color_light, +1);
        color->pixel_light = RGB2Pixel (hdc, 
                        color->color_light.r, 
                        color->color_light.g, 
                        color->color_light.b);
        g_hash_table_insert (shaded_colors_table,
                                GINT_TO_POINTER (color_val), color);
    }

    return color;
}

/*
 * Remove a shaded color (called when ref_count == 0).
 */
static void Dw_style_shaded_color_remove (DwStyleShadedColor *color)
{
    g_hash_table_remove (shaded_colors_table,
                            GINT_TO_POINTER (color->color_val));
    g_free (color);
}

/*
 * This function returns whether something may change its size, when
 * its style changes from style1 to style2. It is mainly for
 * optimizing style changes where only colors etc change (where FALSE
 * would be returned), in some cases it may return TRUE, although a
 * size change does not actually happen (e.g. when in a certain
 * context a particular attribute is ignored).
 */
gboolean a_Dw_style_size_diffs (DwStyle *style1,
                                DwStyle *style2)
{
    /* todo: Should for CSS implemented properly. Currently, size
     * changes are not needed, so always FALSE is returned. See also
     * a_Dw_widget_set_style(). */
    return FALSE;
}


/* ----------------------------------------------------------------------
 *
 *    Drawing functions
 *
 * ----------------------------------------------------------------------
 */

/*
 * Draw a part of a border.
 */
static void Dw_style_draw_polygon (HDC hdc, const RGB* color,
                                   gint32 x1, gint32 y1, gint32 x2, gint32 y2,
                                   gint32 width, gint32 w1, gint32 w2)
{
    POINT points[4];

    SetPenColor (hdc, RGB2Pixel (hdc, color->r, color->g, color->b));
    SetBrushColor (hdc, RGB2Pixel (hdc, color->r, color->g, color->b));

    if (width != 0) {
        if (width == 1) {
            if (x1 == x2) {
                MoveTo (hdc, x1, y1);
                LineTo (hdc, x2, y2 - 1);
            }
            else {
                MoveTo (hdc, x1, y1);
                LineTo (hdc, x2 - 1, y2);
            }
        }
        else if (width == -1) {
            if (x1 == x2) {
                MoveTo (hdc, x1 - 1, y1);
                LineTo (hdc, x2 - 1, y2 - 1);
            }
            else {
                MoveTo (hdc, x1, y1 - 1);
                LineTo (hdc, x2 - 1, y2 - 1);
            }
        }
        else {
            points[0].x = x1;
            points[0].y = y1;
            points[1].x = x2;
            points[1].y = y2;
    
            if (x1 == x2) {
                points[2].x = x1 + width;
                points[2].y = y2 + w2;
                points[3].x = x1 + width;
                    points[3].y = y1 + w1;
            }
            else {
                points[2].x = x2 + w2;
                points[2].y = y1 + width;
                points[3].x = x1 + w1;
                points[3].y = y1 + width;
            }

            FillPolygon (hdc, points, 4);
        }
    }
}

/*
 * Draw the border of a region in window, according to style. Used by
 * Dw_widget_draw_box and Dw_widget_draw_widget_box.
 */

#define LIMIT(v)    if ((v) < -30000) v = -30000; \
                    if ((v) > 30000) v = 30000

void p_Dw_style_draw_border (HDC hdc,
                             DwRectangle *area,
                             gint32 vx,
                             gint32 vy,
                             gint32 x,
                             gint32 y,
                             gint32 width,
                             gint32 height,
                             DwStyle *style,
                             gboolean inverse)
{
   /* todo: a lot! */
    RGB *dark_color, *light_color, *normal_color;
    RGB *top_color, *right_color, *bottom_color, *left_color;
    gint32 xb1, yb1, xb2, yb2, xp1, yp1, xp2, yp2;

    if (style->border_style.top == DW_STYLE_BORDER_NONE)
        return;

    xb1 = x + style->margin.left - vx;
    yb1 = y + style->margin.top - vy;
    xb2 = xb1 + width - style->margin.left - style->margin.right;
    yb2 = yb1 + height - style->margin.top - style->margin.bottom;

    xp1 = xb1 + style->border_width.top;
    yp1 = yb1 + style->border_width.left;
    xp2 = xb2 + style->border_width.bottom;
    yp2 = yb2 + style->border_width.right;

    /* Make sure that we pass 16-bit values to Gdk functions. */
    LIMIT (xb1);
    LIMIT (xb2);
    LIMIT (yb1);
    LIMIT (yb2);

    light_color = inverse ? &style->border_color.top->color_dark :
        &style->border_color.top->color_light;
    dark_color = inverse ? &style->border_color.top->color_light :
        &style->border_color.top->color_dark;
    normal_color = inverse ? &style->border_color.top->inverse_color :
        &style->border_color.top->color;

    switch (style->border_style.top) {
    case DW_STYLE_BORDER_INSET:
        top_color = left_color = dark_color;
        right_color = bottom_color = light_color;
        break;

    case DW_STYLE_BORDER_OUTSET:
        top_color = left_color = light_color;
        right_color = bottom_color = dark_color;
        break;

    default:
        top_color = right_color = bottom_color = left_color = normal_color;
        break;
    }

    Dw_style_draw_polygon (hdc, top_color, xb1, yb1, xb2, yb1,
                            style->border_width.top,
                            style->border_width.left,
                            - style->border_width.right);
    Dw_style_draw_polygon (hdc, right_color, xb2, yb1, xb2, yb2,
                            - style->border_width.right,
                            style->border_width.top,
                            - style->border_width.bottom);
    Dw_style_draw_polygon (hdc, bottom_color, xb1, yb2, xb2, yb2,
                            - style->border_width.bottom,
                            style->border_width.left,
                            - style->border_width.right);
    Dw_style_draw_polygon (hdc, left_color, xb1, yb1, xb1, yb2,
                            style->border_width.left,
                            style->border_width.top,
                          - style->border_width.bottom);
}


/*
 * Draw the background (content plus padding) of a region in window,
 * according to style. Used by Dw_widget_draw_box and
 * Dw_widget_draw_widget_box.
 */
void p_Dw_style_draw_background (HDC hdc,
                                 DwRectangle *area,
                                 gint32 vx,
                                 gint32 vy,
                                 gint32 x,
                                 gint32 y,
                                 gint32 width,
                                 gint32 height,
                                 DwStyle *style,
                                 gboolean inverse)
{
    DwRectangle dw_area, bg_area, intersection;

    if (style->background_color) {
        dw_area.x = area->x + vx;
        dw_area.y = area->y + vy;
        dw_area.width = area->width;
        dw_area.height = area->height;

        bg_area.x = x + style->margin.left + style->border_width.left;
        bg_area.y = y + style->margin.top + style->border_width.top;
        bg_area.width =
            width - style->margin.left - style->border_width.left -
            style->margin.right - style->border_width.right;
        bg_area.height =
            height - style->margin.top - style->border_width.top -
            style->margin.bottom - style->border_width.bottom;

        if (p_Dw_rectangle_intersect (&dw_area, &bg_area, &intersection)) {
            gal_pixel pixel = inverse ? style->background_color->inverse_pixel :
                                style->background_color->pixel;
            SetBrushColor (hdc, pixel);
			printf("##yk##FillBox hdc:%x, pixel:%x\n", hdc, pixel);
            FillBox (hdc, intersection.x - vx, intersection.y - vy,
                                    intersection.width, intersection.height);
        }
    }
}

static gint Dw_bgimage_idle (gpointer data)
{
    DwStyleBgImage *sbi = (DwStyleBgImage*)data;
    MYBITMAP my_bmp;

    if (sbi->image == NULL) {
        sbi->draw_bgimg_idle = 0;
        return FALSE;
    }

    if (((DwImage*)(sbi->image))->state != DW_IMAGE_DONE) {
         sbi->state = IMAGE_LOADING;
         return TRUE;
    }

    my_bmp.flags = MYBMP_TYPE_RGB | MYBMP_FLOW_DOWN | MYBMP_RGBSIZE_3;
    my_bmp.frames = 1;
    my_bmp.depth = 24;
    my_bmp.w = ((DwImage*)sbi->image)->width;
    my_bmp.h = ((DwImage*)sbi->image)->height;
    my_bmp.pitch = ((DwImage*)sbi->image)->width * 3;
    my_bmp.bits = ((DwImage*)sbi->image)->buffer;

    /* create a bitmap */
    if ((sbi->bmp = g_malloc0(sizeof(BITMAP))) == NULL){ 
        sbi->draw_bgimg_idle = 0;
        return FALSE;
    }

    if (ExpandMyBitmap(HDC_SCREEN, sbi->bmp, &my_bmp, NULL, 1) != 0) {
        g_free (sbi->bmp);
        sbi->bmp = NULL;
        sbi->draw_bgimg_idle = 0;
        return FALSE;
    }

    sbi->state = IMAGE_READY;

    InvalidateRect (sbi->hwnd, &(sbi->rc), TRUE);
    sbi->draw_bgimg_idle = 0;
    return FALSE;
}


void p_Dw_style_draw_background_image (HDC hdc,
                                     DwRectangle *area,
                                     gint32 vx,
                                     gint32 vy,
                                     gint32 x,
                                     gint32 y,
                                     gint32 width,
                                     gint32 height,
                                     DwStyle *style,
                                     void * data)
{

   DwRectangle dw_area, bg_area, intersection;
   RECT wrc;
   int real_w, real_h, ix, iy;
   int x_bgimage_off;
   int y_bgimage_off;
   int w_num, h_num, w_offset, h_offset;
   DwWidget * widget = (DwWidget *)data;

   if (style->background_image) {

      if (widget->parent == NULL) {
            GetClientRect (((DwViewport*)widget->viewport)->hwnd, &wrc);

           style->background_image->hdc = hdc;
           style->background_image->hwnd =((DwViewport*)widget->viewport)->hwnd;
           style->background_image->rc = wrc;

            if (style->background_image->state == IMAGE_NOREADY) {
                if (style->background_image->draw_bgimg_idle == 0)
                    style->background_image->draw_bgimg_idle = 
                                g_idle_add_full (G_PRIORITY_HIGH_IDLE, 
                                      Dw_bgimage_idle, (gpointer)style->background_image, 
                                      NULL);
            }
            else if (style->background_image->state == IMAGE_READY)  {
                real_w = style->background_image->bmp->bmWidth;
                real_h = style->background_image->bmp->bmHeight;
                x_bgimage_off = (vx % real_w)*(-1);
                y_bgimage_off = (vy % real_h)*(-1);

                for(iy = y_bgimage_off; iy < wrc.bottom; iy += real_h){
                   for(ix = x_bgimage_off; ix < wrc.right; ix += real_w){
                       FillBoxWithBitmap (hdc, ix, iy, real_w, real_h, 
                                       style->background_image->bmp);
                   }
                }
            }
        }
        else {

            dw_area.x = area->x + vx;
            dw_area.y = area->y + vy;
            dw_area.width = area->width;
            dw_area.height = area->height;

            bg_area.x = x + style->margin.left + style->border_width.left;
            bg_area.y = y + style->margin.top + style->border_width.top;
            bg_area.width =
                width - style->margin.left - style->border_width.left -
                style->margin.right - style->border_width.right;
            bg_area.height =
                height - style->margin.top - style->border_width.top -
                style->margin.bottom - style->border_width.bottom;

            if (p_Dw_rectangle_intersect (&dw_area, &bg_area, &intersection)) {

                style->background_image->hdc = hdc;
                style->background_image->hwnd =((DwViewport*)widget->viewport)->hwnd;
                style->background_image->rc.left = intersection.x - vx;
                style->background_image->rc.top = intersection.y - vy;
                style->background_image->rc.right = style->background_image->rc.left 
                        + intersection.width;
                style->background_image->rc.bottom = style->background_image->rc.top 
                        + intersection.height;

                if (style->background_image->state == IMAGE_NOREADY) {
                    if (style->background_image->draw_bgimg_idle == 0)
                             style->background_image->draw_bgimg_idle = 
                                     g_idle_add_full (G_PRIORITY_HIGH_IDLE, 
                                            Dw_bgimage_idle, (gpointer)style->background_image,
                                            NULL);
                } 
                else if (style->background_image->state == IMAGE_READY
                        && style->background_image->bmp->bmWidth 
                        && style->background_image->bmp->bmHeight) {

                    w_num =(int) (bg_area.width/style->background_image->bmp->bmWidth);
                    h_num =(int) (bg_area.height/style->background_image->bmp->bmHeight);
                    w_offset =bg_area.width%style->background_image->bmp->bmWidth;
                    h_offset =bg_area.height%style->background_image->bmp->bmHeight;

                    for (iy = 0; iy<= h_num-1; iy++) {
                        for (ix =0; ix<= w_num-1; ix++) {
                            FillBoxWithBitmapPart (hdc, 
                                bg_area.x-vx+ix * style->background_image->bmp->bmWidth,
                                bg_area.y-vy +iy* style->background_image->bmp->bmHeight,
                                style->background_image->bmp->bmWidth,
                                style->background_image->bmp->bmHeight,
                                0, 0, style->background_image->bmp, 0, 0);
                        }
                    }

                    if (w_offset != 0) {
                        for (iy = 0; iy <= h_num-1; iy ++)
                            FillBoxWithBitmapPart (hdc, 
                                bg_area.x-vx+w_num * style->background_image->bmp->bmWidth,
                                bg_area.y-vy +iy* style->background_image->bmp->bmHeight,
                                w_offset,
                                style->background_image->bmp->bmHeight, 
                                0, 0, style->background_image->bmp, 0, 0);
            
                    }

                    if (h_offset != 0) {
                        for (ix = 0; ix <= w_num-1; ix ++)
                            FillBoxWithBitmapPart (hdc, 
                                bg_area.x-vx+ ix * style->background_image->bmp->bmWidth,
                                bg_area.y-vy +h_num* style->background_image->bmp->bmHeight,
                                style->background_image->bmp->bmWidth,
                                h_offset,
                                0, 0, style->background_image->bmp, 0, 0);
            
                    }

                    if (h_offset != 0 && w_offset !=0) {
                        FillBoxWithBitmapPart (hdc, 
                            bg_area.x-vx+ w_num * style->background_image->bmp->bmWidth,
                            bg_area.y-vy +h_num* style->background_image->bmp->bmHeight,
                            w_offset, h_offset, 0, 0, style->background_image->bmp, 0, 0);
                    }
                }
            }
        }
    }
}

/*
 * Convert a number into a string, in a given style. Used for ordered lists.
 */
void a_Dw_style_numtostr (gint num,
                          gchar *buf,
                          gint buflen,
                          DwStyleListStyleType list_style_tyle)
{
    int i3, i2, i1, i0;
    gboolean low = FALSE;

    switch (list_style_tyle) {
    case DW_STYLE_LIST_STYLE_TYPE_LOWER_ALPHA:
        low = TRUE;
    case DW_STYLE_LIST_STYLE_TYPE_UPPER_ALPHA:
        i0 = num - 1;
        i1 = i0/26; i2 = i1/26;
        i0 %= 26;   i1 %= 26;
        if (i2 > 26) /* more than 26*26*26=17576 elements ? */
            sprintf(buf, "****.");
        else if (i2)
            sprintf(buf, "%c%c%c.", 'A'+i2-1,'A'+i1-1,'A'+i0);
        else if (i1)
            sprintf(buf, "%c%c.", 'A'+i1-1,'A'+i0);
        else
            sprintf(buf, "%c.", 'A'+i0);
        if (low)
            g_strdown(buf);
        break;

    case DW_STYLE_LIST_STYLE_TYPE_LOWER_ROMAN:
        low = TRUE;
    case DW_STYLE_LIST_STYLE_TYPE_UPPER_ROMAN:
        i0 = num - 1;
        i1 = i0/10; i2 = i1/10; i3 = i2/10;
        i0 %= 10;   i1 %= 10;   i2 %= 10;
        if (i3 > 4) /* more than 4999 elements ? */
            sprintf(buf, "****.");
        else if (i3)
            sprintf(buf, "%s%s%s%s.", roman_I3[i3-1], roman_I2[i2-1],
                    roman_I1[i1-1], roman_I0[i0]);
        else if (i2)
            sprintf(buf, "%s%s%s.", roman_I2[i2-1],
                    roman_I1[i1-1], roman_I0[i0]);
        else if (i1)
            sprintf(buf, "%s%s.", roman_I1[i1-1], roman_I0[i0]);
        else
            sprintf(buf, "%s.", roman_I0[i0]);
        if (low)
            g_strdown(buf);
        break;
    case DW_STYLE_LIST_STYLE_TYPE_DECIMAL:
    default:
        g_snprintf(buf, buflen, "%d.", num);
        break;
    }
}

