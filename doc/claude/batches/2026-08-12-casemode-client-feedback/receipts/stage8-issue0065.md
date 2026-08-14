# Stage 8 receipt — issue 0065

Filed.

**Path**: `/home/qflow/dev/ngspice_test/doc/codex/issues/0065-a-rawfile-option-line-shadows-a-computed-variable.md` (278 lines, untracked; nothing in `src/` or `tests/` touched by me — the modifications `git diff` shows there belong to a concurrent session, notably 0060's in-flight `curcasemode` arm in `options.c`, which is why the issue pins its `options.c` line numbers to `720c8743a`).

**Title**: `A Rawfile `Option:` Line Shadows a Computed Variable — `SIGSEGV` on a Bare `load``

**Reproduced**: yes, four ways — `-p` pipe and `--batch` `.control`, on `build-ver_50/src/ngspice` and on stock `/usr/local/bin/ngspice` (`ngspice-46`), `rc=139` in all four.

**Crashing frame** (from `gdb` on an unstripped relink of the same objects; the shipped `CFLAGS` carry `-s`, so I relinked, took the trace, then restored the original binary — same BuildID `c69c697d…` back in place, still reproducing):

```
#0  __strcmp_avx2
#1  cp_enqvar        <- options.c:61, eq(vv->va_name, word)
#2  cp_usrvars
#3  cp_getvar
#4  com_display      <- the cp_getvar("nosort") at com_display.c:70
#5  doblock / cp_evloop / app_rl_readlines / main
```

It is a **use-after-free**, not a wrong value. `valgrind` gives all three ends: alloc'd by `cp_setparse ← raw_read ← ft_loadfile ← com_load` (the `Option:` arm, `rawfile.c:481/483`); free'd by `free_struct_variable ← cp_getvar ← out_init ← com_display` (the `moremode` read at `terminal.c:77`); read by the frame above (the `nosort` read one statement later). The consumer that dies is **not** a reader of `$curplot` — nothing in `src/` calls `cp_getvar("curplot"/"plots")`. It is `cp_usrvars()` itself, which runs on *every* `cp_getvar()` regardless of the name asked for. `cp_usrvars()` (`options.c:185-214`) declares `int tbfreed;` at `:188`, passes its address five times, never reads it, and does `tv->va_next = v;` (`:193,:197,:201,:205,:209`) on a node borrowed from `plot_cur->pl_env` — which both rewires the plot's own list and enlists a plot-owned node into a chain all three callers free (`variable.c:674`, `:716/:753/:767/:771`, `:1182`).

**Computed variables exposed**: the whole family, `plots` included — `curplot`, `curplotname`, `curplottitle`, `curplotdate`, `plots` all give `rc=139`; `ngbehavior`, `casemode`, `sourcepath`, `zzz` all give `rc=0`. Exactly `cp_usrvars()`'s five and nothing else, because the `pl_env` scan at `options.c:60` precedes both the `curplot` arm (`:70`) and the `plots` arm (`:90`). The match is byte-exact (`eq()` = `strcmp`), so `Option: CurPlot=HAHA` is `rc=0`.

Three extra measured facts in the issue: the poison is per-plot (with a two-plot raw whose first plot carries the line, `load` is `rc=0` and so is a bare `setplot op1`; `setplot op1` + `display`/`print`/`write` is 139, `echo hi` is 0); `set curplot=HAHA` at the prompt is guarded by `cp_usrset()` dispatch and never reaches `pl_env`; and an **adjacent, different** defect recorded but not filed — `load` a raw with the harmless `Option: zzz=HAHA` then `unset zzz` is also `rc=139`, via `cp_remvar()`'s `US_READONLY` arm (`variable.c:641`) freeing at `:672` a node still linked into `pl_env`. It needs no computed-name collision, so it wants its own number.

Status records that it was found while explaining 0061 to the repo owner, that 0061 takes shape A, and that shape A leaves `rawfile.c:472-483` filing arbitrary names — so it does not close this, and criterion 5 requires the fix to survive it. Resolution is "Unresolved." plus the fix shape: have `cp_usrvars()` honour the `tbfreed` flag it already asks for (copy the borrowed node before linking), which is memory safety only and leaves the shadowing semantics to 0061.