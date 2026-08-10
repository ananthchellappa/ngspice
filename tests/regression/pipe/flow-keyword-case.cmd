* Control flow keywords typed interactively must be recognised whatever
* their case, just like the command names the dispatcher already matches
* with strcasecmp. A deck cannot express this: every .control line is
* lowercased at read time, so the keywords must be driven through stdin.
set prompt = ""
let fail_count = 0

* IF must not run the body of a false condition
let n = 0
IF 0
  let n = n + 1
END
if n <> 0
   echo "ERROR: uppercase IF ran the body of a false condition"
   let fail_count = fail_count + 1
end

* IF/ELSE must take the else branch
let e = 0
IF 0
  let e = 1
ELSE
  let e = 2
END
if e <> 2
   echo "ERROR: uppercase ELSE branch not taken"
   let fail_count = fail_count + 1
end

* WHILE must loop
let m = 0
let go = 1
WHILE go
  let m = m + 1
  if m = 3
    let go = 0
  end
END
if m <> 3
   echo "ERROR: uppercase WHILE did not loop"
   let fail_count = fail_count + 1
end

* DOWHILE must loop
let d = 0
let go2 = 1
DOWHILE go2
  let d = d + 1
  if d = 2
    let go2 = 0
  end
END
if d <> 2
   echo "ERROR: uppercase DOWHILE did not loop"
   let fail_count = fail_count + 1
end

* REPEAT must repeat
let r = 0
REPEAT 3
  let r = r + 1
END
if r <> 3
   echo "ERROR: uppercase REPEAT did not repeat"
   let fail_count = fail_count + 1
end

* FOREACH must iterate
let k = 0
FOREACH v 1 2 3
  let k = k + 1
END
if k <> 3
   echo "ERROR: uppercase FOREACH did not iterate"
   let fail_count = fail_count + 1
end

* BREAK must leave the loop early
let b = 0
REPEAT 5
  let b = b + 1
  if b = 2
    BREAK
  end
END
if b <> 2
   echo "ERROR: uppercase BREAK did not leave the loop"
   let fail_count = fail_count + 1
end

* CONTINUE must skip the rest of the body
let c = 0
let i2 = 0
REPEAT 3
  let i2 = i2 + 1
  CONTINUE
  let c = c + 1
END
if i2 <> 3
   echo "ERROR: uppercase CONTINUE broke the loop"
   let fail_count = fail_count + 1
end
if c <> 0
   echo "ERROR: uppercase CONTINUE did not skip the rest of the body"
   let fail_count = fail_count + 1
end

* The block terminators endif/endwhile/endforeach/endrepeat/enddowhile are
* aliases of 'end', installed in cpitf.c, so they have to be recognised
* case insensitively too. Otherwise an uppercase IF opens a block that an
* uppercase ENDIF cannot close and the session swallows everything that
* follows. The trailing END keeps this check free of that hazard: whichever
* of the two closes the block, parsing recovers on the next line.
let endif_ok = 0
IF 0
  let dummy = 1
ENDIF
let endif_ok = 1
END
if endif_ok <> 1
   echo "ERROR: uppercase ENDIF did not close the block"
   let fail_count = fail_count + 1
end

* An alias name is a command name, so it is looked up case insensitively
unalias hello
alias HELLO let alias_ok = 1
let alias_ok = 0
hello
if alias_ok <> 1
   echo "ERROR: alias name not matched case insensitively"
   let fail_count = fail_count + 1
end
unalias hello

if fail_count > 0
  echo "ERROR: $&fail_count uppercase flow keyword cases failed"
  quit 1
else
  echo "INFO: all uppercase flow keyword cases passed"
  quit 0
end
