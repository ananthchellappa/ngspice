* The prompt face of doc/codex/issues/0053.
*
* A user-defined function whose body is a single node -- a bare constant or
* a bare vector name -- handed the STORED node to the evaluator uncounted,
* and the evaluator freed it.  The first call therefore destroyed the
* definition.  Whether reading it back afterwards is a wrong number or a
* fault is an allocator accident, which is why the two harnesses disagree:
* tests/regression/misc/define-const-body.cir catches the wrong number,
* which exits 0, and this file catches the fault.
*
* This directory is the only one that can see an exit status.  tests/
* regression/pipe has no reference output; TESTS_ENVIRONMENT runs
* `ngspice -p < $file` through `sh -c`, which exec's ngspice, so a fault
* arrives at automake as 139.  Measured at 1eb641b24: this file dies on the
* first `define c` after a call, with valgrind reporting `Invalid free()`.
*
* The value checks below are here as well as the fatal ones because a
* repair that only stops the crash still has to produce the right number.

set prompt = ""
let fail_count = 0

* -- a bare constant at the root
define c(x) 5
if c(2) <> 5
  echo "ERROR: first call of a constant-bodied define"
  let fail_count = fail_count + 1
end
if c(2) <> 5
  echo "ERROR: second call of a constant-bodied define"
  let fail_count = fail_count + 1
end
if c(2) <> 5
  echo "ERROR: third call of a constant-bodied define"
  let fail_count = fail_count + 1
end

* -- listing it after the call read the freed node through prtree1()
define c

* -- and undefining it freed the same node a second time
undefine c

* -- a bare vector name at the root
let a = 5
define g(y) a
if g(1) <> 5
  echo "ERROR: first call of a vector-bodied define"
  let fail_count = fail_count + 1
end
if g(1) <> 5
  echo "ERROR: second call of a vector-bodied define"
  let fail_count = fail_count + 1
end
define g
undefine g

* -- an operator at the root always worked; keep it working
define k(x) 5+0
if k(2) <> 5
  echo "ERROR: first call of an operator-rooted define"
  let fail_count = fail_count + 1
end
if k(2) <> 5
  echo "ERROR: second call of an operator-rooted define"
  let fail_count = fail_count + 1
end

if fail_count > 0
  echo "ERROR: $&fail_count single-node define bodies failed"
  quit 1
else
  echo "INFO: all single-node define bodies passed"
  quit 0
end
