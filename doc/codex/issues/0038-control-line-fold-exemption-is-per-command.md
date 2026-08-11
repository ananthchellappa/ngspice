# Issue: The Fold-Mode Exemption For Control Lines Is Per Command, So A File Written By One Command Cannot Be Read By Another

## Status

Open. Found while writing `tests/regression/casedist/bsource-node-case-report.cir`
for `doc/codex/issues/0036`, which writes a file with `echo` and reads it with
`fopen`. Mode independent in the sense that matters: it is wrong in the
**default** mode and correct in `preserve` and `distinguish`.

## Summary

In the default `casemode=fold`, `src/frontend/inpcom.c:1954-1968` lower cases
every control line **except** those whose first word is on a fixed list:

```
write wrdata codemodel osdi pre_osdi echo shell source cd load setcs
strcmp strstr        (plus .lib and .inc, which are not control lines)
```

`fopen`, `fclose` and `fread` are not on that list, so their arguments are
folded. A control block that writes a file under a name containing upper case
and reads it back therefore looks for a name that was never created:

```spice
* fold folds fopen but not a line beginning with echo
.control
echo hi > MiXeD.txt
fopen f1 MiXeD.txt r
fread line $f1 n1
fclose $f1
echo READ $line
.endc
.end
```

Measured at `8557994bb`, `ngspice --batch`:

| Mode | File created | `fopen` opens | Result |
| --- | --- | --- | --- |
| fold (default) | `MiXeD.txt` | `mixed.txt` | `com_fopen() cannot open mixed.txt: No such file or directory`, then `com_fread(): file handle -1 is not in accepted range.`, and `READ` prints nothing |
| preserve | `MiXeD.txt` | `MiXeD.txt` | `READ hi` |
| distinguish | `MiXeD.txt` | `MiXeD.txt` | `READ hi` |

The failure is not about redirects. A command that is *not* on the exempt
list folds its redirect target as well, so the two halves agree again:

```spice
op >& CaP.txt
fopen f1 CaP.txt r
```

creates `cap.txt`, opens `cap.txt`, and reads it back. The mismatch appears
only when the writing line is exempt and the reading line is not.

## Impact

No wrong numbers, and no silent wrong output: both halves of the failure are
loud on `stderr`. What it costs is that a control script cannot use a mixed
case file name across an `echo`/`source`/`write` and an `fopen` pair in the
mode almost every deck runs in, and the diagnostic names a file the author
never typed, which is the same "names a spelling the deck never wrote"
confusion `doc/claude/decisions/0002-deferred-node-resolution-check.md`
records for the singular-matrix message.

It is also a trap for this repository's own decks. The three decks added for
`doc/codex/issues/0036` write their sub-decks with `echo` and read the
captures with `fopen`, and they use lower-case names for exactly this reason;
the first version used upper-case names to survive
`doc/claude/scripts/case_differential_sweep.py`'s uppercased copy and stopped
working under `fold`.

## Root Cause

The exemption at `inpcom.c:1954` is written as a list of command *names*
rather than as a rule about which *arguments* are file names. It grew by
addition — `write`, `wrdata`, `codemodel`, `osdi`, `pre_osdi`, `echo`,
`shell`, `source`, `cd`, `load`, `setcs`, `strcmp`, `strstr` — and each
addition was made when some deck broke, so a command nobody had yet written a
folding-sensitive deck for is simply absent. `fopen` is one of those.

`doc/claude/scripts/case_differential_sweep.py` has the same list under the
name `FILE_CARDS` and the same gap, arrived at independently.

## Acceptance Criteria

1. A control block that writes a file with `echo`/`write` and reads it with
   `fopen` under a name containing upper case works in `fold`, or the reason
   it may not is recorded.
2. Whatever the fix, it decides between the two directions rather than adding
   one more name to the list: either the file-name arguments of the file
   commands are exempted from folding as a class, or the exemption is removed
   from the writing side so that both halves fold and agree. The second is a
   behaviour change for `write`/`source`/`load` decks with mixed case paths on
   case-sensitive filesystems and needs the differential sweep behind it.
3. A deck in `tests/regression/` covers the pair in `fold`, not only in
   `casedist`.
4. If the list is kept, `FILE_CARDS` in
   `doc/claude/scripts/case_differential_sweep.py` and the list in
   `inpcom.c` are stated to be the same list in one place, so the next
   addition reaches both.

## Resolution

Not fixed. Found and measured while closing `doc/codex/issues/0036`; the three
decks that issue adds avoid it by using lower-case names, which is recorded in
each of them.
