#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

int writememsubfont(int fd, Memsubfont *f); // Not in library??

int main(int argc, char *argv[]) {
	int i, fd, x, y;
	char *input, *output;
	Memsubfont *subfont, *subfont2;
	Rectangle r, r2;
	Memimage *bits, *bits2;
	Fontchar *info, *info2;

	ARGBEGIN{
	default:
		sysfatal("usage: sfdouble input output");
	}ARGEND
	if (argc != 2) {
		sysfatal("usage: sfdouble input output");
	}
	input = argv[0];
	output = argv[1];

	memimageinit();

	subfont = openmemsubfont(input);
	if (subfont == nil) {
		sysfatal("sfdouble: can't read subfont %s: %r\n", input);
	}

	bits = subfont->bits;
	r = subfont->bits->r;
	if (!eqpt(r.min, ZP)) {
		sysfatal("sfdouble: non-zero origin for subfont %s\n", input);
	}
	r2 = Rect(0, 0, 2*r.max.x, 2*r.max.y);

	// double the image.
	bits2 = allocmemimage(r2, bits->chan);
	for (y = 0; y < r.max.y; y++) {
		for (x = 0; x < r.max.x; x++) {
			memdraw(bits2, Rect(2*x, 2*y, 2*x+1, 2*y+1), bits, Pt(x, y), memopaque, ZP, S);
			memdraw(bits2, Rect(2*x+1, 2*y, 2*x+2, 2*y+1), bits, Pt(x, y), memopaque, ZP, S);
			memdraw(bits2, Rect(2*x, 2*y+1, 2*x+1, 2*y+2), bits, Pt(x, y), memopaque, ZP, S);
			memdraw(bits2, Rect(2*x+1, 2*y+1, 2*x+2, 2*y+2), bits, Pt(x, y), memopaque, ZP, S);
		}
	}

	// double the font info.
	info = subfont->info;
	info2 = malloc((subfont->n+1)*sizeof(Fontchar));
	for (i = 0; i < subfont->n; i++) {
		info2[i].x = info[i].x*2;
		info2[i].top = info[i].top*2;
		info2[i].bottom = info[i].bottom*2;
		info2[i].left = info[i].left*2;
		info2[i].width = info[i].width*2;
	}

	// allocate the new subfont.
	subfont2 = allocmemsubfont(output,
		subfont->n,
		2*subfont->height,
		2*subfont->ascent,
		info2,
		bits2);

	// write out the new subfont (the image goes first).
	fd = create(output, 1, 0644);
	if (fd < 0) {
		sysfatal("sfdouble: can't create %s: %r", output);
	}
	if (writememimage(fd, bits2) < 0) {
		sysfatal("sfdouble: can't write image for %s: %r", output);
	}
	if (writememsubfont(fd, subfont2) < 0) {
		sysfatal("sfdouble: can't write font info for %s: %r", output);
	}
	close(fd);
	return 0;
}

// Why is there no writememsubfont? These routines are copied from the draw library.
static
void
packinfo(Fontchar *fc, uchar *p, int n)
{
	int j;

	for(j=0;  j<=n;  j++){
		p[0] = fc->x;
		p[1] = fc->x>>8;
		p[2] = fc->top;
		p[3] = fc->bottom;
		p[4] = fc->left;
		p[5] = fc->width;
		fc++;
		p += 6;
	}
}

int
writememsubfont(int fd, Memsubfont *f)
{
	char hdr[3*12+1];
	uchar *data;
	int nb;

	sprint(hdr, "%11d %11d %11d ", f->n, f->height, f->ascent);
	if(write(fd, hdr, 3*12) != 3*12){
   Err:
		werrstr("writesubfont: bad write: %r");
		return -1;
	}
	nb = 6*(f->n+1);
	data = malloc(nb);
	if(data == nil)
		return -1;
	packinfo(f->info, data, f->n);
	if(write(fd, data, nb) != nb)
		goto Err;
	free(data);
	return 0;
}
