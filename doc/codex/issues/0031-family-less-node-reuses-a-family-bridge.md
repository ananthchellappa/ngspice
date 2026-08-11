# Issue: A Node That Names No Family Reuses Another Node's Family Bridge

## Status

Open. Found while writing the RED deck for `doc/codex/issues/0029` defect (c),
by running the same circuit with the two digital output nodes in the opposite
order. Case independent: it reproduces in the default `fold` mode and the
`casemode` variable is not involved anywhere in the path.

## Summary

`find_bridge()` (`src/xspice/evt/evtcheck_nodes.c:552-577`) looks for an
existing bridge with the same `udn_index` and `direction` before building a
new one. The match is asymmetric:

```c
/* src/xspice/evt/evtcheck_nodes.c:554-577 */
    for (bridge = *bridge_list_p; bridge; bridge = bridge->next) {
        if (bridge->udn_index == event_node->udn_index &&
            bridge->direction == direction) {
            if (family) {
                if (bridge->family && !strcmp(family, bridge->family)) {
                    ...
                }
            } else if (bridge->vcc == vcc) {            // Match vcc.
                break;
            }
        }
    }
```

A node that **does** name a family matches only a bridge of the same family —
that is what the `strcmp` is for, and `0029` (c) added the `bridge->family &&`
that keeps a family-less bridge from being mistaken for one. A node that names
**no** family takes the `else if` and matches on `vcc` alone, so it matches the
first bridge of the right type and direction whatever family that bridge was
built for.

## Impact

The node gets a different device from the one it asked for, silently, and
which device depends on the order the event nodes happen to be in.

Measured on branch `ver_50` at commit `6fdb654df` with the two nodes of
`tests/xspice/digital/auto-bridge-family-null.cir` swapped, so that the
family-bearing node is examined first:

```
* family node first, family-less node second
.control
pre_set auto_bridge_demo_d_out =
+ ( ".model auto_da dac_bridge(out_high='%g/2' out_low=0)"
+   "auto_bridge%d [ %s ] [ %s ] auto_da" 1000 )
.endc
V1 in 0 dc 1
R0 in 0 1meg
A1 [in] [d1 d2] adc1
.model adc1 adc_bridge(in_low=0.4 in_high=0.6)
A2 d1 fancy fbuf
.model fbuf d_buffer family="demo"
A3 d2 plain pbuf
.model pbuf d_buffer
R1 plain 0 1k
R2 fancy 0 1k
```

prints

```
v(plain) = 1.650000e+00
v(fancy) = 1.650000e+00
```

`plain` names no family and should get the built-in default `dac_bridge` with
`out_high = vcc = 3.3`, which is what it prints when it is the first of the two
nodes. Instead it gets the `demo` family's `out_high = vcc/2`. Both spellings
of the deck are legal and the only difference between them is node order.

## Root Cause

`bridge->family` is part of a bridge's identity in one direction only. The
`family` case tests it; the `else if (bridge->vcc == vcc)` case does not. The
family-less node's requirement is not "some bridge with this vcc" but "the
bridge with this vcc **and no family**".

## Acceptance Criteria

- A digital output node whose model names no family gets the built-in default
  bridge regardless of whether a family bridge for the same `udn_index`,
  `direction` and `vcc` was created first.
- The deck above prints `v(plain) = 3.300000e+00` and
  `v(fancy) = 1.650000e+00` in both node orders.
- A regression deck in `tests/xspice/digital/`, next to
  `auto-bridge-family-null.cir`, which is the same circuit in the other order.

## Resolution

Not fixed. The one-line shape is `} else if (!bridge->family && bridge->vcc ==
vcc) {`, which makes the match symmetric, but it is a behaviour change for
decks that have no case defect at all and it is not what `0029` is about, so it
is recorded rather than folded into the `0029` (c) commit.
