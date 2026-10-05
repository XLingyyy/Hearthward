# Original material suffix boundary follow-up

Status: source-confirmed path; no UE/Native/Stage/HTTP execution. Root is currently validating the already-reproduced No/Once defects. This note does not amend the production guard patch.

The QA compatibility patch field is corrected from G.Mode to the actual G.QuantityMode. No Source was written.

## Confirmed boundary risk

The current BindMaterials selects the longest registered Name/id at the phrase start, removes that token, then returns true whenever the remainder does not start with 和/与/及/、. Thus a recognized 木材 prefix followed by 粉 or 的替代物 can add wood to Authorized despite a nonempty unknown remainder. The clause-boundary fix prevents consent from starting inside 不允许, but does not prevent this material-token prefix issue. This is a direct source-path conclusion; the two new phrases have not been run in Native.

## Smallest conservative candidate

Replace only the terminal line:

```cpp
if(!Joined)return Tail.IsEmpty();
```

Tail has already had leading whitespace removed. This accepts an exact terminal registered material token, or a complete list of such tokens. It rejects unknown trailing noun text after the final token. The same binder also handles No; ambiguous compound prohibitions become unresolved instead of binding a known prefix. No new aliases, sentence classifier, parser, or allowed-suffix dictionary are needed.

The current positive controls end the material phrase at comma/semicolon or the end of the clause; GoalText constraints already stop at the next canonical No/Once/Max label separator. Therefore this candidate preserves those existing controls by source inspection. Native compatibility remains to be executed by Root if approved.

This candidate conservatively refuses an unseparated trailing action, e.g. 这次允许消耗木材制作绳索, and quantified material suffixes until clarified. The caller can provide the already-supported explicit form 这次允许消耗木材，制作一批绳索. Accepting arbitrary trailing Chinese characters would recreate the confirmed prefix ambiguity.

## Narrow prospective assertions

With the existing valid craft/rope/one-batch goal and limits [once:wood]:

- Original 这次允许使用木材粉，制作一批绳索 must fail Contract Validate.
- Original 这次允许使用木材的替代物，制作一批绳索 must fail Contract Validate.
- Existing exact wood/wood-alias permission and canonical GoalText controls must remain valid.

Root can add the two negatives to OriginalOnceAuthorization if it elects to close this source-confirmed boundary. No new test fixture or Stage API is needed. These assertions are suggestions, not a tested or integrated patch.
