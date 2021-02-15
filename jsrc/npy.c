#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#endif
#include <string.h>

#include "npydict.h"
#include "x.h"

#define NT_MAX_PATH 512

#define ERRZ(exp)                                                              \
  do {                                                                         \
    if (!(exp))                                                                \
      goto error;                                                              \
  } while (0)

#define ASSERTZ(b, e)                                                          \
  do {                                                                         \
    if (!(b)) {                                                                \
      jsignal(e);                                                              \
      goto posterror;                                                          \
    }                                                                          \
  } while (0)

#define GATV0Z(name, type, atoms, rank)                                        \
  GATVS(name, type, atoms, rank, 0, type##SIZE, GACOPYSHAPE0, goto posterror)

/*
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left).  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occurred.
 */
#ifdef SY_LINUX

static size_t
strlcat(char * restrict dst, const char * restrict src, size_t dsize)
{
	const char *odst = dst;
	const char *osrc = src;
	size_t n = dsize;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end. */
	while (n-- != 0 && *dst != '\0')
		dst++;
	dlen = dst - odst;
	n = dsize - dlen;

	if (n-- == 0)
		return(dlen + strlen(src));
	while (*src != '\0') {
		if (n != 0) {
			*dst++ = *src;
			n--;
		}
		src++;
	}
	*dst = '\0';

	return(dlen + (src - osrc));	/* count does not include NUL */
}

#endif

F1(jtfullnamenpy)
{
#if SY_WIN32
  C dirpath[NT_MAX_PATH];
  wchar_t wdirpath[NT_MAX_PATH];
  US* wp;

  RZ(w = str0(vslit(w)));
  ASSERT(AN(w) - 1, EVLENGTH);
  RZ(w = toutf16x(w));
  wp = USAV(w);
  wp[AN(w)] = L'\0';
  ERRZ(_wfullpath(wdirpath, wp, NT_MAX_PATH));
  ERRZ(wcscat_s(wdirpath, NT_MAX_PATH, L".npy") == 0);
  ERRZ(WideCharToMultiByte(CP_UTF8,
                           0,
                           wdirpath,
                           -1, /* null-terminated */
                           dirpath,
                           NT_MAX_PATH,
                           NULL,
                           NULL));
#else
  C dirpath[PATH_MAX];
  C* p;

  RZ(w = str0(vslit(w)));
  ASSERT(AN(w) - 1, EVLENGTH);
  p = CAV(w);
  ERRZ(realpath(p, dirpath) == NULL);
  ERRZ(strlcat(dirpath, ".npy", PATH_MAX) < PATH_MAX);
#endif
  R cstr(dirpath);

error:
  R jerrno();
}

F1(jtnpydescr)
{
  RZ(w);
  switch (CTTZ(AT(w))) {
    case B01X:
      return cstr("'|b1'");
    case LITX:
      return cstr("'|u1'");
    case INTX:
      return cstr("'<i8'");
    case FLX:
      return cstr("'<f8'");
    case CMPXX:
      return cstr("'<c16'");
    case C2TX:
      return cstr("'<u2'");
    case C4TX:
      return cstr("'<u4'");
    default:
      ASSERTD(0, "arg type");
  }
}

#if !SY_WIN32
#define sprintf_s snprintf
#define fprintf_s fprintf
typedef int errno_t;
#endif

F1(jtnpyshape)
{
  RZ(w);
  // maximum digits of AS(w)[i] is 19
  size_t sz = 2 + 20 * (size_t)AR(w) + (AR(w) < 2);
  A z;
  GATV0(z, LIT, sz, 1);
  RANKT i = 0;
  char *p = CAV(z), *ep = p + sz - 1;

  *p++ = '(';

  switch (AR(w)) {
    case 0:
      for (; i < AR(w); ++i) {
        *p++ = ',';
        default:
          p += sprintf_s(p, ep - p, FMTI, AS(w)[i]);
      }
      break;
    case 1:
      p += sprintf_s(p, ep - p, FMTI ",", AS(w)[0]);
      break;
  }

  *p++ = ')';
  *p = '\0';

  AN(z) = AS(z)[0] = ((C*)p - CAV(z));
  RETF(z);
}

F2(jtnpysave)
{
  F f;
  RZ(w);
  ASSERT(BOX & AT(w), EVDOMAIN);

  // add .npy extension and open the file
  RZ(w = box(fullnamenpy(AAV(w)[0])));
  RZ(f = jope(w, FWRITE_O));

  // form header
  A descr, shape;
  RZ(descr = npydescr(a));
  RZ(shape = npyshape(a));
  char const header_tmpl[] =
    "{'descr': %s, 'fortran_order': False, 'shape': %s, }";
  UI4 len = (UI4)(sizeof(header_tmpl) - 5 + AN(descr) + AN(shape));

  // write magic
  ERRZ(fwrite("\x93NUMPY", 1, 6, f) == 6);
  ERRZ(fputc(2, f) != EOF); // major version
  ERRZ(fputc(0, f) != EOF); // minor version

  // write header length in v2
  UI4 nbytes = 8 + sizeof(nbytes);
  nbytes = ((nbytes + len) / 16 + 1) * 16 - nbytes; // leave room for \n
  ERRZ(fwrite(&nbytes, sizeof(nbytes), 1, f) == 1);

  // write header
  ERRZ(fprintf_s(f, header_tmpl, CAV(descr), CAV(shape)) != -1);
  nbytes -= len;
  ERRZ(fwrite("               \n" + (16 - nbytes), 1, nbytes, f) == nbytes);

  // write ravel
  ERRZ(fwrite(AV(a), bp(AT(a)), AN(a), f) == AN(a));

  if (0) {
  error:
    w = jerrno();
  }

  fclose(f);
  R w;
}

static void
inplace_reverse(size_t n, I* a)
{
  DO(n / 2, --n; I tmp = a[i]; a[i] = a[n]; a[n] = tmp;);
}

struct npyheader
{
  UC magic[6];
  UC version[2];
  union
  {
    U2 size_v1;
    UI4 size_v2;
  };
};

/*
 * dict:
 *   { pair-list ,opt }
 *
 * pair-list:
 *   pair
 *   pair-list , pair
 *
 * pair:
 *   descr-info
 *   shape-info
 *   fortran-order-info
 *
 * descr-info:
 *   'descr' : ' typestr '
 *
 * typestr: "|b1" | "|u1" | "<i8" | "<f8" | "<c16" | "<u2" | "<u4"
 * boolean: True | False
 *
 * fortran-order-info:
 *   'fortran_order' : boolean
 *
 * shape-info:
 *   'shape' : ( )
 *   'shape' : ( integer , )
 *   'shape' : ( rank-list )
 *
 * rank-list:
 *   integer , integer
 *   rank-list , integer
 *
 * integer:
 *   nonzero-digit digit-sequence_opt
 *
 * nonzero-digit: [1-9]
 * digit: [0-9]
 *
 * digit-sequence:
 *   digit
 *   digit-sequence digit
 */

F1(jtnpyload)
{
  F f;
  RZ(w);
  ASSERT(BOX & AT(w), EVDOMAIN);

  // add .npy extension and open the file
  RZ(w = box(fullnamenpy(AAV(w)[0])));
  RZ(f = jope(w, FREAD_O));

  struct npyheader h;
  A z = 0;

  // read magic and version
  ERRZ(fread(&h, 1, 8, f) == 8);
  ASSERTZ(memcmp(h.magic, "\x93NUMPY", 6) == 0, EVSECURE);

  // determine header size
  if (h.version[0] == 1 && h.version[1] == 0)
    ERRZ(fread(&h.size_v1, sizeof(h.size_v1), 1, f) == 1);
  else if (h.version[0] == 2 && h.version[1] == 0)
    ERRZ(fread(&h.size_v2, sizeof(h.size_v2), 1, f) == 1);
  else
    ASSERTZ(0, EVSECURE);

  // read header
  UI4 size = h.version[0] == 1 ? h.size_v1 : h.size_v2;
  A s;
  GATV0Z(s, LIT, size, 1);
  ERRZ(fread(CAV(s), 1, size, f) == size);
  AN(s) = AS(s)[0] = size;

  // parse header
  j_args_t args = { .jt = jt, .s = s };
  npydict_context_t* ctx = npydict_create(&args);
  errno = 0;
  int r = npydict_parse(ctx, NULL);
  errno_t ec = errno;
  errno = 0;
  ASSERTZ(r == 0, EVSECURE);
  npydict_destroy(ctx);
  ASSERTZ(args.d.shape, EVSECURE);
  ASSERTZ(ec != ERANGE, EVLIMIT);

  // read ravel
  I m = prod(AN(args.d.shape), AV(args.d.shape));
  GATVS(z,
        args.d.descr,
        m,
        AN(args.d.shape),
        AV(args.d.shape),
        bp(args.d.descr),
        GACOPYSHAPE,
        goto posterror);
  ERRZ(fread(AV(z), bp(AT(z)), AN(z), f) == AN(z));

  if (args.d.fortran_order) {
    fclose(f);
    inplace_reverse(AR(z), AS(z));
    R cant1(z);
  }

  if (0) {
  error:
    z = jerrno();
  }

posterror:
  fclose(f);
  R z;
}
