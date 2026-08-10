* The window-type keywords of the "spec" command (src/frontend/spec.c) must
* be recognised whatever their case.
*
* com_spec() does not read the window name from its own argument list: it
* reads the shell variable "specwindow" with cp_getvar() and then compares
* the string with eq() -- a case sensitive strcmp -- against "none",
* "rectangular", "hanning", "cosine", "hamming", "triangle", "bartlet",
* "blackman" and "gaussian". Anything else falls into the final else and
* aborts the command.
*
* A deck cannot put an uppercase value in front of those comparisons: every
* line read by inp_read() is lowercased in place, so ".control / set
* specwindow = HANNING" already arrives as "hanning". The cp shell input
* path -- the interactive prompt, "ngspice -p" reading stdin, which is what
* this suite drives -- never goes through that fold, so a typed
* "set specwindow = HANNING" is the only way to reach spec.c with a
* capitalised window name.
*
* The circuit is built with circbyline so the test needs no companion .cir.
* Those lines are deck lines and are folded, which keeps the case of the
* typed "set" as the only variable.
*
* Every check is on a value, never on a diagnostic. Two things are asserted
* for each keyword:
*
*   1. the spectrum really was produced. When the window name is rejected
*      com_spec() jumps to its "done" label before plot_alloc(), so no new
*      plot is created and the current plot is still the transient one.
*      length(v(out)) is then the transient record length (>1000) instead
*      of the 11 points of the spectrum. The vector is seeded with 99 first
*      so that a "let" that cannot evaluate its right hand side fails the
*      check as well instead of leaving a stale value behind.
*
*   2. the same window was selected as for the lowercase spelling. The
*      lowercase magnitude is stashed in a shell variable (let vectors live
*      in the plot that was current when they were created, and "spec"
*      switches plots, so a vector cannot carry the reference across).
*
* Each keyword is also exercised in lowercase, and the lowercase spelling is
* asserted the same way, so a check that fails for both spellings cannot be
* mistaken for a case-sensitivity failure.
*
* Checks are fail-fast: a rejected window leaves the session in the
* transient plot, and continuing would compare unrelated numbers.
set prompt = ""

circbyline * inline source for the spec window keyword checks
circbyline .options noacct
circbyline V1 in 0 dc 0 sin(0 1 100k)
circbyline R1 in out 1k
circbyline C1 out 0 1n
circbyline .end

* 1.5 ms of data with a 1 kHz frequency step: the analysed span is truncated
* to 1 ms, so the windows that zero the samples older than the span really do
* differ from the ones that do not. Without that the "none" and "rectangular"
* windows would produce identical numbers.
tran 1u 1.5m

* --- spec.c:93   eq(window, "none") ------------------------------------
setplot tran1
set specwindow = none
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
set ref_none = $&v
if n <> 11
  echo "ERROR: lowercase specwindow=none rejected by spec, length = $&n"
  quit 1
end
setplot tran1
set specwindow = NONE
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=NONE rejected by spec (spec.c:93), length = $&n"
  quit 1
end
if abs(v - $ref_none) > 1e-4 * abs($ref_none)
  echo "ERROR: uppercase specwindow=NONE did not select the none window, $&v vs $ref_none"
  quit 1
end

* --- spec.c:96   eq(window, "rectangular") -----------------------------
setplot tran1
set specwindow = rectangular
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
set ref_rect = $&v
if n <> 11
  echo "ERROR: lowercase specwindow=rectangular rejected by spec, length = $&n"
  quit 1
end
setplot tran1
set specwindow = RECTANGULAR
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=RECTANGULAR rejected by spec (spec.c:96), length = $&n"
  quit 1
end
if abs(v - $ref_rect) > 1e-4 * abs($ref_rect)
  echo "ERROR: uppercase specwindow=RECTANGULAR did not select the rectangular window, $&v vs $ref_rect"
  quit 1
end

* --- spec.c:104  eq(window, "hanning") ---------------------------------
setplot tran1
set specwindow = hanning
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
set ref_hann = $&v
if n <> 11
  echo "ERROR: lowercase specwindow=hanning rejected by spec, length = $&n"
  quit 1
end
setplot tran1
set specwindow = HANNING
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=HANNING rejected by spec (spec.c:104), length = $&n"
  quit 1
end
if abs(v - $ref_hann) > 1e-4 * abs($ref_hann)
  echo "ERROR: uppercase specwindow=HANNING did not select the hanning window, $&v vs $ref_hann"
  quit 1
end

* --- spec.c:104  eq(window, "cosine"), the hanning alias ----------------
setplot tran1
set specwindow = cosine
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: lowercase specwindow=cosine rejected by spec, length = $&n"
  quit 1
end
if abs(v - $ref_hann) > 1e-4 * abs($ref_hann)
  echo "ERROR: lowercase specwindow=cosine is not the hanning window, $&v vs $ref_hann"
  quit 1
end
setplot tran1
set specwindow = COSINE
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=COSINE rejected by spec (spec.c:104), length = $&n"
  quit 1
end
if abs(v - $ref_hann) > 1e-4 * abs($ref_hann)
  echo "ERROR: uppercase specwindow=COSINE did not select the hanning window, $&v vs $ref_hann"
  quit 1
end

* --- spec.c:112  eq(window, "hamming") ---------------------------------
setplot tran1
set specwindow = hamming
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
set ref_hamm = $&v
if n <> 11
  echo "ERROR: lowercase specwindow=hamming rejected by spec, length = $&n"
  quit 1
end
setplot tran1
set specwindow = HAMMING
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=HAMMING rejected by spec (spec.c:112), length = $&n"
  quit 1
end
if abs(v - $ref_hamm) > 1e-4 * abs($ref_hamm)
  echo "ERROR: uppercase specwindow=HAMMING did not select the hamming window, $&v vs $ref_hamm"
  quit 1
end

* --- spec.c:120  eq(window, "triangle") --------------------------------
setplot tran1
set specwindow = triangle
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
set ref_tri = $&v
if n <> 11
  echo "ERROR: lowercase specwindow=triangle rejected by spec, length = $&n"
  quit 1
end
setplot tran1
set specwindow = TRIANGLE
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=TRIANGLE rejected by spec (spec.c:120), length = $&n"
  quit 1
end
if abs(v - $ref_tri) > 1e-4 * abs($ref_tri)
  echo "ERROR: uppercase specwindow=TRIANGLE did not select the triangle window, $&v vs $ref_tri"
  quit 1
end

* --- spec.c:120  eq(window, "bartlet"), the triangle alias --------------
setplot tran1
set specwindow = bartlet
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: lowercase specwindow=bartlet rejected by spec, length = $&n"
  quit 1
end
if abs(v - $ref_tri) > 1e-4 * abs($ref_tri)
  echo "ERROR: lowercase specwindow=bartlet is not the triangle window, $&v vs $ref_tri"
  quit 1
end
setplot tran1
set specwindow = BARTLET
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=BARTLET rejected by spec (spec.c:120), length = $&n"
  quit 1
end
if abs(v - $ref_tri) > 1e-4 * abs($ref_tri)
  echo "ERROR: uppercase specwindow=BARTLET did not select the triangle window, $&v vs $ref_tri"
  quit 1
end

* --- spec.c:128  eq(window, "blackman") --------------------------------
setplot tran1
set specwindow = blackman
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
set ref_black = $&v
if n <> 11
  echo "ERROR: lowercase specwindow=blackman rejected by spec, length = $&n"
  quit 1
end
setplot tran1
set specwindow = BLACKMAN
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=BLACKMAN rejected by spec (spec.c:128), length = $&n"
  quit 1
end
if abs(v - $ref_black) > 1e-4 * abs($ref_black)
  echo "ERROR: uppercase specwindow=BLACKMAN did not select the blackman window, $&v vs $ref_black"
  quit 1
end

* --- spec.c:138  eq(window, "gaussian") --------------------------------
setplot tran1
set specwindow = gaussian
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
set ref_gauss = $&v
if n <> 11
  echo "ERROR: lowercase specwindow=gaussian rejected by spec, length = $&n"
  quit 1
end
setplot tran1
set specwindow = GAUSSIAN
spec 0 10k 1k v(out)
let n = 99
let n = length(v(out))
let v = 99
let v = mag(v(out))[3]
if n <> 11
  echo "ERROR: uppercase specwindow=GAUSSIAN rejected by spec (spec.c:138), length = $&n"
  quit 1
end
if abs(v - $ref_gauss) > 1e-4 * abs($ref_gauss)
  echo "ERROR: uppercase specwindow=GAUSSIAN did not select the gaussian window, $&v vs $ref_gauss"
  quit 1
end

* The nine window names must not all collapse onto one another: if they did,
* the comparisons above would pass whatever branch was taken. none,
* rectangular, hanning, hamming, triangle, blackman and gaussian are seven
* distinct results here, so require the neighbouring pairs to differ.
setplot tran1
let d = 99
let d = abs($ref_none - $ref_rect)
if d < 1e-6
  echo "ERROR: the none and rectangular windows are indistinguishable here, $&d"
  quit 1
end
let d = 99
let d = abs($ref_hann - $ref_hamm)
if d < 1e-9
  echo "ERROR: the hanning and hamming windows are indistinguishable here, $&d"
  quit 1
end
let d = 99
let d = abs($ref_tri - $ref_black)
if d < 1e-9
  echo "ERROR: the triangle and blackman windows are indistinguishable here, $&d"
  quit 1
end

echo "INFO: all uppercase spec window keyword cases passed"
quit 0
