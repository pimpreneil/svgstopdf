/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set sw=4 sts=4 ts=4 expandtab: */
/*
   svgstopdf.c: Command line utility for building multipage PDFs from multiple SVGs
 
   Copyright (C) 2016 Arnaud Lejosne <ad-gh@arnaud-lejosne.com>

   Several parts of the code were insipred by this Gist https://gist.github.com/julian-klode/ac45de6d4d2227d9febc
   as well as LibRSVG's rsvg-convert utility https://git.gnome.org/browse/librsvg/plain/rsvg-convert.c
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
  
   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
  
   Authors: Arnaud Lejosne <ad-gh@arnaud-lejosne.com>
*/

#define _GNU_SOURCE
#include <cairo.h>
#include <cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <librsvg/rsvg.h>
#include <string.h>

static cairo_status_t
rsvg_cairo_write_func (void *closure, const unsigned char *data, unsigned int length)
{
    if (fwrite (data, 1, length, (FILE *) closure) == length)
        return CAIRO_STATUS_SUCCESS;
    return CAIRO_STATUS_WRITE_ERROR;
}

int
main(int argc, char *argv[])
{
    GOptionContext *g_option_context;
    char **args = NULL;
    gint n_args = 0;
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    GError *error = NULL;
    RsvgHandle *rsvg = NULL;
    RsvgHandleFlags flags = RSVG_HANDLE_FLAGS_NONE;
    RsvgDimensionData dimensions;
    gboolean create_latex_package = FALSE;
    FILE *tex_file = NULL;

    GOptionEntry options_table[] = {
        {"latex-package", 0, 0, G_OPTION_ARG_NONE, &create_latex_package, "Create LaTeX package", NULL},
        {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &args, NULL, "[1.svg 2.svg ... out.pdf]"},
        {NULL}
    };

    g_option_context = g_option_context_new ("- SVGs to PDF");
    g_option_context_add_main_entries(g_option_context, options_table, NULL);
    g_option_context_set_help_enabled(g_option_context, TRUE);

    if (!g_option_context_parse(g_option_context, &argc, &argv, &error)) {
        g_option_context_free(g_option_context);
        g_printerr("%s\n", error->message);
        exit(1);
    }

    g_option_context_free(g_option_context);

    if (args) {
        while (args[n_args] != NULL) {
            n_args++;
        }
    }
    if (n_args < 2) {
        g_printerr("Usage: svgstopdf 1.svg 2.svg ... out.pdf\n");
        exit(1);
    }

    char* output = args[n_args - 1];
    int outlen = strlen(output);
    char* point;

    if((point = strrchr(output,'.')) != NULL) {
        if(strcmp(point,".pdf") != 0) {
            g_printerr("The output file should be a .pdf file\n");
            exit(1);
        }
    }


    FILE *output_file = fopen(output, "wb");

    if(!output_file) {
        g_printerr("Error opening output file: %s\n", args[n_args - 1]);
        exit(1);
    }
    if(create_latex_package) {
        tex_file = fopen("svgimages.sty", "wb");

        if(!tex_file) {
            g_printerr("Error opening LaTeX output file: %s\n", args[n_args - 1]);
            exit(1);
        }

        fputs("\\ProvidesPackage{svgimages}\n", tex_file);
        fputs("\\usepackage{xifthen}\n", tex_file);
        fputs("\\usepackage{graphicx}\n", tex_file);
        fputs("\\newcommand{\\ifequals}[3]{\\ifthenelse{\\equal{#1}{#2}}{#3}{}}\n", tex_file);
        fputs("\\newcommand{\\case}[2]{#1 #2}\n", tex_file);
        fputs("\\newenvironment{switch}[1]{\\renewcommand{\\case}{\\ifequals{#1}}}{}\n", tex_file);
        fputs("\\newcommand{\\includesvg}[1]{%\n", tex_file);
        fputs("    \\begin{switch}{#1}%\n", tex_file);
    }

    /*
     * We initialize the surface with DIN A4 default
     */
    surface = cairo_pdf_surface_create_for_stream(rsvg_cairo_write_func, output_file, 595.276, 841.89);
    cr = cairo_create(surface);

    int i;

    for (i = 0; i < n_args - 1; i++) {
        GFile *file;
        GInputStream *stream;

        file = g_file_new_for_commandline_arg(args[i]);
        stream = (GInputStream *) g_file_read(file, NULL, &error);

        rsvg = rsvg_handle_new_from_stream_sync(stream, file, flags, NULL, &error);

        g_clear_object(&stream);
        g_clear_object(&file);

        if (error != NULL) {
            g_printerr("Error reading SVG:");
            g_printerr("%s\n", error->message);
            g_printerr("\n");
            exit(1);
        }
        if (!rsvg_handle_get_dimensions_sub(rsvg, &dimensions, NULL)) {
            g_printerr("Could not get dimensions for file %s\n", args[i]);
            g_printerr("\n");
            exit(1);
        }

        cairo_pdf_surface_set_size(surface, dimensions.width, dimensions.height);

        rsvg_handle_render_cairo_sub(rsvg, cr, NULL);
        cairo_show_page(cr);
        g_object_unref(rsvg);

        if (create_latex_package) {
            char* string;
            asprintf(&string, "        \\case{%s}{\\includegraphics[page=%d]{%s}}%%\n", args[i], i, output);
            fputs(string, tex_file);
            free(string);
        }
    }

    if (create_latex_package) {
        fputs("    \\end{switch}%\n", tex_file);
        fputs("}", tex_file);
        fclose(tex_file);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    fclose(output_file);
    return 0;
}
