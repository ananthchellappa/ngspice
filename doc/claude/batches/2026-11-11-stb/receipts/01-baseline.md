# Receipt 01: baseline before the port

Tree: `ver_53` at `5a5251547` (equal to `ver_52`).

```
./autogen.sh                                            rc 0
../configure --prefix=$PWD/stage --with-readline=yes \
             --enable-pss --enable-cider                rc 0
make -j6                                                rc 0
make install                                            rc 0
env -u DISPLAY make -C build-ver_53 -k check            rc 2
```

`build-ver_53/claude-check-baseline.log`: **363 `PASS:` lines, 33 `FAIL:`
lines.** All 33 are in `tests/regression/pipe`: with `DISPLAY` unset
`ngspice -p` prints `no graphics interface` and the 33 command files of that
directory fail. This is not the known state of the fork (395 PASS, 1 FAIL),
which holds with an X server:

```
xvfb-run -a make -C build-ver_53/tests/regression/pipe -k check
    32 PASS, 1 FAIL: resource-keyword-case.cmd
```

363 + 32 = 395 PASS and 1 FAIL.
