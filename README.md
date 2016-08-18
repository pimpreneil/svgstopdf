# SVGs to PDF

A simple command allowing to convert multiple SVG files into a single multi-paged PDF.

It is mainly based on `librsvg` and therefore, also on `cairo`.

In order to compile it on Debian-based systems, you will need the `librsvg2-dev` package as well as its dependencies. You can then simply execute `make` without parameters to compile as binary and then optionally `make install` to make the command available system-wide (copy in `/usr/bin/`).

This tool is more efficient than converting the SVGs into separate PDFs before merging them together as it does not duplicate font subsets.

Here is an example of use of this script:

```bash
svgstopdf 1.svg 2.svg out.pdf
```

The main purpose of this script is to avoid loading duplicate font subsets while including several vector images in a LaTeX document. After generating the PDF from various SVGs, you can load them easily using the LaTeX `graphicx` package:

```latex
\includegraphics[page=1]{out.pdf}
\includegraphics[page=2]{out.pdf}
```
